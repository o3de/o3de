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
import argparse
import os
import shutil
import subprocess

def error(msg):
    print(msg)
    exit(1)

def is_windows():
    if os.name == 'nt':
        return True
    else:
        return False


def get_shader_list(game_name, asset_platform, shader_type, shader_platform, shadergen_path):
    """
    Gets the shader list for a specific platform using ShaderCacheGen.
    Right now the shader list will always output at Cache/<game_name>/<asset_platform>/user/cache/shaders
    That will change when this is updated to take a destination path
    """
    shadergen_path = os.path.join(shadergen_path, 'ShaderCacheGen')
    if is_windows():
        shadergen_path += '.exe'

    command_args = [
        shadergen_path,
        '/GetShaderList',
        '/ShadersPlatform={}'.format(shader_type),
        '/TargetPlatform={}'.format(asset_platform)
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

parser.add_argument('game_name', type=str, help="Name of the game")
parser.add_argument('asset_platform', type=str, help="The asset cache sub folder to use for shader generation")
parser.add_argument('shader_type', type=str, help="The shader type to use")
parser.add_argument('-p', '--shader_platform', type=str, required=False, default='', help="The target platform to generate shaders for.")
parser.add_argument('-s', '--shadergen_path', type=str, help="Path to where the the ShaderCacheGen executable lives")

args = parser.parse_args()

print('Getting shader list for {}'.format(args.asset_platform))
get_shader_list(args.game_name, args.asset_platform, args.shader_type, args.shader_platform, args.shadergen_path)
print('Finish getting shader list')
