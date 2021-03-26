# """
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.

# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# Utility functions for image comparison tests.
# """
# import test_tools.shared.images.qssim as qssim
# import PythonMagick
# import logging
# import os
# import shared.s3_utils
# import platform


####################################
# Commented out due to need to shift to new LyTestTools, Python3 and new screenshot workflow
# Don't merge to Mainline
####################################


# def create_image_path(screenshot, path, extension, golden):
    # """
    # Create image path from name, path and extension
    # From a specified path, create the path for the diff image
    # :param screenshot: path to the screenshot which needs to be saved
    # :param path: path to where screenshot should be saved
    # :param extension: extension of the diff image (.dds, .jpg)
    # :param golden: True or False whether screenshot is golden image or not
    # :return diff_full_path: path of the iamge to be saved
    # """
    # screenshot_name = os.path.basename(screenshot)
    # if golden:
        # diff_name = "{}_golden{}".format(screenshot_name.split('.')[:-1][0], extension)
    # else:
        # diff_name = "{}{}".format(screenshot_name.split('.')[:-1][0], extension)
    # diff_full_path = os.path.join(path, diff_name)
    # return diff_full_path


# def convert_dds_to_jpg(image, path, golden):
    # """
    # Convert DDS to JPEG
    # :param image: DDS image to convert
    # :param path: path to where iamge will be saved
    # :return screenshotJPG_path: path to the newly JPEG-converted DDS image
    # """
    # # Convert image as JPEG for quick review
    # screenshotJPG_path = create_image_path(image, path, '.jpg', golden)
    # screenshot = PythonMagick.Image(image)
    # screenshot.quality(100)
    # screenshot.magick('JPEG')
    # screenshot.write(screenshotJPG_path)
    # return screenshotJPG_path


# def compare_screenshot_to_golden_image(screenshot, golden_image, path, threshold=0.985):
    # """
    # Compare Screenshots to Golden Images
    # Function to compare a newly taken screenshot with the golden image
    # :param screenshot: path of the screenshot
    # :param golden_image: path of the golden image (in Perforce)
    # :param path: path to where the screenshot diff image will be saved
    # :param threshold: threshold for the image comparison test to fail/pass (optional)
    # :return failure_not_found: True or False whether screenshots are similar (due to threshold) or not
    # """
    # failure_not_found = True
    # logging.info("Comparing screenshot {}".format(screenshot))
    # # Calculating screenshots similarity
    # quaternion_similarity = qssim.qssim(screenshot, golden_image, diff_path = path)
    # # Converting original screenshots to jpg
    # convert_dds_to_jpg(screenshot, path, False)
    # convert_dds_to_jpg(golden_image, path, True)
    # # Checking if similarity index is bypassing the threshold
    # if (quaternion_similarity < threshold):
        # failure_not_found = False
        # logging.error("%s failed the image comparison with %s", screenshot, golden_image)
    # else:
        # logging.info("Comparison successful, screenshots are similar.")
    # return failure_not_found


# def upload_screenshots_to_s3(folder_path, folder_name):
    # """
    # Uploading screenshots to certain s3 bucket
    # Will require certain credentials (from the IAM that has access to l-qa@amazon acc) on the machine to work
    # :param folder_path: full path to folder that needs to be uploaded to s3
    # :param folder_name: name of the folder to be uploaded to s3
    # :return: None
    # """
    # host_name = platform.uname()[1]
    # s3_folder_name = '_'.join([folder_name, host_name])

    # logging.info("Trying to create a folder on S3; bucket: ly.screenshot.automation.artifacts, folder: {}".format(s3_folder_name))
    # shared.s3_utils.create_folder_in_bucket('ly.screenshot.automation.artifacts', s3_folder_name)

    # for file in os.listdir(folder_path):
        # key = '{}/{}'.format(s3_folder_name, file)
        # shared.s3_utils.upload_to_bucket('ly.screenshot.automation.artifacts', '{}/{}'.format(folder_path, file), key)

