# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------

import OpenImageIO as oiio

def convert(src):
    try:
        rgba = oiio.ImageBuf(src)
        spec = get_image_spec(rgba)

        # Normal output ------>
        rgb = oiio.ImageBufAlgo.channels(rgba, (0, 1, 2))
        normal_output = src.replace('ddna', 'Normal')

        # Roughness output ------>
        alpha = oiio.ImageBufAlgo.channels(rgba, (3,))
        roughness = oiio.ImageBufAlgo.invert(alpha)
        roughness_output = normal_output.replace('Normal', 'Roughness')

        write_image(rgba, normal_output, spec['format'])
        write_image(roughness, roughness_output, spec['format'])
        # logging.info('Output Normal Map: {}'.format(normal_output))
        # logging.info('Output Roughness Map: {}'.format(roughness_output))

    except Exception as e:
        logging.info('OIIO error in image conversion: {}'.format(e))
        return None

def get_image_spec(target_image):
    spec = target_image.spec()
    info = {'resolution': (spec.width, spec.height, spec.x, spec.y), 'channels': spec.channelnames,
                'format': str(spec.format)}
    if spec.channelformats :
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

def write_image(image, filename, image_format):
    if not image.has_error:
        image.set_write_format(image_format)
        image.write(filename)
    if image.has_error:
        print("Error writing", filename, ":", image.geterror())

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # run a test
    convert('AA_Gun_01_ddna.tif')
    # validate()
