#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Setup the Python global variables and download and associate python to the correct package

ly_set(LY_PYTHON_VERSION 3.10.13)
ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.10)
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev1-linux-aarch64)
ly_set(LY_PYTHON_PACKAGE_HASH f8c0c6d0c744451a18058a05b9eef4a0f4e341b83af3c155aabc21c47b6a8302)
ly_set(LY_PYTHON_EXECUTABLE "python/bin/python")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "lib/python3.10/site-packages")

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
