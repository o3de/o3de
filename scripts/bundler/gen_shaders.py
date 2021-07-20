#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import importlib
import json
import logging
import os
import pathlib
import shutil
import subprocess
import sys


ROOT_ENGINE_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
if ROOT_ENGINE_PATH not in sys.path:
    sys.path.append(ROOT_ENGINE_PATH)


class _Configuration:
    def __init__(self, platform, asset_platform, core_compiler):
        self.platform = platform
        self.asset_platform = asset_platform
        self.compiler = '{}-{}'.format(platform, core_compiler)

    def __str__(self):
        return '{} ({})'.format(self.platform, self.asset_platform)

class _ShaderType:
    def __init__(self, name, base_compiler):
        self.name = name
        self.core_compiler = '{}-{}'.format(base_compiler, name)
        self.configurations = []

    def add_configuration(self, platform, asset_platform):
        self.configurations.append(_Configuration(platform, asset_platform, self.core_compiler))

def find_shader_type(shader_type_name, shader_types):
    return next((shader for shader in shader_types if shader.name == shader_type_name), None)

def find_shader_configuration(platform, assets, shader_configurations):
    if platform:
        check_func = lambda config: config.platform == platform and config.asset_platform == assets
    else:
        check_func = lambda config: config.asset_platform == assets

    return next((config for config in shader_configurations if check_func(config)), None)

def error(msg):
    print(msg)
    exit(1)

def is_windows():
    if os.name == 'nt':
        return True
    else:
        return False


def read_project_name_from_project_json(project_path):
    project_name = None
    try:
        with (pathlib.Path(project_path) / 'project.json').open('r') as project_file:
            project_json = json.load(project_file)
            project_name = project_json['project_name']
    except OSError as os_error:
        logging.warning(f'Unable to open "project.json" file: {os_error}')
    except json.JSONDecodeError as json_error:
        logging.warning(f'Unable to decode json in {project_file}: {json_error}')
    except KeyError as key_error:
        logging.warning(f'{project_file} is missing project_name key: {key_error}')

    return project_name


def gen_shaders(shader_type, shader_config, shader_list, bin_folder, project_path, engine_path, verbose):
    """
    Generates the shaders for a specific platform and shader type using a list of shaders using ShaderCacheGen.
    The generated shaders will be output at Cache/<game_name>/<asset_platform>/user/cache/Shaders/Cache/<shader_type>
    """
    platform = shader_config.platform
    asset_platform = shader_config.asset_platform
    compiler = shader_config.compiler

    project_name = read_project_name_from_project_json(project_path)
    asset_cache_root = os.path.join(project_path, 'Cache', asset_platform)

    # Make sure that the <project-path>/user folder exists
    user_cache_folder = os.path.join(project_path, 'user', 'Cache')
    if not os.path.isdir(user_cache_folder):
        try:
            os.makedirs(user_cache_folder)
        except os.error as err:
            error("Unable to create the required cache folder '{}': {}".format(user_cache_folder, err))

    cache_shader_list = os.path.join(user_cache_folder, 'shaders', 'shaderlist.txt')

    if shader_list is None:
        if is_windows():
            shader_compiler_platform = 'x64'
        else:
            shader_compiler_platform = 'osx'

        shader_list_path = os.path.join(user_cache_folder, shader_compiler_platform, compiler,
                                        'ShaderList_{}.txt'.format(shader_type))
        if not os.path.isfile(shader_list_path):
            shader_list_path = cache_shader_list

        print("Source Shader List not specified, using {} by default".format(shader_list_path))
    else:
        shader_list_path = shader_list

    normalized_shaderlist_path = os.path.normpath(os.path.normcase(os.path.realpath(shader_list_path)))
    normalized_cache_shader_list = os.path.normpath(os.path.normcase(os.path.realpath(cache_shader_list)))

    if normalized_shaderlist_path != normalized_cache_shader_list:
        cache_shader_list_basename = os.path.split(cache_shader_list)[0]
        if not os.path.exists(cache_shader_list_basename):
            os.makedirs(cache_shader_list_basename)
        print("Copying shader_list from {} to {}".format(shader_list_path, cache_shader_list))
        shutil.copy2(shader_list_path, cache_shader_list)

    platform_shader_cache_path = os.path.join(user_cache_folder, 'shaders', 'cache', shader_type.lower())
    shutil.rmtree(platform_shader_cache_path, ignore_errors=True)

    shadergen_path = os.path.join(engine_path, bin_folder, 'ShaderCacheGen')
    if is_windows():
        shadergen_path += '.exe'

    if not os.path.isfile(shadergen_path):
        error("ShaderCacheGen could not be found at {}".format(shadergen_path))
    else:
        command_arguments = [
            shadergen_path,
            f'--project-path={project_path}',
            '--BuildGlobalCache',
            '--ShadersPlatform={}'.format(shader_type),
            '--TargetPlatform={}'.format(asset_platform)
        ]
        if verbose:
            print('Running: {}'.format(' '.join(command_arguments)))
        subprocess.call(command_arguments)

def add_shaders_types():
    """
    Add the shader types for the non restricted platforms.
    The compiler argument is used for locating the shader_list file.
    """
    shaders = []
    d3d11 = _ShaderType('D3D11', 'D3D11_FXC')
    d3d11.add_configuration('PC', 'pc')
    shaders.append(d3d11)

    gl4 = _ShaderType('GL4', 'GLSL_HLSLcc')
    gl4.add_configuration('PC', 'pc')
    shaders.append(gl4)

    gles3 = _ShaderType('GLES3', 'GLSL_HLSLcc')
    gles3.add_configuration('Android', 'android')
    shaders.append(gles3)

    metal = _ShaderType('METAL', 'METAL_LLVM_DXC')
    metal.add_configuration('Mac', 'mac')
    metal.add_configuration('iOS', 'ios')
    shaders.append(metal)

    restricted_path = os.path.join(ROOT_ENGINE_PATH, 'restricted')
    if os.path.exists(restricted_path):
        restricted_platforms = os.listdir(restricted_path)
        for platform in restricted_platforms:
            try:
                imported_module = importlib.import_module(f'restricted.{platform}.Tools.PakShaders.gen_shaders')
            except ImportError:
                continue

            restricted_func = getattr(imported_module, 'get_restricted_platform_shader', lambda: iter(()))

            for shader_type, shader_compiler, platform_name, asset_platform in restricted_func():

                shader = find_shader_type(shader_type, shaders)
                if shader is None:
                    shader = _ShaderType(shader_type, shader_compiler)
                    shaders.append(shader)

                shader.add_configuration(platform_name, asset_platform)

    return shaders

def check_arguments(args, parser, shader_types):
    """
    Check that the platform and shader type arguments are correct.
    """
    shader_names = [shader.name for shader in shader_types]

    shader_found = find_shader_type(args.shader_type, shader_types)
    if shader_found is None:
        parser.error('Invalid shader type {}. Must be one of [{}]'.format(args.shader_type, ' '.join(shader_names)))

    else:
        config_found = find_shader_configuration(args.shader_platform, args.asset_platform, shader_found.configurations)
        if config_found is None:
            parser.error('Invalid configuration for shader type "{}". It must be one of the following: {}'.format(shader_found.name, ', '.join(str(config) for config in shader_found.configurations)))


parser = argparse.ArgumentParser(description='Generates the shaders for a specific platform and shader type.')
parser.add_argument('asset_platform', type=str, help="The asset cache sub folder to use for shader generation")
parser.add_argument('shader_type', type=str, help="The shader type to use")
parser.add_argument('-p', '--shader-platform', type=str, required=False, default='', help="The target platform to generate shaders for.")
parser.add_argument('-b', '--bin-folder', type=str, help="Folder where the ShaderCacheGen executable lives. This is used along the project (ShaderCacheGen)")
parser.add_argument('-e', '--engine-path', type=str, help="Path to the engine root folder. This the same as game_path for non external projects")
parser.add_argument('-g', '--project-path', type=str, required=True, help="Path to the game root folder. This the same as engine_path for non external projects")
parser.add_argument('-s', '--shader-list', type=str, required=False, help="Optional path to the list of shaders. If not provided will use the list generated by the local shader compiler.")
parser.add_argument('-v', '--verbose', action="store_true", required=False, help="Increase the logging output")

args = parser.parse_args()

shader_types = add_shaders_types()

check_arguments(args, parser, shader_types)
print('Generating shaders for {} (shaders={}, platform={}, assets={})'.format(args.project_path, args.shader_type, args.shader_platform, args.asset_platform))

shader = find_shader_type(args.shader_type, shader_types)
shader_config = find_shader_configuration(args.shader_platform, args.asset_platform, shader.configurations)
gen_shaders(args.shader_type, shader_config, args.shader_list, args.bin_folder, args.project_path, args.engine_path, args.verbose)

print('Finish generating shaders')
