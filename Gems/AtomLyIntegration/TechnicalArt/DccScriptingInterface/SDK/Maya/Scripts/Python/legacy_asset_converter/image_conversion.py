# coding:utf-8
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# File Description:
# This file is contains OpenImageIO operations for file texture conversions
# -------------------------------------------------------------------------


#  built-ins
from shutil import copyfile
import logging as _logging
from pathlib import Path
import constants
import os

# 3rd Party (we may or do provide)
import OpenImageIO
from OpenImageIO import ImageInput, ImageOutput, ImageBuf, ImageSpec, ImageBufAlgo, ROI


module_name = 'legacy_asset_converter.main.image_conversion'
_LOGGER = _logging.getLogger(module_name)


def get_pbr_textures(legacy_textures, destination_directory, base_directory, uses_alpha):
    pbr_textures = {}
    for texture_type, texture_path in legacy_textures.items():
        _LOGGER.info(f'TEXTURETYPE::> {texture_type}  TEXTUREPATH::> {texture_path}')
        if not texture_path.is_file():
            _LOGGER.info(f'(((((((((((((((((((((((((((((((((((((((((((((((((((((((((((( Missing:::: {texture_path}')
            existing_path = resolve_path(texture_path, base_directory)

            # Fallback on tiff texture as it might exist like this on disk
            if not existing_path and texture_path.suffix == '.dds':
                existing_path = resolve_path(texture_path.with_suffix('.tif'), base_directory)

            if not existing_path:
                continue

            _LOGGER.info(f'Found texture [{existing_path}]. Continuing...')
            texture_path = Path(existing_path)
        if texture_type == 'diffuse':
            filemask = 'BaseColorA' if uses_alpha else 'BaseColor'
            dst = get_converted_filename(texture_path, destination_directory, filemask)
            pbr_textures['BaseColor'] = transfer_texture(dst, texture_path) if not os.path.isfile(dst) else dst
        elif texture_type == 'specular':
            dst = get_converted_filename(texture_path, destination_directory, 'Metallic')
            pbr_textures['Metallic'] = convert_metallic_texture(dst, texture_path) if not os.path.isfile(dst) else dst
        elif texture_type == 'emittance':
            dst = get_converted_filename(texture_path, destination_directory, 'Emissive')
            pbr_textures['Emissive'] = transfer_texture(dst, texture_path) if not os.path.isfile(dst) else dst
        elif texture_type == 'scattering':
            dst = get_converted_filename(texture_path, destination_directory, 'Scattering')
            pbr_textures['SubsurfaceScattering'] = transfer_texture(dst, texture_path) if not os.path.isfile(dst) else dst
        elif texture_type == 'bumpmap':
            dst = get_converted_filename(texture_path, destination_directory, 'Normal')
            pbr_textures['Normal'] = convert_normal_texture(dst, texture_path) if not os.path.isfile(dst) else dst
            if texture_path.stem.endswith('ddna'):
                dst = get_converted_filename(texture_path, destination_directory, 'Roughness')
                pbr_textures['Roughness'] = convert_roughness_texture(dst, texture_path) if not os.path.isfile(dst) else dst
        else:
            pass

    if pbr_textures and 'Metallic' not in pbr_textures.keys():
        _LOGGER.info(f'Metallic Not found... adding: {pbr_textures}')
        try:
            pbr_texture_keys = list(pbr_textures.keys())
            texture_path = Path(pbr_textures[pbr_texture_keys[0]])
            filename = texture_path.name.replace(get_filemask(texture_path), 'Metallic')
            _LOGGER.info(f'Filename: {filename}')
            target_file_path = destination_directory / filename
            _LOGGER.info(f'TargetFilePath: {target_file_path}')
            if target_file_path.is_file():
                _LOGGER.info(f'File found: {target_file_path}')
                pbr_textures['Metallic'] = os.path.normpath(target_file_path)
            else:
                _LOGGER.info(f'Creating Generic Metallic File: {target_file_path}')
                convert_metallic_texture(target_file_path)
                pbr_textures['Metallic'] = target_file_path
        except Exception:
            pass

    _LOGGER.info(f'OUTPUT // -------------->> {pbr_textures}')
    return pbr_textures


def resolve_path(texture_path, base_directory):
    _LOGGER.info(f'Resolving Path... Filename[{texture_path.name}: {base_directory}')
    
    # Try to match path via topmost assets folder
    split_dir = base_directory.as_posix().lower().split('/assets/')
    if len(split_dir) > 1:
        resolved_path = os.path.abspath(os.path.join(split_dir[0], 'assets', texture_path))
        if os.path.exists(resolved_path):
            return resolved_path

    cache_dir(base_directory)
    for (root, dirs, files) in os_walk_cache(base_directory):
        for file in files:
            if Path(file).name.lower() == texture_path.name.lower():
                return os.path.abspath(os.path.join(root, file))
    return None

cache = {}
def cache_dir(dir):
    if dir in cache:
        return
    else:
        cache[dir] = []
        for x in os.walk(dir, topdown=True):
            cache[dir].append(x)

def os_walk_cache(dir):
    if dir in cache:
        for x in cache[dir]:
            yield x

def transfer_texture(dst, src, overwrite=False):
    if not os.path.exists(dst) or overwrite:
        return Path(transfer_file(src, dst))
    return Path(dst)


def convert_normal_texture(dst, src, overwrite=False):
    if not os.path.exists(dst) or overwrite:
        try:
            rgba = ImageBuf(str(src))
            spec = get_image_spec(rgba)
            rgb = ImageBufAlgo.channels(rgba, (0, 1, 2))
            write_image(rgba, dst, spec['format'])
            _LOGGER.info(f'Output Normal Map: {dst}')
        except Exception as e:
            _LOGGER.info(f'{src} -- Normal Map Conversion Failed. Error: {e}')
            return None
    return Path(dst)


def convert_roughness_texture(dst, src, overwrite=False):
    if not os.path.exists(dst) or overwrite:
        try:
            rgba = ImageBuf(str(src))
            spec = get_image_spec(rgba)
            alpha = ImageBufAlgo.channels(rgba, (3,))
            roughness = ImageBufAlgo.invert(alpha)
            write_image(roughness, dst, spec['format'])
            _LOGGER.info(f'Output Roughness Map: {dst}')
        except Exception as e:
            _LOGGER.info(f'{src} -- Roughness Map Conversion Failed. Error: {e}')
            return None
    return Path(dst)


def convert_metallic_texture(dst, src=None, overwrite=False):
    _LOGGER.info(f'Convert Metallic ::::: >> {dst}')
    if not os.path.exists(dst) or overwrite:
        try:
            if src:
                _LOGGER.info(f'Converting Metallic image using: {src}')
                # Get min/max pixels ---------------------->>
                buf = ImageBuf(str(src))
                minval, maxval = find_min_max(buf)
                buf.clear()
                print(f'Minval: {minval}')
                print(f'Maxval: {maxval}')

                # Run remap filter using mix/max values -------->>
                buf = ImageBuf(str(src))
                spec = get_image_spec(buf)
                img_width = spec['resolution'][0]
                img_height = spec['resolution'][1]
                remap = get_contrast_remap(buf, minval, maxval)
                remove_color = remove_color_from_image(remap, dst)
            else:
                _LOGGER.info(f'Creating Dielectric Metallic Map')
                buf = ImageBuf(ImageSpec(4, 4, 1, "uint8"))
                ImageBufAlgo.fill(buf, constants.DIELECTRIC_METALLIC_COLOR, ROI(0, 1, 0, 1))
                write_image(buf, os.path.normpath(dst), 'uint8')
            _LOGGER.info(f'Output Metallic Map: {dst}')
        except Exception as e:
            src = 'None' if not src else src
            _LOGGER.info(f'{src} -- Metallic Map Conversion Failed. Error: {e}')
            return None
    return Path(dst)


def get_contrast_remap(buf, minval, maxval):
    range_threshold = (maxval - minval) * .4
    ceiling_adjustment = .65
    white_point = float(round(maxval * ceiling_adjustment, 3))
    black_point = float(round(white_point - range_threshold, 3))
    percentage = 15
    black_point = (percentage * (white_point - black_point) / 100) + black_point
    remap = ImageBufAlgo.contrast_remap(buf, black=black_point, white=white_point)
    return remap


def remove_color_from_image(buf, dst):
    """
    Some operations leave color in what should remain grayscale images (i.e. 'contrast_remap'). This function attempts
    to gently remove color information without affecting the luminosity of the image
    :param buf:
    :param dst:
    :return:
    """
    buf.read(convert='float')
    lin = ImageBufAlgo.colorconvert(buf, "sRGB", "linear")
    luma = ImageBufAlgo.channel_sum(buf, constants.WEIGHTED_RGB_VALUES)
    luma = ImageBufAlgo.colorconvert(luma, "linear", "sRGB")
    write_image(luma, dst, 'uint8')


def find_min_max(buf):
    """
    When attempting to automate the conversion of specular maps to metallic maps, the first step is to find high values
    of luminosity in the image and compare against the low values. The intention is to expose what would likely be
    metal and non-metal areas, and with this information attempt to clamp values to either black and white for the
    generation of the metallic map. This function decreases the size of the image to a manageable representation of
    pixel luminosity values, scans for the high and low values and returns those values to be further manipulated by
    the contrast remap filter
    :param buf: The image buffer containing the pixel information that needs to be scanned
    :return:
    """
    # Resize Image ------------------->>
    goal_width = 512
    goal_height = 512

    spec = buf.spec()
    w = spec.width
    h = spec.height
    nchans = spec.nchannels
    aspect = float(w) / float(h)
    if aspect >= 1.0:
        goal_height = int(h * goal_height / w)
    else:
        goal_width = (w * goal_width / h)
    resized = ImageBuf(ImageSpec(goal_width, goal_height, spec.nchannels, spec.format))
    ImageBufAlgo.resize(resized, buf)

    # Scan buffer for min/max
    pixels = resized.get_pixels(OpenImageIO.UINT8)
    minval = 255
    maxval = 0
    test_dimensions = 512

    for y in range(test_dimensions):
        for x in range(test_dimensions):
            pixel_value = pixels[y][x]
            average_value = sum(pixel_value) / 3

            if average_value < minval:
                minval = average_value
            if average_value > maxval:
                maxval = average_value

    minimum_level = minval / 255
    maximum_level = maxval / 255

    return minimum_level, maximum_level


def get_converted_filename(src, dst, texture_type):
    """
    Takes the structure of an existing filename extracted from the legacy mtl files, and renames it, base on the
    texture type argument that is passed to it
    :param src: The legacy filename to be manipulated
    :param dst: The destination directory that the new file will be saved to
    :param texture_type: PBR texture type
    :return: filepath
    """
    if src.is_file():
        filemask = get_filemask(src)
        filename = f'{src.stem}_{texture_type}{src.suffix}' if filemask[-1].isdigit() else src.name.replace('_' + filemask, '_' + texture_type)
        dst = os.path.normpath(dst / filename)
        _LOGGER.info(f'+=+=+=+=+=+=+=+=+=+=+=+ {dst} -- overwriting {os.path.isfile(dst)}')
    return dst


def get_image_spec(target_image):
    """
    Pulls metadata about the image to be leveraged in file operations
    :param target_image: Path to the target image intended for alterations
    :return:
    """
    spec = target_image.spec()
    info = {'resolution': (spec.width, spec.height, spec.x, spec.y), 'channels': spec.channelnames,
            'format': str(spec.format)}
    if spec.channelformats:
        info['channelformats'] = str(spec.channelformats)
    info['alpha channel'] = str(spec.alpha_channel)
    info['z channel'] = str(spec.z_channel)
    info['deep'] = str(spec.deep)
    for i in range(len(spec.extra_attribs)):
        if type(spec.extra_attribs[i].value) == str:
            info[spec.extra_attribs[i].name] = spec.extra_attribs[i].value
        else:
            info[spec.extra_attribs[i].name] = spec.extra_attribs[i].value
    return info


def get_texture_basename(image_path):
    """
    Attempts to extract the naming convention base for each texture set
    :param image_path: Path to an image from the texture set to use for basename extraction
    :return:
    """
    image_name = image_path.stem
    if image_name.find('_') != -1:
        image_components = image_name.split('_')
        return ('_').join(image_components[:-1])
    return None


def get_filemask(image_path):
    """
    Returns the string after the last '_' from filename
    :param image_path: Path to the image from which to extract pbr type
    :return:
    """
    filename = Path(image_path)
    texture_type = filename.stem.split('_')[-1]
    return texture_type


def set_image_spec(spec):
    """
    Writes new specification for images manipulated or created when needed
    :param spec:
    :return:
    """
    output_spec = ImageSpec()
    output_spec.set_format(OpenImageIO.UINT8)
    output_spec.width = spec['resolution'][0]
    output_spec.height = spec['resolution'][0]
    output_spec.nchannels = 1
    return output_spec


def write_image(image, filename, image_format):
    """
    Writes final processed image after operations have been completed and final result in buffer

    :param image: Image buffer
    :param filename: Image name to save
    :param image_format: File format for export
    :return:
    """
    if not image.has_error:
        image.set_write_format(image_format)
        image.write(filename)
    if image.has_error:
        _LOGGER.info(f'Error writing {filename}: {image.geterror()}')


def transfer_file(src, dst):
    """
    Some legacy textures only need to be renamed an moved to the processed folder with no further manipulation
    required. This function handles this transfer operation
    :param src: Source of the image intended for transfer
    :param dst: The destination path for the transferred image
    :return:
    """
    _LOGGER.info(f'{src} +++TRANSFER FILE+++>> {dst}')
    try:
        copyfile(src, dst)
        return dst
    except Exception as e:
        _LOGGER.info(f'Copy error encountered: {e}')
        return None

