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
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev1-darwin)
ly_set(LY_PYTHON_PACKAGE_HASH 14a88370fa8673344cf51354ce1a1021a0a1fb1742d774b5a0033a94e3384ec1)

# Python package relative paths
ly_set(LY_PYTHON_BIN_PATH "Python.framework/Versions/Current/bin/")
ly_set(LY_PYTHON_LIB_PATH "Python.framework/Versions/Current/lib")
ly_set(LY_PYTHON_EXECUTABLE "python3")

# Python venv relative paths
ly_set(LY_PYTHON_VENV_BIN_PATH "bin")
ly_set(LY_PYTHON_VENV_LIB_PATH "lib")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "${LY_PYTHON_VENV_LIB_PATH}/python3.10/site-packages")
ly_set(LY_PYTHON_VENV_PYTHON "${LY_PYTHON_VENV_BIN_PATH}/python")

function(ly_post_python_venv_install venv_path)
    # Noop
endfunction()
