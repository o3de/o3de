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
import os
# import main as app_main

@click.command()
@click.version_option('1.0.0')
@click.option('--output', default='pbr', help='O3DE material type. Current options: [pbr]')
@click.option('--material', default='true', help='Generate .material file per material found. Options: [true, false]')
@click.option('--logs', default='true', help='Generate log output. Options: [true, false]')
@click.option('--overwrite', default='false', help='Overwrite existing conversion files. Options: [true, false]')
@click.argument('source', type=click.STRING)
@click.argument('destination', type=click.STRING)
def main():
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
    name from which to set relative texture paths in the .material files.'

    # -- Conversion Options:
    # ---->> Create .material Files ::: Default setting = True
    # ---->> Create Output Logs ::: Default setting = True
    # ---->> Reprocess Existing Files ::: Default setting = False

    :param output:
    :param operands:
    :return:
    """
    print('Hello')


def validate():
    pass


if __name__ == '__main__':
    main()

