#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Setup the Python global variables and download and associate python to the correct package

# Python package info
ly_set(LY_PYTHON_VERSION 3.10.13)
ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.10)
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev1-linux)
ly_set(LY_PYTHON_PACKAGE_HASH 8b403923947f1e9fd102081f9962cae2e1e3833f2e30c0f2d33653e9606208f4)

# Python package relative paths
ly_set(LY_PYTHON_BIN_PATH "python/bin")
ly_set(LY_PYTHON_LIB_PATH "python/lib")
ly_set(LY_PYTHON_EXECUTABLE "python")

# Python venv relative paths
ly_set(LY_PYTHON_VENV_BIN_PATH "bin")
ly_set(LY_PYTHON_VENV_LIB_PATH "lib")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "${LY_PYTHON_VENV_LIB_PATH}/python3.10/site-packages")
ly_set(LY_PYTHON_VENV_PYTHON "${LY_PYTHON_VENV_BIN_PATH}/python")

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
