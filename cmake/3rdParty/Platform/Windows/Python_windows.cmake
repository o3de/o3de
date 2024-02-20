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
ly_set(LY_PYTHON_PACKAGE_HASH e2b5926ac0d378ec08e5bef111177effc84f0e2dde457bb22d8acb604cb70b86)
ly_set(LY_PYTHON_EXECUTABLE "python/python.exe")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "Lib/site-packages")

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
