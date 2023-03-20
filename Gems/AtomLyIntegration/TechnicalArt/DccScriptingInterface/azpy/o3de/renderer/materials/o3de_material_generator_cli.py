#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
# File Description:
# This file contains Lumberyard specific file and helper functions for handling material conversion
# -------------------------------------------------------------------------


import click
from pathlib import *
import os

# TODO- you are going to need to create a separate ticket to figure out environment variable bootstrapping without
# the use of BAT files


@click.command()
@click.version_option('1.0.0')
@click.argument('src', nargs=1, type=click.Path())
@click.argument('dst', nargs=1, type=click.Path())
@click.argument('dcc', default='Maya', help='Which DCC package to use for conversion. Conversion will fail running '
                                            'this argument with the wrong file type for chosen dcc package. Currently '
                                            'supported: [Maya, Blender]')
@click.argument('operation', default='query', help='Operation to perform. Options: [query, validate, modify, convert]')
@click.option('--options', default=['Preserve Transform Values'], help='Available options: Preserve Transform Values, '
                                                                       'Preserve Grouped Objects')
@click.option('--scope', default='Scene', help='Where is the content that needs to be converted. Options: '
                                               '[Scene, Directory]')


def run(src, dst, dcc, operation, options, scope):
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
        'dcc': dcc,
        'operation': operation,
        'options': options,
        'scope': scope
    }

    print('Material Generation CLI firing...')
    for key, value in options_dictionary.items():
        print(f'--> Key: {key}  Option: {value}')


if __name__ == '__main__':
    run()

