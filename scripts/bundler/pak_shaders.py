#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import fnmatch
import glob
import os
import re
import sys
import zipfile


def _create_zip(source_path, zip_file_path, append, filter_list, ignore_list, compression_level, zip_internal_path = ''):
    """
    Internal function for creating a zip file containing the files that match the list of filters provided.
    File matching is case insensitive.
    """
    ignore_list = [filter.lower() for filter in ignore_list]
    filter_list = [filter.lower() for filter in filter_list]
    
    ignore_list = r'|'.join([fnmatch.translate(x) for x in ignore_list])
    try:
        mode = 'a' if append else 'w'        
        with zipfile.ZipFile(zip_file_path, mode, compression_level) as myzip:
            # Go through all the files in the source path.
            for root, dirnames, filenames in os.walk(source_path):
                # Remove files that match the ignore list.
                files = [os.path.relpath(os.path.join(root, file), source_path).lower() for file in filenames if not re.match(ignore_list, file.lower())]
                # Match the files we have found against the filters.
                for filter in filter_list:
                    for filename in fnmatch.filter(files, filter):
                        # Add the file to the zip using the specified internal destination.
                        myzip.write(os.path.join(source_path, filename), os.path.join(zip_internal_path, filename))
    except IOError as error:
       print("I/O error({0}) while creating zip file {1}: {2}".format(error.errno, zip_file_path, error.strerror))
       return False
    except:
       print("Unexpected error while creating zip file {0}: {1}".format(zip_file_path, sys.exc_info()[0]))
       return False

    return True

# Create or append a pak file with all the shaders found in source_path.
def pak_shaders_in_folder(source_path, output_folder, shader_type, append):
    """
    Creates the shadercache.pak and the shadercachestartup.pak using the shader files located at source_path.
    """

    ignore_list = ['shaderlist.txt', 'shadercachemisses.txt']

    shaders_cache_startup_filters = ['Common.cfib', 'FXConstantDefs.cfib', 'FXSamplerDefs.cfib', 'FXSetupEnvVars.cfib',
                                     'FXStreamDefs.cfib', 'fallback.cfxb', 'fallback.fxb', 'FixedPipelineEmu.cfxb',
                                     'FixedPipelineEmu.fxb', 'Stereo.cfxb', 'Stereo.fxb', 'lookupdata.bin',
                                     'Video.cfxb', 'Video.fxb',
                                     os.path.join('CGPShaders', 'FixedPipelineEmu@*'),
                                     os.path.join('CGVShaders', 'FixedPipelineEmu@*'),
                                     os.path.join('CGPShaders', 'FixedPipelineEmu', '*'),
                                     os.path.join('CGVShaders', 'FixedPipelineEmu', '*'),
                                     os.path.join('CGPShaders', 'Stereo@*'),
                                     os.path.join('CGVShaders', 'Stereo@*'),
                                     os.path.join('CGPShaders', 'Stereo', '*'),
                                     os.path.join('CGVShaders', 'Stereo', '*'),
                                     os.path.join('CGPShaders', 'Video@*'),
                                     os.path.join('CGVShaders', 'Video@*'),
                                     os.path.join('CGPShaders', 'Video', '*'),
                                     os.path.join('CGVShaders', 'Video', '*')
                                     ]

    print('Packing shader source folder {}'.format(source_path))
    result = True
    if os.path.exists(source_path):
        if not os.path.exists(output_folder):
            os.makedirs(output_folder)

        # We want the files to be added to the "shaders/cache/$shader_type" path inside the pak file.
        zip_interal_path = os.path.join('shaders', 'cache', shader_type)

        result &= _create_zip(source_path, os.path.join(output_folder, 'shadercache.pak'), append, ['*.*'], ignore_list, zipfile.ZIP_STORED, zip_interal_path)
        result &= _create_zip(source_path, os.path.join(output_folder, 'shadercachestartup.pak'), append, shaders_cache_startup_filters, ignore_list, zipfile.ZIP_STORED, zip_interal_path)
    else:
        print('[Error] Shader source folder is not available at {}. Shader type: {}.'.format(source_path, shader_type))
        result = False
    return result

# Generate a shaders pak file with all the shader types indicated.
# NOTE: A shader type can specify an specific source path or not. Examples:
#       - 'metal,specific/path/to/shaders': Use the folder specified as the source path to all metal shaders.
#       - 'metal': Use source_path/metal as source path to all metal shaders. Wildcard usage is allowed, for example 'gles3*'.
def pak_shaders(source_path, output_folder, shader_types):
    shader_flavors_packed = 0
    for shader_info in shader_types:
        # First element is the type, the second (if present) is the specific source
        shader_type = shader_info[0]
        if len(shader_info) > 1:
            shader_type_source = shader_info[1]
            if pak_shaders_in_folder(shader_type_source, output_folder, shader_type, shader_flavors_packed > 0):
                shader_flavors_packed = shader_flavors_packed + 1
            else:
                return False
        else:
            # No specific source path for this shader type so use the global source path. Wildcard allowed.
            listing = glob.glob(os.path.join(source_path, shader_type))
            for shader_type_source in listing:
                if os.path.isdir(shader_type_source):
                    # Since the shader_type can use wildcard we have to obtain the actual shader type found by removing source_path
                    # Example: If shader_type is 'gl4*' then now will be 'gl4_4'
                    shader_type = shader_type_source[len(source_path)+1:]
                    if pak_shaders_in_folder(shader_type_source, output_folder, shader_type, shader_flavors_packed > 0):
                        shader_flavors_packed = shader_flavors_packed + 1
                    else:
                        return False

    if shader_flavors_packed == 0:
        print('Failed to pack any shader type')

    return shader_flavors_packed > 0

def pair_arg(arg):
    return [str(x) for x in arg.split(',')]

parser = argparse.ArgumentParser(description='Pack the provided shader files into paks.')
parser.add_argument("output", type=str, help="specify the output folder")
parser.add_argument('-r', '--source', type=str, required=False, help="specify global input folder")
parser.add_argument('-s', '--shaders_types', required=True, nargs='+', type=pair_arg,
                    help='list of shader types with optional source path')

args = parser.parse_args()
print('Packing shaders...')
if not pak_shaders(args.source, args.output, args.shaders_types):
    print('Failed to pack shaders')
    exit(1)

print('Packs have been placed at "{}"'.format(args.output))
print('To use them, deploy them in your assets folder.')
print('Finish packing shaders')
