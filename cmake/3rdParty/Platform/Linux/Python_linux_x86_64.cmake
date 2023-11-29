#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Setup the Python global variables and download and associate python to the correct package

ly_set(LY_PYTHON_VERSION 3.10.5)
ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.10)
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.5-rev4-linux)
ly_set(LY_PYTHON_PACKAGE_HASH 0144a4a5c9d39a834a319c98ce021741dac579bc39f60a08b6784b71d0983462)
ly_set(LY_PYTHON_EXECUTABLE "python/bin/python")

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
