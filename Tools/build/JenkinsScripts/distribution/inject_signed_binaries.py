"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

Description: 
    Release automation script that injects signed binary files into package zips 
    then generates new MD5 checksum files for the modified packages. 
"""
import argparse
import os
import subprocess
import sys

from AWS_PyTools.LyChecksum import getMD5ChecksumForSingleFile
from Installer.InstallerAutomation import defaultFilesToSign
from Installer.SignTool import signtoolVerifySign

DEFAULT_FILES_TO_INJECT = defaultFilesToSign
DEFAULT_PLATFORMS = ['pc', 'provo', 'consoles']
DEFAULT_WORKING_DIR = '%TEMP%/installerAuto'
DEFAULT_PATH_7ZIP = 'C:/7z.exe'

DEFAULT_VERSION = os.environ.get('VERSION')
DEFAULT_P4_CL = os.environ.get('P4_CL')
DEFAULT_BUILD_NUMBER = os.environ.get('BUILD_NUMBER')


class InjectionError(Exception): pass


def create_md5_file(file_path, checksum):
    """Write the provided checksum to an .MD5 file and return the file path."""
    md5_file = '{}.MD5'.format(file_path)
    with open(md5_file, 'w') as f:
        f.write(checksum)
    return md5_file


def format_package_name(package_version, changelist, platform, build_number):
    """Return a string of the package name in the standard format"""
    package_name = 'lumberyard-{0}-{1}-{2}-{3}.zip'.format(package_version, changelist, platform, build_number)
    return package_name


def verify_signed_files(binary_files, working_dir, verbose):
    """Verifies that the binary files in the workspace are signed

    Function imported from Installer/SignTool.py to verify binary files.
    Returns True if file is signed.

    Returns:
        The list of relative paths for the verified signed files

    """
    signed_binary_files = []
    for b in binary_files:
        binary_file_path = os.path.join(working_dir, b)
        signed = signtoolVerifySign(binary_file_path, verbose)
        if not signed:
            raise InjectionError('Unsigned binary file found in workspace: {0}'.format(b))
        signed_binary_files.append(b)
    return signed_binary_files


def print_result(description, list):
    """Prints the result with formatting"""
    list_new_line = '\n'.join(map(str, list))
    result = '\n'.join([description, list_new_line])
    print(result)


def generate_md5_checksums(updated_zips):
    """Using a list of file paths, generate MD5 checksums.

    Function is imported from AWS_PyTools/LyChecksum.py to generate checksum 
    A file is then created for each checksum.

    Returns:
        The list of paths for the generated .MD5 files.

    """
    md5_files = []
    for z in updated_zips:
        checksum = getMD5ChecksumForSingleFile(z).hexdigest()
        md5_file = create_md5_file(z, checksum)
        md5_files.append(md5_file)
    return md5_files


def inject_binaries(args):
    """Inject signed binary files into package zips.

    Verify that the binary files are signed.
    Write the signed binary list to a file seprated by new lines to supply to 7-Zip.

    Run command to inject signed binary into package zips.
    Command line syntax: 7z.exe a -spf2 <zip_file_path> @<file_list>

    Returns:
        The list of file paths for the updated package zips. 

    """
    signed_binary_files = verify_signed_files(args.files_to_inject, args.working_dir, args.verbose)

    list_file_path = os.path.join(args.working_dir, 'list_file.txt')
    with open(list_file_path, 'w') as list_file:
        list_file.write('\n'.join(signed_binary_files))

    updated_zips = []
    for p in args.platforms:
        package_name = format_package_name(args.package_version, args.changelist, p, args.build_number)
        package_path = os.path.join(args.working_dir, package_name)
        try:
            subprocess.check_call([args.path_7zip, 'a', '-spf2', package_path, '@{0}'.format(list_file_path)], 
                                    cwd=args.working_dir)
            updated_zips.append(package_path)
        except subprocess.CalledProcessError as e:
            raise InjectionError('Error using 7z to inject signed binaries: {0}'.format(e))
    return updated_zips


def parse_args():
    """Setup arguments. Defaults to using build parameters and binary list also used by CODESIGN_Windows."""
    parser = argparse.ArgumentParser(
        description='Inject signed binary files into package zips then generate new MD5 checksums')
    parser.add_argument('-w', '--working-dir', default=DEFAULT_WORKING_DIR, 
        help='Directory where the binary files and package zips are located.')
    parser.add_argument('-p', '--package-version', default=DEFAULT_VERSION, 
        help='<Major>.<Minor> version of the target packages.')
    parser.add_argument('-c', '--changelist', default=DEFAULT_P4_CL, 
        help='Perforce changelist for the target packages')
    parser.add_argument('-b', '--build-number', default=DEFAULT_BUILD_NUMBER, 
        help='Perforce changelist for the target packages')
    parser.add_argument('--files-to-inject', nargs='+', default=DEFAULT_FILES_TO_INJECT, 
        help='List of binaries to inject into the package zips. Defaults to the list used to sign the files.')
    parser.add_argument('--platforms', nargs='+', default=DEFAULT_PLATFORMS, 
        help='Specifies which platform packages to inject.')
    parser.add_argument('--path-7zip', default=DEFAULT_PATH_7ZIP, 
        help='Path for the 7zip executable.')
    parser.add_argument('--verbose', action='store_true', 
        help='Verbose output on codesigning commands.')
    args = parser.parse_args()
    return args


def main():
    try:
        args = parse_args()
        updated_zips = inject_binaries(args)
        md5_files = generate_md5_checksums(updated_zips)

        print_result('Package zips updated with signed binaries:', updated_zips)
        print_result('Generated MD5 checksum files:', md5_files)
    except InjectionError as e:
        raise SystemExit('Injection Error: {}'.format(e))

    sys.exit(0)
    

if __name__ == "__main__":
    main()
