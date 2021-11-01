#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
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

    def __init__(self, config_file=None):
        self.config_file = config_file
        self.config_data = self._load_config()
        self.file_regex = self._load_file_regex()

    def _load_config(self):
        """Load config from the provided file. Sets default file if one is not provided."""
        if self.config_file is None:
            script_directory = os.path.dirname(os.path.abspath(__file__))  # Default file expected in same dir as script
            self.config_file = os.path.join(script_directory, self.DEFAULT_CONFIG_FILE)

        try:
            with open(self.config_file) as f:
                return json.load(f)
        except FileNotFoundError:
            print('Config file cannot be found')
            raise

    def _load_file_regex(self):
        """Returns regex object with case-insensitive matching from the list of filename patterns."""
        regex_patterns = []
        if 'license_patterns' in self.config_data:
            for pattern in self.config_data['license_patterns']:
                regex_patterns.append(fnmatch.translate(pattern))
        if 'package_patterns' in self.config_data:
            for pattern in self.config_data['package_patterns']:
                regex_patterns.append(fnmatch.translate(pattern))
        else:
            print('Invalid key values found in config file! Use "license_patterns" for licenses, "package_patterns" for 3p packages')
            raise KeyError

        return re.compile('|'.join(regex_patterns), re.IGNORECASE)

    def scan(self, path=os.curdir):
        """Scan directory tree for filenames matching file_regex.

        :param path: Path of the directory to run scanner
        :return: Package paths and their corresponding file contents
        :rtype: dict
        """
        files = 0
        matching_files = {}

        for dirpath, dirnames, filenames in os.walk(path):
            for file in filenames:
                if self.file_regex.match(file):
                    matching_file_content = self._get_file_contents(os.path.join(dirpath, file))
                    rel_dirpath = os.path.relpath(dirpath, path)  # Limit path inside scanned directory
                    matching_files[rel_dirpath] = matching_file_content
                    files += 1
                    print(f'Matching file: {os.path.join(dirpath, file)}')

            # Remove directories that should not be scanned
            for dir in self.config_data['excluded_directories']:
                if dir in dirnames:
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
                license_output = '\n\n'.join([
                    f'{license_separator}',
                    f'Package path: {directory}',
                    'License:',
                    f'{license}\n'
                ])
                lf.write(license_output)
        return None
    
    def create_package_file(self, packages, filepath='SPDX-Licenses.json', dirpath=None):
        """Creates file with all the provided SPDX package info summaries.

        :param licenses: Dict with package paths and their corresponding license file contents
        :param filepath: Path to write the file
        :param dirpath: Root path for packages
        :rtype: dict
        """
        licenses = {}
        package_separator = ','
        with open(filepath, 'w', encoding='utf8') as pf:
            for directory, package in packages.items():
                package_obj = json.loads(package)
                package_json = json.dumps(package_obj, indent=4)
                package_output = ''.join([
                    f'{package_json}',
                    f'{package_separator}\n'
                ])
                pf.write(package_output)
                if dirpath:
                    license_path = os.path.join(directory, pathlib.Path(package_obj['LicenseFile'])) # Gets relative path
                    license_content = self._get_file_contents(os.path.join(dirpath, license_path))
                    licenses[license_path] = license_content
        return licenses


def parse_args():
    parser = argparse.ArgumentParser(
        description='Script to run LicenseScanner and generate license file')
    parser.add_argument('--config-file', '-c', type=pathlib.Path, help='Config file for LicenseScanner')
    parser.add_argument('--license-file-path', '-l', type=pathlib.Path, help='Create license file in the provided path')
    parser.add_argument('--package-file-path', '-p', type=pathlib.Path, help='Create package summary file in the provided path')
    parser.add_argument('--scan-path', '-s', default=os.curdir, type=pathlib.Path, help='Path to scan')
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
            license_files = ls.create_package_file(scanned_path_data, args.package_file_path, args.scan_path)
            ls.create_license_file(license_files, args.license_file_path)
    except FileNotFoundError as e:
        print(f'Type: {type(e).__name__}, Error: {e}')
        return 1


if __name__ == '__main__':
    sys.exit(main())
