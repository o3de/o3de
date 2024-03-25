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
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev2-linux-aarch64)
ly_set(LY_PYTHON_PACKAGE_HASH 30bc2731e2ac54d8e22d36ab15e30b77aefe2dce146ef92d6f20adc0a9c5b14e)

# Python package relative paths
ly_set(LY_PYTHON_BIN_PATH "python/bin")
ly_set(LY_PYTHON_LIB_PATH "python/lib")
ly_set(LY_PYTHON_EXECUTABLE "python")
ly_set(LY_PYTHON_SHARED_LIB "libpython3.10.so.1.0")

# Python venv relative paths
ly_set(LY_PYTHON_VENV_BIN_PATH "bin")
ly_set(LY_PYTHON_VENV_LIB_PATH "lib")
ly_set(LY_PYTHON_VENV_SITE_PACKAGES "${LY_PYTHON_VENV_LIB_PATH}/python3.10/site-packages")
ly_set(LY_PYTHON_VENV_PYTHON "${LY_PYTHON_VENV_BIN_PATH}/python")

function(ly_post_python_venv_install venv_path)

    # We need to create a symlink to the shared library in the venv
    execute_process(COMMAND ln -s -f "${PYTHON_PACKAGES_ROOT_PATH}/${LY_PYTHON_PACKAGE_NAME}/${LY_PYTHON_LIB_PATH}/${LY_PYTHON_SHARED_LIB}" ${LY_PYTHON_SHARED_LIB}
                    WORKING_DIRECTORY "${PYTHON_VENV_PATH}/${LY_PYTHON_VENV_LIB_PATH}"
                    RESULT_VARIABLE command_result)
    if (NOT ${command_result} EQUAL 0)
        message(WARNING "Unable to create a venv shared library link.")
    endif()

endfunction()
