"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
This file generates automated asset load benchmark timings for different asset loading scenarios.  It uses the launcher 
so that it can be run on any platform.  The test works as follows:
- Starts with a series of predefined "benchmarksettings" asset files that are used by the Asset Processor to generate 
  "benchmark" files for loading
- These get copied to the current project, where AssetProcessor will process them
- After they're processed, the Launcher is launched and tries to load each set of benchmark assets N times in a row
- Once that's done, the results are gathered and summarized into a CSV
"""

import os
import shutil
import pytest
import logging
import re
from statistics import mean, median
from ctypes import *
from ctypes.wintypes import *

pytest.importorskip("ly_test_tools")

import time as time

import ly_test_tools.launchers.launcher_helper as launcher_helper
import ly_remote_console.remote_console_commands as remote_console_commands
from ly_remote_console.remote_console_commands import (
    send_command_and_expect_response as send_command_and_expect_response,
)
import ly_test_tools.environment.waiter as waiter
from automatedtesting_shared.network_utils import check_for_listening_port
from automatedtesting_shared.file_utils import delete_check

remote_console_port = 4600
remote_command_timeout = 20
listener_timeout = 120

logger = logging.getLogger(__name__)

# Number of times to run each benchmark test.
benchmark_trials = 5

class Benchmark:
    """ 
    Data structure used to define the list of benchmark tests we want to run.
    
    :attr base_name: The base name of the .benchmarksettings test file. This is also used as 
                     a tag when logging.
    :attr num_copies: Number of copies to make of the .benchmarksettings test file; the 
                      generated .benchmark files will all get loaded separately in parallel.
    :attr load_times: The set of load times that have been captured.
    """
    def __init__(self, base_name, num_copies):
        self.base_name = base_name
        self.num_copies = num_copies
        self.load_times = []

@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["Simple"])
class TestBenchmarkAssetLoads(object):
    @pytest.fixture
    def launcher_instance(self, request, workspace, level):
        self.launcher = launcher_helper.create_launcher(workspace)

    @pytest.fixture
    def remote_console_instance(self, request):
        console = remote_console_commands.RemoteConsole()

        def teardown():
            if console.connected:
                console.stop()

        request.addfinalizer(teardown)

        return console

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project):
        def teardown():
            # Clean up in the following order:
            #  1) Kill the interactive AP
            #  2) Remove our source .benchmarksettings files that we put in the project
            #  3) Run APBatch to process the missing source files to clean up our generated cached files
            # By going in this order, we guarantee that we won't hit any file locks and we can
            # easily detect when AP is done cleaning everything up for us.
            workspace.asset_processor.stop()
            logger.debug('Cleaning up the benchmark asset artifacts.')
            if self.remove_benchmark_assets(self.get_temp_benchmark_asset_directory(workspace)):
                self.run_ap_batch(workspace.asset_processor)

        # On setup, make sure to clean up any benchmark assets that stuck around from a previous run.
        request.addfinalizer(teardown)
        logger.debug('Cleaning up any previously-existing benchmark asset artifacts.')
        if self.remove_benchmark_assets(self.get_temp_benchmark_asset_directory(workspace)):
            self.run_ap_batch(workspace.asset_processor)

    def run_ap_batch(self, asset_processor):
        """ 
        Run AssetProcessorBatch to process any changed assets.
        
        :param asset_processor: The asset processor object 
        """
        process_assets_success = asset_processor.batch_process(timeout=600, fastscan=True)
        assert process_assets_success, 'Assets did not process correctly'


    def get_temp_benchmark_asset_directory(self, workspace):
        return os.path.join(workspace.paths.engine_root(), workspace.project, 'benchmark')

    def create_benchmark_assets(self, dest_directory, benchmarks):
        """ 
        Create a directory of benchmarksettings assets in the selected project.

        :param dest_direcotry: The path of the temporary directory to create the benchmark assets in
        :param benchmarks: The list of Benchmark objects to process and create
        """
        source_directory = os.path.join(os.path.dirname(__file__), 'assets')
        logger.debug(f'Creating directory: {dest_directory}')
        os.makedirs(dest_directory, exist_ok=True)
        
        # For each Benchmark object, either just copy the .benchmarksettings file as-is with the same name
        # if we only want one copy, or else append _<num> to the name for each copy if we want multiples.
        for benchmark in benchmarks:
            if benchmark.num_copies > 1:
                for asset_num in range (0, benchmark.num_copies):
                    shutil.copy(os.path.join(source_directory, f'{benchmark.base_name}.benchmarksettings'), 
                                os.path.join(dest_directory, f'{benchmark.base_name}_{asset_num}.benchmarksettings'))
            else:
                shutil.copy(os.path.join(source_directory, f'{benchmark.base_name}.benchmarksettings'), dest_directory)


    def remove_benchmark_assets(self, dest_directory):
        """ 
        Remove the project/benchmark directory that we created.

        :param dest_directory: The path to the temp directory that we created the source benchmarks assets in
        :return True if assets were found and removed, False if they weren't
        """
        return delete_check(dest_directory)
        

    def set_cvar(self, remote_console_instance, cvar_name, cvar_value):
        """
        Helper function to set a CVar value and wait for confirmation.
        """        
        send_command_and_expect_response(remote_console_instance, f"{cvar_name} {cvar_value}", 
                                         f"{cvar_name} : {cvar_value}", timeout=remote_command_timeout)


    def run_benchmark(self, remote_console_instance, num_trials, benchmark):
        """
        Run a series of benchmark trials.

        :param remote_console_instance: The remote console instance
        :param num_trials: The number of times to run the same benchmark in a row
        :param benchmark: The Benchmark object describing the set of assets to try loading
        """
        # The root benchmark asset is the one with sub ID 1, so it has '_00000001' at the end of the base name.
        name_suffix='_00000001.benchmark'
    
        for trial in range (0, num_trials):
            # Set a unique log label to make it easier to parse for the results
            self.set_cvar(remote_console_instance, 'benchmarkLoadAssetLogLabel', f"{benchmark.base_name}_{trial}")

            # Clear the list of assets to load for the benchmark
            send_command_and_expect_response(remote_console_instance, 
                f"benchmarkClearAssetList", 
                f"({benchmark.base_name}_{trial}) - Benchmark asset list cleared.", timeout=remote_command_timeout)

            # Build up the load asset command.
            # Note that there is a limit to the number of parameters we can send in each console command, so we need to split it up
            # if we have too many assets.
            max_params_per_line = 50
            if benchmark.num_copies > 1:
                benchmark_command = f"benchmarkAddAssetsToList"
                for asset_num in range (0, benchmark.num_copies):
                    benchmark_command += f" benchmark/{benchmark.base_name}_{asset_num}{name_suffix}"
                    if (asset_num % max_params_per_line == (max_params_per_line - 1)) or (asset_num == benchmark.num_copies - 1):
                        send_command_and_expect_response(remote_console_instance, 
                            benchmark_command, 
                            f"({benchmark.base_name}_{trial}) - All requested assets added.", timeout=remote_command_timeout)
                        benchmark_command = f"benchmarkAddAssetsToList"
            else:
                send_command_and_expect_response(remote_console_instance, 
                    f"benchmarkAddAssetsToList benchmark/{benchmark.base_name}{name_suffix}", 
                    f"({benchmark.base_name}_{trial}) - All requested assets added.", timeout=remote_command_timeout)

            # Attempt to clear the Windows file cache between each run
            self.clear_windows_file_cache()

            # Send it and wait for the benchmark to complete
            send_command_and_expect_response(remote_console_instance, 
                f"benchmarkLoadAssetList", 
                f"({benchmark.base_name}_{trial}) - Benchmark run complete.", timeout=remote_command_timeout)

    def move_artifact(self, artifact_manager, artifact_name):
        """ 
        Move the artifact to our TestResults.
        """
        # Save the artifact to be examined later
        logger.debug('Preserving artifact: {}'.format(artifact_name))
        artifact_manager.save_artifact(artifact_name)
        os.remove(artifact_name)

    def extract_benchmark_results(self, launcher_log, extracted_log, benchmarks):
        """
        Extract the benchmark timing results, save them in a summary file, and put the timings into our benchmarks structure.

        :param launcher_log: the path to the game.log to parse
        :param extracted_log: the output path to save the summarized log
        :param benchmarks: the benchmarks structure to fill in with our timing results
        """
        logger.debug('Parsing benchmark results.')
        with open(extracted_log, 'w') as summary_log:
            with open(launcher_log) as log:
                for line in log:
                    for benchmark in benchmarks:
                        for trial in range (0, benchmark_trials):
                            if f'({benchmark.base_name}_{trial}) - Results:' in line:
                                # For each timing result we find, write it as-is into our summary log.
                                summary_log.write(line)
                                # Pull out the timing value and save it in the appropriate benchmark entry.
                                match = re.search("Time=([\\d]+)", line)
                                if match:
                                    benchmark.load_times.append(int(match.groups()[0]))

    def clear_windows_file_cache(self):
        """
        Clear the Windows file cache, using the undocumented NtSetSystemInformation call.
        For this to work, the process needs to be run with Administrator privileges.
        :return True if file cache cleared, False if not.
        """

        # Define the LUID Windows class
        class LUID(ctypes.Structure):
            _fields_ = [
                ('low_part', c_uint32),
                ('high_part', c_int32),
                ]

        # Define an abbreviated version of the TOKEN_PRIVILEGES Windows class.
        # The correct version would use an array of dynamic size for LUID_AND_ATTRIBUTES inside of itself,
        # but since we're only every using one entry, we can simplify the code and hard-code the definition at
        # a single entry.
        class TOKEN_PRIVILEGES(ctypes.Structure):
            _fields_ = [
                ('count', c_uint32),
                ('luid', LUID),
                ('attributes', c_uint32),
                ]

            # Force the construction to always fill in the array count with 1, since we've hard-coded a single
            # array definition into our structure definition.
            def __init__(self, luid, attributes):
                super(TOKEN_PRIVILEGES, self).__init__(1, luid, attributes)

        # Windows constants that we'll need
        TOKEN_QUERY = 0x0008
        TOKEN_ADJUST_PRIVILEGES = 0x0020
        SE_PRIVILEGE_ENABLED = 0x0002
        STATUS_PRIVILEGE_NOT_HELD = 0xc0000061
        ERROR_NOT_ALL_ASSIGNED = 1300
        SystemMemoryListInformation = 0x0050
        MemoryPurgeStandbyList = 4

        logger.info('Attempting to clear the Windows file cache')

        # Get the token for the currently-running process
        token_handle = HANDLE()
        result = windll.advapi32.OpenProcessToken(
            windll.kernel32.GetCurrentProcess(), 
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, 
            byref(token_handle))
        if (result == 0):
            logger.error('Failed on the OpenProcessToken call')

        # Get the LUID for the privilege we need to escalate
        luid = LUID()
        if (result > 0):
            result = windll.advapi32.LookupPrivilegeValueW(None, 'SeProfileSingleProcessPrivilege', byref(luid))
            if (result == 0):
                logger.error('Failed on the LookupPrivilegeValueW call')

        # Adjust the privilege for the current process so that we have access to clear the Windows file cache.
        # This will only succeed if we're running with Administrator privileges.
        if (result > 0):
            tp = TOKEN_PRIVILEGES(luid, SE_PRIVILEGE_ENABLED)
            result = windll.advapi32.AdjustTokenPrivileges(token_handle, False, byref(tp), 0, None, None)
            if (result > 0):
                # The AdjustTokenPrivileges call itself succeeded, but we need to separately check to see if the privilege was escalated
                errCode = windll.kernel32.GetLastError()
                if (errCode == ERROR_NOT_ALL_ASSIGNED):
                    # This is an acceptable error, but it means we won't be able to clear the file cache.
                    # Don't fail the benchmark run due to it though.
                    windll.kernel32.CloseHandle(token_handle)
                    logger.info('Windows file cache will not be cleared, insufficient privileges.')
                    logger.info('If you wish to clear the cache when running this benchmark, run with Administrator privileges.')
                    return False
                elif (errCode > 0):
                    # Flag that something unexpected happened.
                    result = 0
            if (result == 0):
                logger.error('Failed on the AdjustTokenPrivileges call')

        # Privileges have been escalated, so clear the Windows file cache.
        if (result > 0):
            command = c_uint32(MemoryPurgeStandbyList)
            status = c_int32(windll.ntdll.NtSetSystemInformation(SystemMemoryListInformation, byref(command), sizeof(command) ))
            if (status.value == c_int32(STATUS_PRIVILEGE_NOT_HELD).value):
                # This is an acceptable error, but it means we won't be able to clear the file cache.
                # Don't fail the benchmark run due to it though.
                windll.kernel32.CloseHandle(token_handle)
                logger.info('Windows file cache will not be cleared, insufficient privileges.')
                logger.info('If you wish to clear the cache when running this benchmark, run with Administrator privileges.')
                return False
            elif (status.value < 0):
                # Flag that something unexpected happened.
                result = 0
                logger.error('Failed on the NtSetSystemInformation call')

        # Print the final results
        if (result > 0):
            logger.info('Successfully cleared the Windows file cache.')
        else:
            errCode = windll.kernel32.GetLastError()
            logger.error(f'Unexpected error while attempting to clear the Windows file cache: {errCode}')

        windll.kernel32.CloseHandle(token_handle)
        return (result > 0)


    def base_test_BenchmarkAssetLoads(self, workspace, launcher, level, remote_console_instance, use_dev_mode):
        benchmarks = [
            # The choice of 650 MB as the benchmark size is arbitrary.  It was picked as large enough to get reasonably stable
            # and measureable results even when running on an SSD, and because the "balanced tree" test causes a total size
            # of (2^N + 1)*AssetSize.  Working with that constraint, and a base asset size of 10 MB, the main contenders were 
            # either 330 MB, 650 MB, or 1290 MB.

            # The first set of benchmarks measures the load time effects of different asset dependency hierarchies.

            # Load 650 MB from a single root 10MB asset where each asset has 2 dependent 10MB assets 6 levels deep
            Benchmark('10mb_2x6', 1),
            # Load 650 MB from a single root 10MB asset where each asset has 4 dependent 10MB assets 3 levels deep
            Benchmark('10mb_4x3', 1),
            # Load 650 MB from a single root 10MB asset that has 64 dependent 10MB assets
            Benchmark('10mb_64x1', 1),
            # Load 650 MB from a single root 10MB asset where each asset has 1 dependent 10MB asset 64 levels deep
            # (Currently removed because it crashes Open 3D Engine, re-enable once LY can handle it - SPEC-1314)
            #Benchmark('10mb_1x64', 1),

            # The second set of benchmarks measures the load time effects of different quantities of parallel asset loads.

            # Load a single 650 MB asset
            Benchmark('650mb', 1),
            # Load 2 x 325 MB assets in parallel
            Benchmark('325mb', 2),
            # Load 3 x 216 MB assets in parallel
            Benchmark('216mb', 3),
            # Load 4 x 162 MB assets in parallel
            Benchmark('162mb', 4),
            # Load 65 x 10 MB assets in parallel
            Benchmark('10mb', 65),
            # Load 650 x 1 MB assets in parallel
            Benchmark('1mb', 650),
            ]

        # Create test assets
        logger.debug('Generating new benchmark asset artifacts.')
        self.create_benchmark_assets(self.get_temp_benchmark_asset_directory(workspace), benchmarks)
        self.run_ap_batch(workspace.asset_processor)

        # Set the launcher arguments to choose either dev mode or optimized mode for the streamer.
        if (use_dev_mode):
            launcher.args = ['-NullRenderer', '-cl_streamerDevMode=true']
        else:
            launcher.args = ['-NullRenderer', '-cl_streamerDevMode=false']
        streamer_mode = 'devMode' if use_dev_mode else 'optMode'

        # Run the benchmarks in the launcher
        logger.debug(f'Running benchmark in launcher with args={launcher.args}')
        with launcher.start():
            waiter.wait_for(
                lambda: check_for_listening_port(remote_console_port),
                listener_timeout,
                exc=AssertionError("Port {} not listening.".format(remote_console_port)),
            )
            remote_console_instance.start(timeout=remote_command_timeout)

            # Set up the benchmarking parameters to use across all the tests
            self.set_cvar(remote_console_instance, "benchmarkLoadAssetTimeoutMs", "30000")
            self.set_cvar(remote_console_instance, "benchmarkLoadAssetPollMs", "20")
            self.set_cvar(remote_console_instance, "benchmarkLoadAssetDisplayProgress", "false")
            self.set_cvar(remote_console_instance, "cl_assetLoadWarningEnable", "false")

            # Run all our benchmarks
            for benchmark in benchmarks:
                self.run_benchmark(remote_console_instance, benchmark_trials, benchmark)

            # Give the launcher a chance to quit before forcibly stopping it.  
            # This way our game.log will contain all the information we want instead of potentially truncating early.
            send_command_and_expect_response(remote_console_instance, f"quit", "Executing console command 'quit'", 
                                                timeout=remote_command_timeout)
            launcher.stop()
            remote_console_instance.stop()

        # Extract all the benchmark timings out of the game.log, save it into a summary log file,
        # and put the timings into the benchmarks list so we can gather summarized metrics
        launcher_log = os.path.join(launcher.workspace.paths.project_log(), 'Game.log')
        summary_log = f'load_benchmark_{level}_{streamer_mode}.log'
        self.extract_benchmark_results(launcher_log, summary_log, benchmarks)
        self.move_artifact(workspace.artifact_manager, summary_log)

        # Generate a CSV of the min/max/avg results for each test.
        results_csv = f'load_benchmark_{level}_{streamer_mode}.csv'
        with open(results_csv, 'w') as csv_file:
            csv_file.write(f'"Test Name","Num Assets Loaded","First Time(ms)","Last Time(ms)","Max Time(ms)","Min Time (ms)",'+
                            f'"Avg Time (ms)","Median Time (ms)"\n')
            for benchmark in benchmarks:
                csv_file.write(f'{benchmark.base_name},{benchmark.num_copies},{benchmark.load_times[0]},{benchmark.load_times[-1]},'+
                                f'{max(benchmark.load_times)},{min(benchmark.load_times)},{mean(benchmark.load_times)},'+
                                f'{median(benchmark.load_times)}\n')
        self.move_artifact(workspace.artifact_manager, results_csv)

    @pytest.mark.SUITE_benchmark
    def test_BenchmarkAssetLoads_optMode(self, workspace, launcher, level, remote_console_instance):
        self.base_test_BenchmarkAssetLoads(workspace, launcher, level, remote_console_instance, use_dev_mode = False)

    @pytest.mark.SUITE_benchmark
    def test_BenchmarkAssetLoads_devMode(self, workspace, launcher, level, remote_console_instance):
        self.base_test_BenchmarkAssetLoads(workspace, launcher, level, remote_console_instance, use_dev_mode = True)
