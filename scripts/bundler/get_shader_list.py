#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import os
import pathlib
import subprocess

def error(msg):
    print(msg)
    exit(1)

def is_windows():
    if os.name == 'nt':
        return True
    else:
        return False


def get_shader_list(project_path, asset_platform, shader_type, shader_platform, shadergen_path):
    """
    Gets the shader list for a specific platform using ShaderCacheGen.
    Right now the shader list will always output at <project-path>/user/Cache/Shaders
    That will change when this is updated to take a destination path
    """
    shadergen_path = os.path.join(shadergen_path, 'ShaderCacheGen')
    if is_windows():
        shadergen_path += '.exe'

    command_args = [
        shadergen_path,
        f'--project-path={str(project_path)}'
        '--GetShaderList',
        '--ShadersPlatform={}'.format(shader_type),
        '--TargetPlatform={}'.format(asset_platform)
    ]

    if not os.path.isfile(shadergen_path):
        error("[ERROR] ShaderCacheGen could not be found at {}".format(shadergen_path))
    else:
        command = ' '.join(command_args)
        print('[INFO] get_shader_list: Running command - {}'.format(command))
        try:
            subprocess.check_call(command, shell=True)
        except subprocess.CalledProcessError:
            error('[ERROR] Failed to get the shader list for {}'.format(shader_type))


parser = argparse.ArgumentParser(description='Gets the shader list for a specific platform from the current shader compiler server')

parser.add_argument('-g', '--project-path', type=pathlib.Path, required=True, help="Path to the project")
parser.add_argument('asset-platform', type=str, help="The asset cache sub folder to use for shader generation")
parser.add_argument('shader-type', type=str, help="The shader type to use")
parser.add_argument('-p', '--shader_platform', type=str, required=False, default='', help="The target platform to generate shaders for.")
parser.add_argument('-s', '--shadergen_path', type=str, help="Path to where the the ShaderCacheGen executable lives")

args = parser.parse_args()

print('Getting shader list for {}'.format(args.asset_platform))
get_shader_list(args.project_path, args.asset_platform, args.shader_type, args.shader_platform, args.shadergen_path)
print('Finish getting shader list')
