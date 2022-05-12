#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
from collections import OrderedDict
import fnmatch
import json
import os
import pathlib
import re
import sys


class LicenseScanner:
    """Class to contain license scanner.

    Scans source tree for license files using provided filename patterns and generates a file
    with the contents of all the licenses.

    :param config_file: Config file with license patterns and scanner settings
    """

    DEFAULT_CONFIG_FILE = 'scanner_config.json'
    DEFAULT_EXCLUDE_FILE = '.gitignore'
    DEFAULT_PACKAGE_INFO_FILE = 'PackageInfo.json'

    def __init__(self, config_file=None):
        self.config_file = config_file
        self.config_data = self._load_config()
        self.file_regex = self._load_file_regex(self.config_data['license_patterns'])
        self.package_info = self._load_file_regex(self.config_data['package_patterns'])
        self.excluded_directories = self._load_file_regex(self.config_data['excluded_directories'])

    def _load_config(self):
        """Load config from the provided file. Sets default file if one is not provided."""
        if not self.config_file:
            script_directory = os.path.dirname(os.path.abspath(__file__))  # Default file expected in same dir as script
            self.config_file = os.path.join(script_directory, self.DEFAULT_CONFIG_FILE)

        try:
            with open(self.config_file) as f:
                return json.load(f)
        except FileNotFoundError:
            print('Config file cannot be found')
            raise

    def _load_file_regex(self, patterns):
        """Returns regex object with case-insensitive matching from the list of filename patterns."""
        regex_patterns = []
        for pattern in patterns:
            regex_patterns.append(fnmatch.translate(pattern))
        
        if not regex_patterns:
            print(f'Warning: No patterns from {patterns} found')
            return None

        return re.compile('|'.join(regex_patterns), re.IGNORECASE)

    def scan(self, paths=os.curdir):
        """Scan directory tree for filenames matching file_regex, package info, and exclusion files.

        :param paths: Paths of the directory to run scanner
        :return: Package paths and their corresponding file contents
        :rtype: Ordered dict
        """
        files = 0
        matching_files = OrderedDict()
        excluded_directories = None

        if not self.package_info:
            self.package_info = self.DEFAULT_PACKAGE_INFO_FILE

        if not self.excluded_directories:
            print(f'No excluded directory in config, looking for {self.DEFAULT_EXCLUDE_FILE} instead')

        for path in paths:
            for dirpath, dirnames, filenames in os.walk(path, topdown=True):
                dirnames.sort(key=str.casefold) # Ensure that results are sorted
                for file in filenames:
                    if self.file_regex.match(file) or self.package_info.match(file):
                        file_path = os.path.join(dirpath, file)
                        matching_file_content = self._get_file_contents(file_path)
                        matching_files[file_path] = matching_file_content
                        files += 1
                        print(f'Matching file: {file_path}')
                        if self.package_info.match(file):
                            dirnames[:] = [] # Stop scanning subdirectories if package info file found
                    if self.DEFAULT_EXCLUDE_FILE in file and not self.excluded_directories:
                        ignore_list = self._get_file_contents(os.path.join(dirpath, file)).splitlines()
                        ignore_list.append('.git') # .gitignore doesn't usually have .git in its exclusions
                        excluded_directories = self._load_file_regex(ignore_list)

                # Remove directories that should not be scanned
                if self.excluded_directories:
                    excluded_directories = self.excluded_directories
                for dir in dirnames:
                    if excluded_directories.match(dir):
                        dirnames.remove(dir)

        print(f'{files} files found.')
        return matching_files

    def _get_file_contents(self, filepath):
        try:
            with open(filepath, encoding='utf8') as f:
                return f.read()
        except UnicodeDecodeError:
            print(f'Unable to read file: {filepath}')
            pass

    def create_license_file(self, licenses, filepath='NOTICES.txt'):
        """Creates file with all the provided license file contents.

        :param licenses: Dict with package paths and their corresponding license file contents
        :param filepath: Path to write the file
        """
        license_separator = '------------------------------------'
        with open(filepath, 'w', encoding='utf8') as lf:
            for directory, license in licenses.items():
                if not self.package_info.match(os.path.basename(directory)):
                    license_output = '\n\n'.join([
                        f'{license_separator}',
                        f'Package path: {os.path.relpath(directory)}',
                        'License:',
                        f'{license}\n'
                    ])
                lf.write(license_output)
        return None
    
    def create_package_file(self, packages, filepath='SPDX-Licenses.json', get_contents=False):
        """Creates file with all the provided SPDX package info summaries in json.
        Optional dirpath parameter will follow the license file path in the package info and return its contents in a dictionary

        :param licenses: Dict with package info paths and their corresponding file contents
        :param filepath: Path to write the file
        :param dirpath: Root path for packages
        :rtype: Ordered dict
        """
        licenses = OrderedDict()
        package_json = []

        with open(filepath, 'w', encoding='utf8') as pf:
            for directory, package in packages.items():
                if self.package_info.match(os.path.basename(directory)):
                    package_obj = json.loads(package)
                    package_json.append(package_obj)
                    if get_contents:
                        license_path = os.path.join(os.path.dirname(directory), pathlib.Path(package_obj['LicenseFile']))
                        licenses[license_path] = self._get_file_contents(license_path)
                else:
                    licenses[directory] = package
            pf.write(json.dumps(package_json, indent=4))
        return licenses


def parse_args():
    parser = argparse.ArgumentParser(
        description='Script to run LicenseScanner and generate license file')
    parser.add_argument('--config-file', '-c', type=pathlib.Path, help='Config file for LicenseScanner')
    parser.add_argument('--license-file-path', '-l', type=pathlib.Path, help='Create license file in the provided path')
    parser.add_argument('--package-file-path', '-p', type=pathlib.Path, help='Create package summary file in the provided path')
    parser.add_argument('--scan-path', '-s', default=os.curdir, type=pathlib.Path, nargs='+', help='Path to scan, multiple space separated paths can be used')
    return parser.parse_args()


def main():
    try:
        args = parse_args()
        ls = LicenseScanner(args.config_file)
        scanned_path_data = ls.scan(args.scan_path)

        if args.license_file_path:
            ls.create_license_file(scanned_path_data, args.license_file_path)
        if args.package_file_path:
            ls.create_package_file(scanned_path_data, args.package_file_path)
        if args.license_file_path and args.package_file_path:
            license_files = ls.create_package_file(scanned_path_data, args.package_file_path, True)
            ls.create_license_file(license_files, args.license_file_path)
    except FileNotFoundError as e:
        print(f'Type: {type(e).__name__}, Error: {e}')
        return 1


if __name__ == '__main__':
    sys.exit(main())
