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
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev1-darwin)
ly_set(LY_PYTHON_PACKAGE_HASH e3474ce2d61805f606c003df3d9299fda6ad8f98f1b44096de103bb9cd8efff2)
ly_set(LY_PYTHON_EXECUTABLE "Python.framework/Versions/Current/bin/python3")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "lib/python3.10/site-packages")

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
