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
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev1-windows)
ly_set(LY_PYTHON_PACKAGE_HASH a6f1fd50552c8780852b75186a97469f77d55d06a01de73e5ca601e4a12be232)
ly_set(LY_PYTHON_EXECUTABLE "python/python.exe")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "Lib/site-packages")
ly_set(LY_PYTHON_VENV_PYTHON "Scripts/python.exe")

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
