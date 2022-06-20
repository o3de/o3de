"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A wrapper to simplify invoking CTest with common parameters and sub-filter to specific suites
"""
import argparse
import multiprocessing
import os
import result_processing.result_processing as rp
import subprocess
import sys
import shutil

SUITES_AND_DESCRIPTIONS = {
    "smoke": "Quick across-the-board set of tests designed to check if something is fundamentally broken",
    "main": "The default set of tests, covers most of all testing.",
    "periodic": "Tests which can take a long time and should be done periodially instead of every commit - these should not block code submission",
    "benchmark": "Benchmarks - instead of pass/fail, these collect data for comparison against historic data",
    "sandbox": "Flaky/Intermittent failing tests, this is used as a temporary spot to hold flaky tests, this will not block code submission. Ideally, this suite should always be empty",
    "awsi": "Time consuming AWS integration end-to-end tests"
}

BUILD_CONFIGURATIONS = [
    "profile",
    "debug",
    "release",
]


def _regex_matching_any(words):
    """
    :param words: iterable of strings to match
    :return: a regex with groups to match each string
    """
    return "^(" + "|".join(words) + ")$"


def run_single_test_suite(suite, ctest_path, cmake_build_path, build_config, disable_gpu, only_gpu, generate_xml, repeat, extra_args):
    """
    Starts CTest to filter down to a specific suite
    :param suite: subset of tests to run, see SUITES_AND_DESCRIPTIONS
    :param ctest_path: path to ctest.exe
    :param cmake_build_path: path to build output
    :param build_config: cmake build variant to select
    :param disable_gpu: optional, run only non-gpu tests
    :param only_gpu: optional, run only gpu-required tests
    :param generate_xml: optional, enable to produce the CTest xml file
    :param repeat: optional, number of times to run the tests in the suite
    :param extrargs: optional, forward args to ctest
    :return: CTest exit code
    """
    ctest_command = [
        ctest_path,
        "--build-config", build_config,
        "--output-on-failure",
        "--parallel", str(multiprocessing.cpu_count()),  # leave serial vs parallel scheduling to CTest via set_tests_properties()
        "--no-tests=error",
    ]

    label_excludes = []
    label_includes = []

    # ctest can't actually do "AND" queries in label include and name-include, if any
    # match, it will accept them.  In addition, the regex language it uses does 
    # not include positive lookahead to use workarounds...
    # So if someone is asking for  "main" AND "requires_gpu"
    # the only way to do this is to exclude all OTHER suites
    # to solve this problem generally, we will always exclude all other suites than
    # the one being tested.

    for label_name in SUITES_AND_DESCRIPTIONS.keys():
        if label_name != suite:
            label_excludes.append(f"SUITE_{label_name}")
    
    # only one of these can be true, or neither.  If neither, we apply no REQUIRES_* filter.
    if only_gpu:
        label_includes.append("REQUIRES_gpu")
    elif disable_gpu:
        label_excludes.append("REQUIRES_gpu")

    union_regex = _regex_matching_any(label_includes) if label_includes else None
    difference_regex = _regex_matching_any(label_excludes) if label_excludes else None

    if union_regex:
        ctest_command.append("--label-regex")
        ctest_command.append(union_regex)
    if difference_regex:
        ctest_command.append("--label-exclude")
        ctest_command.append(difference_regex)
    if generate_xml:
        ctest_command.append('-T')
        ctest_command.append('Test')

    for extra_arg in extra_args:
        ctest_command.append(extra_arg)

    ctest_command_string = ' '.join(ctest_command) # ONLY used for display
    print(f"Executing CTest {repeat} time(s) with command:\n"
          f"  {ctest_command_string}\n"
          "in working directory:\n"
          f"  {cmake_build_path}\n")

    error_code = 0

    if repeat:
        # Run the tests multiple times. Previous test results are deleted, new test results are combined in a file per
        # test runner.

        test_result_prefix = 'Repeat'
        repeat = int(repeat)
        if generate_xml:
            rp.clean_test_results(cmake_build_path)

        for iteration in range(repeat):
            print(f"Executing CTest iteration {iteration + 1}/{repeat}")
            result = subprocess.run(ctest_command, shell=False, cwd=cmake_build_path, stdout=sys.stdout, stderr=sys.stderr)

            if generate_xml:
                rp.rename_test_results(cmake_build_path, test_result_prefix, iteration + 1, repeat)
            if result.returncode:
                error_code = result.returncode

        if generate_xml:
            rp.collect_test_results(cmake_build_path, test_result_prefix)
            summary = rp.summarize_test_results(cmake_build_path, repeat)
            
            print()  # empty line
            print('Test stability summary:')
            if summary:
                print('The following test(s) failed:')
                for line in summary: print(line)
            else:
                print(f'All tests were executed {repeat} times and passed 100% of the time.')

    else:
        # Run the tests one time. Previous test results are not deleted but
        # might be overwritten.
        result = subprocess.run(ctest_command, shell=False, cwd=cmake_build_path, stdout=sys.stdout, stderr=sys.stderr)
        error_code = result.returncode
    
    return error_code


def main():
    # establish defaults
    ctest_version = "3.17.0"
    if sys.platform == "win32":
        ctest_build = "Windows"
        ctest_relpath = "bin"
        ctest_exe = "ctest.exe"
    elif sys.platform.startswith("linux"):
        ctest_build = "Linux"
        ctest_relpath = "bin"
        ctest_exe = "ctest"
    elif sys.platform.startswith('darwin'):
        ctest_build = "Mac"
        ctest_relpath = "CMake.app/Contents/bin"
        ctest_exe = "ctest"
    else:
        raise NotImplementedError(f"CTest is not currently configured for platform '{sys.platform}'")

    current_script_path = os.path.dirname(__file__)
    dev_default = os.path.dirname(current_script_path)
    thirdparty_default = os.path.join(os.path.dirname(dev_default), "3rdParty")

    # if a specific known location contains cmake, we'll use it
    ctest_default = os.path.join(thirdparty_default, "CMake", ctest_version, ctest_build, ctest_relpath, ctest_exe)
    
    # parse args, with defaults
    parser = argparse.ArgumentParser(
        description="CTest CLI driver: simplifies providing common arguments to CTest",
        # extra wide help messages to avoid newlines appearing in path defaults, which break copy-paste of paths
        formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(prog, width=4096),
        epilog="(Unrecognised parameters will be sent to ctest directly)"
    )

    parser.add_argument('-x', '--ctest-executable', 
                        help="Override path to the CTest executable (will use PATH env otherwise)")
    parser.add_argument('-B', '--build-path', # -B to match cmake's syntax for same thing.
                        help="Path to a CMake build folder (generated by running cmake).",
                        required=True)
    parser.add_argument('--config', choices=BUILD_CONFIGURATIONS, default="debug", # --config to match cmake
                        help="CMake variant build configuration to target (debug/profile/release)")
    parser.add_argument('-s', '--suite', choices=SUITES_AND_DESCRIPTIONS.keys(), 
                        default="main",
                        help="Which subset of tests to execute")
    parser.add_argument('--generate-xml', action='store_true', 
                        help='Enable this option to produce the CTest xml file.')
    parser.add_argument('-r', '--repeat', help="Run the tests the specified number times to identify intermittent "
                                               "failures (e.g. --repeat 3 for running the test three times). When used"
                                               "with --generate-xml, the resulting test reports will be combined, "
                                               "aggregated and summarized.", type=int)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--no-gpu', action='store_true',
                        help="Disable tests that require a GPU")
    group.add_argument('--only-gpu', action='store_true',
                        help="Run only tests that require a GPU")

    args, unknown_args = parser.parse_known_args()

    # handle the CTEST executable.
    # we always obey command line, and its an error if the command line has
    # a bad executable
    # if no command line is specified, it will fallback to a known good location
    # and then finally, use the PATH.
    if args.ctest_executable and not os.path.exists(args.ctest_executable):
        print(f"Error: Invalid ctest executable specified - not found: {args.ctest_executable}")
        return 1
    if not args.ctest_executable:
        # try the default
        if os.path.exists(ctest_default):
            print(f"Using default CTest executable: {ctest_default}")
            args.ctest_executable = ctest_default
        else: # try the PATH env var:
            found_ctest = shutil.which(ctest_exe)
            if found_ctest:
                print(f"Using CTest executable from PATH: {found_ctest}")
                args.ctest_executable = found_ctest
            else:
                print(f"Could not find CTest Executable ('{ctest_exe}')on PATH or in a pre-set location.")
                return 1
    
    # handle the build path.  You must specify a build path, and it must contain CTestTestfile.cmake
    if not os.path.exists(args.build_path):
        print(f"Error: specified folder does not exist: {args.build_path}")
        return 1
    ctest_testfile = os.path.join(args.build_path, "CTestTestfile.cmake")
    if not os.path.exists(ctest_testfile):
        print(f"Error: '{ctest_testfile}' missing, run CMake configure+generate on the folder first.")
        return 1

    print(f"Starting '{args.suite}' suite: {SUITES_AND_DESCRIPTIONS[args.suite]}")

    # execute
    return run_single_test_suite(
        suite=args.suite,
        ctest_path=args.ctest_executable,
        cmake_build_path=args.build_path,
        build_config=args.config,
        disable_gpu=args.no_gpu,
        only_gpu=args.only_gpu,
        generate_xml=args.generate_xml,
        repeat=args.repeat,
        extra_args=unknown_args)


if __name__ == "__main__":
    sys.exit(main())
