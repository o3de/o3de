""" 
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Python implementation of Quaternion Structural Similarity by Amir Kolaman and Orly Yadid-Pecht as seen in
IEEE Transaction on Image Processing Vol 21 No 4 April 2012.
"""

import imageio
import numpy
import os

from scipy import ndimage


def _quaternion_matrix_conj(q):
    q_out = numpy.zeros(q.shape)
    q_out[:, :, 0] = q[:, :, 0]
    q_out[:, :, 1] = -q[:, :, 1]
    q_out[:, :, 2] = -q[:, :, 2]
    q_out[:, :, 3] = -q[:, :, 3]
    return q_out


def _quaternion_matrix_dot(q1, q2):
    return numpy.sqrt(numpy.sum(numpy.multiply(q1, q2), 2))


def _quaternion_matrix_norm(q):
    return _quaternion_matrix_dot(q, q)


def _quaternion_matrix_mult(q1, q2):
    """q = (q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3],
        q1[0] * q2[1] + q1[1] * q2[0] - q1[2] * q2[3] + q1[3] * q2[2],
        q1[0] * q2[2] + q1[1] * q2[3] + q1[2] * q2[0] - q1[3] * q2[1],
        q1[0] * q2[3] - q1[1] * q2[2] + q1[2] * q2[1] + q1[3] * q2[0])"""
    # add error checking for q1 and q2 being same size
    q = numpy.zeros(q1.shape)
    q[:, :, 0] = numpy.multiply(q1[:, :, 0], q2[:, :, 0]) - numpy.multiply(q1[:, :, 1], q2[:, :, 1]) \
                 - numpy.multiply(q1[:, :, 2], q2[:, :, 2]) - numpy.multiply(q1[:, :, 3], q2[:, :, 3])

    q[:, :, 1] = numpy.multiply(q1[:, :, 0], q2[:, :, 1]) + numpy.multiply(q1[:, :, 1], q2[:, :, 0]) \
                 - numpy.multiply(q1[:, :, 2], q2[:, :, 3]) + numpy.multiply(q1[:, :, 3], q2[:, :, 2])

    q[:, :, 2] = numpy.multiply(q1[:, :, 0], q2[:, :, 2]) + numpy.multiply(q1[:, :, 1], q2[:, :, 3]) \
                 + numpy.multiply(q1[:, :, 2], q2[:, :, 0]) - numpy.multiply(q1[:, :, 3], q2[:, :, 1])

    q[:, :, 3] = numpy.multiply(q1[:, :, 0], q2[:, :, 3]) - numpy.multiply(q1[:, :, 1], q2[:, :, 2]) \
                 + numpy.multiply(q1[:, :, 2], q2[:, :, 1]) + numpy.multiply(q1[:, :, 3], q2[:, :, 0])

    return q


def _quaternion_matrix_div(q1, q2):
    q2_norm = _quaternion_matrix_norm(q2)
    q = _quaternion_matrix_mult(q1, _quaternion_matrix_conj(q2))
    return numpy.divide(q, numpy.dstack([q2_norm] * 4))


def qssim(screenshot, goldenimage, channel_max=255, diff_path='.'):
    """
    Returns the mean quaternion similarity index between two images.
    For images that are the same the expected result is 1.000.
    By default we are assuming a channel max of 255(8bit channels)

    There are a series of tuning parameters that are taken from the 2004 paper by Wang et al
    Image Quality Assesment: From Error Visibility to Structural Similarity.

    :param screenshot: Screenshot filename to test
    :param goldenimage: Golden image to test against.
    :param channel_max: Maximum channel value.
    :param diff_path: Target path where diff image should be stored.
    :return: Mean quaternion similarity from 0.00->1.00 (identical).
    """

    # load image and copy it into a 4 channel array to treat rgb like quaternions
    img1 = imageio.imread(screenshot)
    img2 = imageio.imread(goldenimage)
    # Avoid later precision issues
    img1 = img1.astype(numpy.float64) / channel_max
    img2 = img2.astype(numpy.float64) / channel_max

    im1_size = img1.shape
    im2_size = img2.shape
    # check im1 and im2 are same size

    hue1 = numpy.zeros((im1_size[0], im1_size[1], im1_size[2] + 1))
    hue2 = numpy.zeros((im1_size[0], im1_size[1], im1_size[2] + 1))
    mu1 = numpy.zeros((im1_size[0], im1_size[1], im1_size[2] + 1))
    mu2 = numpy.zeros((im1_size[0], im1_size[1], im1_size[2] + 1))
    offset1 = numpy.zeros((im1_size[0], im1_size[1], im1_size[2] + 1))
    offset2 = numpy.zeros((im1_size[0], im1_size[1], im1_size[2] + 1))

    hue1[:, :, 1:4] = img1
    hue2[:, :, 1:4] = img2
    mu1[:, :, 1:4] = img1
    mu2[:, :, 1:4] = img2

    # Algorithm tuning parameters. Can me modified as needed.
    sigma = 1.5

    # These parameters are just to prevent divide by zero issues.
    L = 1
    K1 = 0.01
    K2 = 0.03

    C1 = (K1 * L) ** 2
    C2 = (K2 * L) ** 2
    offset1[:, :, 0] = C1
    offset2[:, :, 0] = C2

    # blur each color channel of both images
    mu1 = ndimage.gaussian_filter1d(mu1, sigma, 0)
    mu1 = ndimage.gaussian_filter1d(mu1, sigma, 1)
    mu2 = ndimage.gaussian_filter1d(mu2, sigma, 0)
    mu2 = ndimage.gaussian_filter1d(mu2, sigma, 1)

    mu1_sq = _quaternion_matrix_mult(mu1, _quaternion_matrix_conj(mu1))
    mu2_sq = _quaternion_matrix_mult(mu2, _quaternion_matrix_conj(mu2))
    mu12 = _quaternion_matrix_mult(mu1, _quaternion_matrix_conj(mu2))
    hue1_sq = _quaternion_matrix_mult(hue1 - mu1, _quaternion_matrix_conj(hue1 - mu1))
    hue2_sq = _quaternion_matrix_mult(hue2 - mu2, _quaternion_matrix_conj(hue2 - mu2))
    hue12 = _quaternion_matrix_mult(hue1 - mu1, _quaternion_matrix_conj(hue2 - mu2))

    sigma1 = ndimage.gaussian_filter1d(hue1_sq, sigma, 0)
    sigma1 = ndimage.gaussian_filter1d(sigma1, sigma, 1)
    sigma2 = ndimage.gaussian_filter1d(hue2_sq, sigma, 0)
    sigma2 = ndimage.gaussian_filter1d(sigma2, sigma, 1)
    sigma12 =ndimage.gaussian_filter1d(hue12, sigma, 0)
    sigma12 =ndimage.gaussian_filter1d(sigma12, sigma, 1)
    numerator1 = 2 * mu12 + offset1
    numerator2 = 2 * sigma12 + offset2

    denominator1 = mu1_sq + mu2_sq + offset1
    denominator2 = sigma1 + sigma2 + offset2

    part1 = _quaternion_matrix_norm(numerator1) / denominator1[:, :, 0]
    part2 = _quaternion_matrix_norm(numerator2) / denominator2[:, :, 0]

    qssim_map = numpy.multiply(part1, part2)

    extension = os.path.splitext(screenshot)[1]
    screenshot_name = os.path.basename(screenshot)
    diff_name = '.'.join(screenshot_name.split('.')[:-1]) + "_diff" + extension
    diff_full_path = os.path.join(diff_path, diff_name)

    imageio.imwrite(diff_full_path, (qssim_map * channel_max).astype(numpy.uint8))
    return ndimage.mean(numpy.abs(qssim_map))


if __name__ == "__main__":
    """ If this is run by accident, inform user that this is a module, not a separate script. """
    print('qssim.py is not a standalone script.')

