#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Setup the Python global variables and download and associate python to the correct package

# Python package info
ly_set(LY_PYTHON_VERSION 3.14.9)
ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.14)
ly_set(LY_PYTHON_PACKAGE_NAME python-3.14.9-rev1-windows)
ly_set(LY_PYTHON_PACKAGE_HASH 2fa922b213cb39654f38777a7d80929f3ca20e668a0941c4c108bcd61af62612)

# Python package relative paths
ly_set(LY_PYTHON_BIN_PATH "python")
ly_set(LY_PYTHON_LIB_PATH "Lib")
ly_set(LY_PYTHON_EXECUTABLE "python.exe")

# Python venv relative paths
ly_set(LY_PYTHON_VENV_BIN_PATH "Scripts")
ly_set(LY_PYTHON_VENV_LIB_PATH "Lib")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "${LY_PYTHON_VENV_LIB_PATH}/site-packages")
ly_set(LY_PYTHON_VENV_PYTHON "${LY_PYTHON_VENV_BIN_PATH}/python.exe")

function(ly_post_python_venv_install venv_path)
    # Noop
endfunction()
