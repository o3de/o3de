#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Setup the Python global variables and download and associate python to the correct package

# Python package info
ly_set(LY_PYTHON_VERSION 3.15.2)
ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.15)
ly_set(LY_PYTHON_PACKAGE_NAME python-3.15.2-rev2-mac-arm64)
ly_set(LY_PYTHON_PACKAGE_HASH bf6c6f10e6b78f11426eb74eea86a338f3f7357c226ce034a2ef194e1140792f)

# Python package relative paths
ly_set(LY_PYTHON_BIN_PATH "python/Python.framework/Versions/Current/bin/")
ly_set(LY_PYTHON_LIB_PATH "python/Python.framework/Versions/Current/lib")
ly_set(LY_PYTHON_EXECUTABLE "python3")

# Python venv relative paths
ly_set(LY_PYTHON_VENV_BIN_PATH "bin")
ly_set(LY_PYTHON_VENV_LIB_PATH "lib")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "${LY_PYTHON_VENV_LIB_PATH}/python3.15/site-packages")
ly_set(LY_PYTHON_VENV_PYTHON "${LY_PYTHON_VENV_BIN_PATH}/python")

function(ly_post_python_venv_install venv_path)
    # Noop
endfunction()
