# coding:utf-8
# !/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# File Description:
# This file contains Lumberyard specific file and helper functions for handling material conversion
# -------------------------------------------------------------------------


import click
from pathlib import *
import os
import main as app_main

@click.command()
@click.version_option('1.0.0')
@click.argument('src', nargs=1, type=click.Path())
@click.argument('dst', nargs=1, type=click.Path())
@click.option('--material', default=True, help='Generate .material file per material found. Options: [true, false]')
@click.option('--logs', default=True, help='Generate log output. Options: [true, false]')
@click.option('--overwrite', default=False, help='Overwrite existing conversion files. Options: [true, false]')
@click.option('--base', default='None', help='This parameter must be set, as well as a directory name in the source '
                                             'path if performing an in-place conversion. ')
def run(src, dst, material, logs, overwrite, base):
    """
    This is the commmand line interface for the Legacy Asset Converter tool. The purpose of this tool is to convert
    legacy Lumberyard material files (.mtl) and their associated textures to the current O3DE format (.material). The
    current PBR workflow is Metallic/Roughness, so the script will do its best to convert Diffuse, DDNA, and Specular
    maps from the legacy format into BaseColor, Roughness, Normal, and Metallic maps. Additional Maps (i.e. Alpha,
    Emissive, etc.) present in the .mtl files, if compatible with the supported material types will carry over.
    Additional functionality can be found by using the Standalone UI version of the tool.

    # -- Conversion Arguments
    # -- Input Path (file or directory):
    This can be a path to a directory that contains one or more FBX files, or a single FBX file for conversion.

    # -- Output Path (directory)
    The destination directory to send asset files to once converted. If both source and destination paths are to
    the same directory, an in-place conversion will be made. In-place conversions require an additional base directory
    name from which to set relative texture paths in the .material files. This value is set using the "base" option.

    NOTE: If both paths are identical, the conversion will be in-place

    # -- Conversion Options:
    # ---->> Create .material Files ::: Default setting = True
    # ---->> Create Output Logs ::: Default setting = True
    # ---->> Reprocess Existing Files ::: Default setting = False
    """
    options_dictionary = {
        'src': src,
        'dst': dst,
        'material': material,
        'logs': logs,
        'overwrite': overwrite,
        'base': base
    }

    validated, options_dictionary = validate_paths(options_dictionary)
    if validated:
        app_main.run_cli(options_dictionary)


def validate_paths(options_dictionary):
    validated = False
    if is_valid_path(options_dictionary['src']) and is_valid_path(options_dictionary['dst']):
        if get_directory(options_dictionary['src']) == options_dictionary['dst']:
            if options_dictionary['base'] == 'None':
                print('The base option was not set. Please refer to help menu for further information.')
            else:
                p = Path(options_dictionary['src'])
                p = p.parent if p.is_file() else p
                print(f'+++++++++++++ : {p}')
                if not options_dictionary['base'] in p.parts:
                    if [i for i in p.rglob('**/') if i.name == options_dictionary['base']]:
                        validated = True
                    else:
                        print('The base option must be a directory reachable from the specified destination path.')
                else:
                    validated = True
        else:
            validated = True
    return validated, options_dictionary


def get_directory(target_path):
    src = Path(target_path)
    if src.is_dir():
        return target_path
    else:
        if src.suffix.lower() == '.fbx':
            return str(src.parent)
    return 'invalid_path'


def is_valid_path(target_path):
    return True if os.path.exists(target_path) else False


if __name__ == '__main__':
    run()

