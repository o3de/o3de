"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from argparse import ArgumentParser
from xml.etree import ElementTree
import json
import os
import platform
import re
import sys


__version__ = '0.1.0'
__copyright__ = 'Copyright (c) Contributors to the Open 3D Engine Project.\nFor complete copyright and license terms please see the LICENSE at the root of this distribution.'

wwise_versions = dict()
project_platforms = dict()


class WwiseVersionInfo:
    """
    Defines information about a singular Wwise installation.
    """
    def __init__(self, version):
        self.cli_tool = ''
        self.exe_tool = ''
        self.display_name = ''
        self.install_dir = ''
        self.version_name = version
        self.is_ltx = False
        self.is_new_cli = False     # For different invoke syntax

    def __str__(self):
        return f'{self.display_name}\n' \
               f'  LTX: {self.is_ltx}\n' \
               f'  CLI: {self.cli_tool}\n' \
               f'  EXE: {self.exe_tool}\n'


class PlatformInfo:
    """
    Defines information about a platform as defined by the Wwise project.
    """
    def __init__(self, name, reference):
        self.name = name
        self.ref_name = reference
        self.dir_name = ''
        self.bnk_path = ''
        self.ext_path = ''
        self.pre_gen = ''
        self.post_gen = ''

    def __str__(self):
        return f'Platform {self.name}\n' \
               f'  BankPath: {self.bnk_path}\n' \
               f'  ExtSrcPath: {self.ext_path}\n' \
               f'  Pre-Gen: {self.pre_gen}\n' \
               f'  Post-Gen: {self.post_gen}'


def get_options():
    """
    Define and parse program arguments, do some error checking, then return the program options.
    :return: The program options
    """
    parser = ArgumentParser()
    parser.add_argument('--project_path', '-p', required=True,
                        help='Path to LY project to set up')
    options = parser.parse_args()

    wwise_install_table_file = os.path.join(os.environ['ProgramFiles(x86)'],
                                            'Audiokinetic', 'Data', 'install-table.json')

    if not os.path.exists(wwise_install_table_file):
        sys.exit(f'[Error] {wwise_install_table_file}: Not Found')

    if not os.path.exists(options.project_path):
        sys.exit(f'[Error] {options.project_path}: Not Found')

    wwise_project_path = os.path.join(options.project_path, 'Sounds', 'wwise_project')
    if not os.path.exists(wwise_project_path):
        sys.exit(f'[Error] {wwise_project_path}: Not Found')

    project_files = [f for f in os.listdir(wwise_project_path) if f.endswith('.wproj')]
    if len(project_files) != 1:
        sys.exit('[Error] Wwise Project File: More Than One Found')

    # Add the install table file to the options
    options.table_file = wwise_install_table_file
    options.wwise_project = os.path.join(wwise_project_path, project_files[0])
    return options


def parse_wwise_project(wwise_project):
    """
    Parse the Wwise project xml file (*.wproj) and extract information from it.
    :param wwise_project: Path to the Wwise project file.
    :return: The version of the Wwise project.
    """
    wwise_doc = ElementTree.parse(wwise_project)
    root = wwise_doc.getroot()

    # There is no known 'LTX' designation contained in the Wwise Project xml.
    # But we can count on the existence (or lack thereof) of certain features/folders/etc to figure
    # out if this project is Wwise LTX or not.
    music_folder = root.find('./ProjectInfo/Project/PhysicalFolderList/PhysicalFolder[@Path="Interactive Music Hierarchy"]')
    if music_folder is None:
        sys.exit('[Error] Wwise LTX Project: Project Needs Upgrade')

    project_version = root.get('WwiseVersion')

    # Get the platforms...
    global project_platforms    # Modify the dict
    platforms = root.findall('./ProjectInfo/Project/Platforms/Platform')
    for p in platforms:
        name = p.get('Name')
        reference = p.get('ReferencePlatform')
        project_platforms[name] = PlatformInfo(name, reference)

    # Parse platform-specific properties...
    property_list = root.find('./ProjectInfo/Project/PropertyList')
    if property_list is not None:
        def fetch_property_for_platforms(property_name, attr):
            """
            Finds properties in Xml and assigns them to PlatformInfo attributes.
            :param property_name: The name of the Wwise property (as an Xml attribute) to find
            :param attr: The name of the PlatformInfo attribute to assign the property value to
            """
            child = property_list.find(f'./Property[@Name="{property_name}"]')
            if child is not None:
                values = child.findall('./ValueList/Value')
                for val in values:
                    if val.get('Platform'):
                        setattr(project_platforms[val.get('Platform')], attr, val.text)

        fetch_property_for_platforms('ExternalSourcesOutputPath', 'ext_path')
        fetch_property_for_platforms('SoundBankPaths', 'bnk_path')
        fetch_property_for_platforms('SoundBankPreGenerateCustomCmdLines', 'pre_gen')
        fetch_property_for_platforms('SoundBankPostGenerateCustomCmdLines', 'post_gen')

    return project_version


def parse_json_file(json_file):
    """
    Parse a JSON file and return the parsed contents.
    :json_file: Path to the JSON file.
    :return: Python object parsed from the file contents.
    """
    with open(json_file) as file:
        try:
            json_data = json.load(file)
        except json.JSONDecodeError as decode_error:
            sys.exit(f'[Error] JSON Parse Error\nFile: {json_file}\nLine: {decode_error.lineno}')

    return json_data


def parse_wwise_versions(install_table):
    """
    Inspects the install table data and store information about installed versions of Wwise.
    :param install_table: Install table as parsed Json data.
    """
    global wwise_versions       # Modify the dict

    for key, version_data in install_table.items():
        version_name = key.replace('wwise.', '')
        version_name = version_name.replace('_', '.')
        info = WwiseVersionInfo(version_name)
        info.install_dir = version_data['targetDir']
        info.display_name = version_data['bundle']['displayName']
        info.is_ltx = 'LTX' in version_data['bundle']['version']['nickname']

        # Wwise Authoring EXE
        info.exe_tool = os.path.join(info.install_dir,
                                     'Authoring', 'x64', 'Release', 'bin',
                                     'Wwise.exe')

        # CLI tool names, order matters here!
        # 'WwiseConsole' is newer and should be checked first.
        # During the transition from 'WwiseCLI' to 'WwiseConsole'
        # some versions of Wwise contain both.
        tool_base_names = ['WwiseConsole', 'WwiseCLI']
        for tool in tool_base_names:
            tool_path = os.path.join(info.install_dir,
                                     'Authoring', 'x64', 'Release', 'bin',
                                     tool + '.exe')
            if os.path.exists(tool_path):
                info.is_new_cli = tool is 'WwiseConsole'
                info.cli_tool = tool_path
                break

        wwise_versions[version_name] = info


def get_matching_version_key(project_version):
    """
    Given a version string, match against all known installed versions of Wwise and return the version key.
    :param project_version: Wwise project version string.
    :return: A version key that closely matches the input version.
    """
    installed_versions = [key for key in wwise_versions.keys() if 'ltx' not in key]
    version_key = None
    for ver in installed_versions:
        if project_version[1:] in ver:
            version_key = ver
            break

    return version_key


def generate_game_wwise_config(project_path):
    """
    Create a JSON object and update it according to Platform information already obtained.
    Write this object to the final output destination.
    :param project_path: Path to the game project.
    """
    current_dir = os.path.dirname(os.path.realpath(__file__))

    # Get the Platform sub-path relative to the directory where *this script* file resides...
    platforms_path = os.path.join(current_dir, 'Platform')
    if not os.path.exists(platforms_path):
        sys.exit(f'[Error] {platforms_path}: Not Found')

    platform_folders = list(filter(lambda d: os.path.isdir(os.path.join(platforms_path, d)), os.listdir(platforms_path)))

    # Create the json object to hold the data...
    json_data = {'platformMaps': []}
    # Add in data for all platforms to the platformMaps array...
    for p in platform_folders:
        file = os.path.join(platforms_path, p, f'wwise_config_{p.lower()}.json')
        file_data = parse_json_file(file)
        json_data['platformMaps'].append(file_data)
        print(f'Added {p} data')

    # now attempt to find the restricted platforms
    engine_root = os.path.normpath(os.path.join(current_dir, '../../../..'))
    restricted_root = os.path.join(engine_root, 'restricted')
    if os.path.exists(restricted_root):
        path_to = os.path.relpath(current_dir, engine_root)
        for restricted_platform in os.listdir(restricted_root):
            resticted_path = os.path.join(restricted_root, restricted_platform, path_to)
            if os.path.exists(resticted_path): 
                file = os.path.join(resticted_path, f'wwise_config_{restricted_platform.lower()}.json')
                file_data = parse_json_file(file)
                json_data['platformMaps'].append(file_data)
                print(f'Added {restricted_platform} data')

    # Update the data with Wwise bank paths that have been found...
    for info in project_platforms.values():
        # Only need to update for platforms that have dir_name set...
        if info.dir_name:
            for pm in json_data['platformMaps']:
                if pm['wwisePlatform'].lower() == info.ref_name.lower():
                    if pm['bankSubPath'].lower != info.dir_name.lower():
                        pm['bankSubPath'] = info.dir_name.lower()

    # Update the real config file in the project assets hierarchy...
    config_file = os.path.join(project_path, 'Sounds', 'wwise', 'wwise_config.json')
    try:
        with open(config_file, 'w') as file:
            json.dump(json_data, file, indent=4)
    except OSError as e:
        sys.exit(f'[OSError] {e.strerror}\n'
                 f'Failed to write output file {config_file}\n'
                 f'Check permissions and try again.')

    print(f'Generated {config_file} successfully!')


def check_path_consistency():
    """
    Checks the sound bank paths that were obtained from the Wwise Project for consistency.
    """
    bnk_path_error = "Sound Bank paths should be set to '..\\wwise\\<Platform>\\'\n" \
                     "        (e.g. Windows is set to '..\\wwise\\Windows\\')\n" \
                     "  1. Open the project in Wwise, then click Project->Project Settings...\n" \
                     "  2. Select the SoundBanks tab.\n" \
                     "  3. Edit the per-platform paths in the 'SoundBank Paths' panel.\n"

    ext_path_error = "External Sources paths should be set to '..\\wwise\\<Platform>\\external\\'\n" \
                     "        (e.g. Windows is set to '..\\wwise\\Windows\\external\\')\n" \
                     "It is okay keep defaults if this Wwise project doesn't use External Sources.\n" \
                     "  1. Open the project in Wwise, then click Project->Project Settings...\n" \
                     "  2. Select the External Sources tab.\n" \
                     "  3. Edit the per-platform paths in the 'Output Path' panel.\n"

    bnk_re = re.compile(r'^\.\.[\\/]wwise[\\/](\w+)[\\/]?$', re.I)
    ext_re = re.compile(r'^\.\.[\\/]wwise[\\/](\w+)[\\/]external[\\/]?$', re.I)
    global project_platforms    # Modify the dict

    for p in project_platforms.values():
        bnk_m = bnk_re.match(p.bnk_path)
        ext_m = ext_re.match(p.ext_path)

        # Check no match on the Sound Bank path...
        if not bnk_m:
            sys.exit(f'[Error] {bnk_path_error}')

        # Check the default case for External Sources path...
        # That is, the path hasn't been changed from default.
        if p.ext_path and not ext_m:
            if p.ext_path.startswith('GeneratedSoundBanks'):
                print(f'[Warn] {ext_path_error}')
            else:
                sys.exit(f'[Error] {ext_path_error}')

        if bnk_m.group(1) != ext_m.group(1):
            sys.exit(f'[Error] {ext_path_error}')

        p.dir_name = bnk_m.group(1).lower()


def run_wwise_setup():
    """
    Program entry point.
    """
    print(f'Wwise Configuration Setup Tool v{__version__}')
    print(__copyright__)
    print()

    if platform.system() is not 'Windows':
        sys.exit(f'[Error] {platform.system()} is not supported.  Please run this script from a Windows OS.')

    options = get_options()
    options.project_version = parse_wwise_project(options.wwise_project)
    check_path_consistency()

    install_table = parse_json_file(options.table_file)
    parse_wwise_versions(install_table)

    print(f'Wwise Project: {options.wwise_project}')
    print(f'Wwise Project Version: {options.project_version}')

    version_key = get_matching_version_key(options.project_version)
    if not version_key:
        sys.exit(f'[Error] Wwise Project Version {options.project_version}: No Matching Installed Version Found')

    print('\nThe platforms found in the Wwise project are:')
    for p in project_platforms.values():
        if p.name != p.ref_name:
            print(f'  {p.name} ({p.ref_name})')
        else:
            print(f'  {p.name}')

    print('\nIs this correct?')
    print('(Y) Proceed to config file generator')
    print('(N) Abort (instructions will be given)')
    answer = input('? ')

    if answer.lower() not in ['y', 'yes']:
        print('\nEditing Wwise platforms cannot be automated by script.')
        print('Manually update platforms in the Wwise project and try again.')
        print('  1. Open the project in Wwise, then click Project->Platform Manager...')
        print('  2. Add/Remove/Rename platforms as needed then confirm the changes.')
        print('  3. After the project reloads, click Project->Project Settings... and select the SoundBanks tab.')
        print('  4. Check that your SoundBank Folders are set correctly to ..\\wwise\\<Platform>\\ for each platform.')
        print('       (e.g. Windows is set to ..\\wwise\\Windows\\ and so on)')
        print('  5. Save the project.')
        return

    print('\nProceeding...')
    generate_game_wwise_config(options.project_path)


if __name__ == '__main__':
    run_wwise_setup()
