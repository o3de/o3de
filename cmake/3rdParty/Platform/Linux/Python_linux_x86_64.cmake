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
ly_set(LY_PYTHON_PACKAGE_NAME python-3.10.13-rev2-linux)
ly_set(LY_PYTHON_PACKAGE_HASH a7832f9170a3ac93fbe678e9b3d99a977daa03bb667d25885967e8b4977b86f8)

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

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})

# On Linux, if the installed system python is the same version as the 3rd party python,
# we need to check if the system's python shared library is ABI compatible with the one
# in the 3rd party system. If it is, then we will skip creating the symlink to the shared
# library in the venv so that the PythonLoader will not attempt to load it into memory
# for the python library symbol resolution, otherwise it may collide if the system 
# shared python library was loaded by another dependency into the process symbols. (ie ROS2)
find_package(Python ${LY_PYTHON_VERSION_MAJOR_MINOR} 
             COMPONENTS Interpreter Development
             QUIET)

ly_set(LY_LINK_TO_SHARED_LIBRARY TRUE)
if (Python_FOUND)
    # If a system python is found, then make sure shared object is ABI compatible with the 3rd party python
    string(SUBSTRING "${Python_SOABI}" 0 11 CHECK_PYTHON_ABI_STRING)
    if ("${CHECK_PYTHON_ABI_STRING}" STREQUAL "cpython-310")
        # If the system python is ABI compatible, then do not create the sym-link
        ly_set(LY_LINK_TO_SHARED_LIBRARY FALSE)
    endif()
endif()

function(ly_post_python_venv_install venv_path)

    if (LY_LINK_TO_SHARED_LIBRARY)
        # We need to create a symlink to the shared library in the venv
        execute_process(COMMAND ln -s -f "${LY_3RDPARTY_PATH}/packages/${LY_PYTHON_PACKAGE_NAME}/${LY_PYTHON_LIB_PATH}/${LY_PYTHON_SHARED_LIB}" ${LY_PYTHON_SHARED_LIB}
                        WORKING_DIRECTORY "${PYTHON_VENV_PATH}/${LY_PYTHON_VENV_LIB_PATH}"
                        COMMAND_ECHO STDOUT
                        RESULT_VARIABLE command_result)
        if (NOT ${command_result} EQUAL 0)
            message(WARNING "Unable to createa venv shared library link.")
        endif()
    else()
        # No symlink needed, but make sure any existing one doesnt exist
        execute_process(COMMAND rm -f ${LY_PYTHON_SHARED_LIB}
                        WORKING_DIRECTORY "${PYTHON_VENV_PATH}/${LY_PYTHON_VENV_LIB_PATH}"
                        COMMAND_ECHO STDOUT)
    endif()

endfunction()
