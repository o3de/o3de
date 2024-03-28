#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard()

include(cmake/LySet.cmake)
include(cmake/3rdPartyPackages.cmake)

# this script exists to make sure a python interpreter is immediately available
# it will both locate and run pip on python for our requirements.txt
# but you can also call update_pip_requirements(filename) at any time after.

# this is different from the usual package usage, because even if we are targeting
# android, for example, we may still be doing so on a windows HOST pc, and the
# python interpreter we want to use is for the windows HOST pc, not the PAL platform:


set(LY_PAL_PYTHON_PACKAGE_FILE_NAME ${LY_ROOT_FOLDER}/cmake/3rdParty/Platform/${PAL_HOST_PLATFORM_NAME}/Python_${PAL_HOST_PLATFORM_NAME_LOWERCASE}${LY_HOST_ARCHITECTURE_NAME_EXTENSION}.cmake)
cmake_path(NORMAL_PATH LY_PAL_PYTHON_PACKAGE_FILE_NAME)
include(${LY_PAL_PYTHON_PACKAGE_FILE_NAME})

# settings and globals
ly_set(LY_PYTHON_DEFAULT_REQUIREMENTS_TXT "${LY_ROOT_FOLDER}/python/requirements.txt")

# The expected venv for python is based on an ID generated based on the path of the current
# engine using the logic in cmake/CalculateEnginePathId.cmake. 
execute_process(COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CalculateEnginePathId.cmake "${CMAKE_CURRENT_SOURCE_DIR}/"
                OUTPUT_VARIABLE ENGINE_SOURCE_PATH_ID
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Normalize the expected location of the Python venv and set its path globally
# The root Python folder is based off of the same folder as the manifest file
if(DEFINED ENV{USERPROFILE} AND EXISTS $ENV{USERPROFILE})
    set(PYTHON_ROOT_PATH "$ENV{USERPROFILE}/.o3de/Python") # Windows
else()
    set(PYTHON_ROOT_PATH "$ENV{HOME}/.o3de/Python") # Unix
endif()

set(PYTHON_PACKAGES_ROOT_PATH "${PYTHON_ROOT_PATH}/packages")
cmake_path(NORMAL_PATH PYTHON_PACKAGES_ROOT_PATH )

set(PYTHON_PACKAGE_CACHE_ROOT_PATH "${PYTHON_ROOT_PATH}/downloaded_packages")
cmake_path(NORMAL_PATH PYTHON_PACKAGE_CACHE_ROOT_PATH )

set(PYTHON_VENV_PATH "${PYTHON_ROOT_PATH}/venv/${ENGINE_SOURCE_PATH_ID}")
cmake_path(NORMAL_PATH PYTHON_VENV_PATH )

ly_set(LY_PYTHON_VENV_PATH ${PYTHON_VENV_PATH})

function(ly_setup_python_venv)

    # Check if we need to setup a new venv.
    set(CREATE_NEW_VENV FALSE)
    # We track if an existing venv matches the current version of the Python 3rd Party Package
    # by tracking the package hash within the venv folder. If there it is missing or does not
    # match the current package hash ${LY_PYTHON_PACKAGE_HASH} then reset the venv and request
    # that a new venv is created with the current Python package at :
    # 
    if (EXISTS "${PYTHON_VENV_PATH}/.hash")
        file(READ "${PYTHON_VENV_PATH}/.hash" LY_CURRENT_VENV_PACKAGE_HASH
             LIMIT 80)
        if (NOT ${LY_CURRENT_VENV_PACKAGE_HASH} STREQUAL ${LY_PYTHON_PACKAGE_HASH})
            # The package hash changed, re-install the venv
            set(CREATE_NEW_VENV TRUE)
            file(REMOVE_RECURSE "${PYTHON_VENV_PATH}")
        else()
            # Sanity check to make sure the python launcher in the venv works
            execute_process(COMMAND ${PYTHON_VENV_PATH}/${LY_PYTHON_VENV_PYTHON} --version
                            OUTPUT_QUIET
                            RESULT_VARIABLE command_result)
            if (NOT ${command_result} EQUAL 0)
                message(STATUS "Error validating python inside the venv. Reinstalling")
                set(CREATE_NEW_VENV TRUE)
                file(REMOVE_RECURSE "${PYTHON_VENV_PATH}")
            else()
                message(STATUS "Using Python venv at ${PYTHON_VENV_PATH}")
            endif()
        endif()
    else()
        set(CREATE_NEW_VENV TRUE)
    endif()

    if (CREATE_NEW_VENV)
        # Run the install venv command, but skip the pip setup because we may need to manually create 
        # a link to the shared library within the created virtual environment before proceeding with
        # the pip install command.
        message(STATUS "Creating Python venv at ${PYTHON_VENV_PATH}")
        execute_process(COMMAND "${PYTHON_PACKAGES_ROOT_PATH}/${LY_PYTHON_PACKAGE_NAME}/${LY_PYTHON_BIN_PATH}/${LY_PYTHON_EXECUTABLE}" -m venv "${PYTHON_VENV_PATH}" --without-pip --clear
                        WORKING_DIRECTORY "${PYTHON_PACKAGES_ROOT_PATH}/${LY_PYTHON_PACKAGE_NAME}/${LY_PYTHON_BIN_PATH}"
                        COMMAND_ECHO STDOUT
                        RESULT_VARIABLE command_result)

        if (NOT ${command_result} EQUAL 0)
            message(FATAL_ERROR "Error creating a venv")
        endif()

        ly_post_python_venv_install(${PYTHON_VENV_PATH})

        # Manually install pip into the virtual environment
        message(STATUS "Installing pip into venv at ${PYTHON_VENV_PATH} (${PYTHON_VENV_PATH}/${LY_PYTHON_BIN_PATH})")

        execute_process(COMMAND "${PYTHON_VENV_PATH}/${LY_PYTHON_VENV_PYTHON}" -m ensurepip --upgrade --default-pip -v
                        WORKING_DIRECTORY "${PYTHON_VENV_PATH}/${LY_PYTHON_VENV_BIN_PATH}"
                        COMMAND_ECHO STDOUT
                        RESULT_VARIABLE command_result)

        if (NOT ${command_result} EQUAL 0)
            message(FATAL_ERROR "Error installing pip into venv: ${LY_PIP_ERROR}")
        endif()

        file(WRITE "${PYTHON_VENV_PATH}/.hash" ${LY_PYTHON_PACKAGE_HASH})
    endif()
    
endfunction()

# update_pip_requirements
#    param: requirements_file_path = path to a requirements.txt file.
# ensures that all the requirements in the requirements.txt are present.
# you can call it repeatedly on sub-requirement.txt files (for exmaple, in gems)
# note that unique_name is a string of your choosing to track and refer to this
# file, and should be unique to your particular gem/package/3rdParty.  It will be
# used as a file name, so avoid special characters that would fail as a file name.
function(update_pip_requirements requirements_file_path unique_name)

    # we run with --no-deps to prevent it from cascading to child dependencies 
    # and getting more than we expect.
    # to produce a new requirements.txt use pip-compile from pip-tools alongside pip freeze
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${requirements_file_path})

    # We skip running requirements.txt if we can (we use a stamp file to keep track)
    # the stamp file is kept in the binary folder with a similar file path to the source file.
    set(stamp_file ${LY_PYTHON_VENV_PATH}/requirements_files/${unique_name}.stamp)
    get_filename_component(stamp_file_directory ${stamp_file} DIRECTORY)
    file(MAKE_DIRECTORY ${stamp_file_directory})

    if(EXISTS ${stamp_file} AND ${stamp_file} IS_NEWER_THAN ${requirements_file_path})
        # this means we more recently ran PIP than the requirements file was changed.
        # however, users may have deleted and reinstalled python.  If this happens
        # then the package will be newer than our stamp file, and we must not return.
        ly_package_is_newer_than(${LY_PYTHON_PACKAGE_NAME} ${stamp_file} package_is_newer)
        if (NOT package_is_newer)
            # we can early out becuase the python installation is older than our stamp file
            # and the stamp file is newer than the requirements.txt
            return()
        endif()
    endif()

    message(CHECK_START "Python: Getting/Checking packages listed in ${requirements_file_path}")
    
    # dont allow or use installs in python %USER% location.
    if (NOT DEFINED ENV{PYTHONNOUSERSITE})
        set(REMOVE_USERSITE TRUE)
    endif()

    set(ENV{PYTHONNOUSERSITE} 1)

    execute_process(COMMAND 
        ${LY_PYTHON_CMD} -m pip install -r "${requirements_file_path}" --disable-pip-version-check --no-warn-script-location
        WORKING_DIRECTORY ${LY_ROOT_FOLDER}/python
        RESULT_VARIABLE PIP_RESULT
        OUTPUT_VARIABLE PIP_OUT 
        ERROR_VARIABLE PIP_OUT
        )

    message(VERBOSE "pip result: ${PIP_RESULT}")
    message(VERBOSE "pip output: ${PIP_OUT}")

    if (NOT ${PIP_RESULT} EQUAL 0)
        message(CHECK_FAIL "Failed to fetch / update python dependencies from ${requirements_file_path}\nPip install log:\n${PIP_OUT}")
        message(FATAL_ERROR "The above failure will cause errors later - stopping now.  Check the output log (above) for details.")
    else()
        string(FIND "${PIP_OUT}" "Installing collected packages" NEW_PACKAGES_INSTALLED)

        if (NOT ${NEW_PACKAGES_INSTALLED} EQUAL -1)
            # this indicates it was found, meaning new stuff was installed.
            # in this case, output the pip output to normal message mode
            message(VERBOSE ${PIP_OUT})
            message(CHECK_PASS "New packages were installed")
        else()
            message(CHECK_PASS "Already up to date.")
        endif()

        # since we're in a success state, stamp the stampfile so we don't run this rule again 
        # unless someone updates the requirements.txt file.
        file(TOUCH ${stamp_file})
    endif()

    # finally, verify that all packages are OK.  This runs locally and does not
    # hit any repos, so its fairly quick.

    execute_process(COMMAND 
        ${LY_PYTHON_CMD} -m pip check
        WORKING_DIRECTORY ${LY_ROOT_FOLDER}/python
        RESULT_VARIABLE PIP_RESULT
        OUTPUT_VARIABLE PIP_OUT 
        ERROR_VARIABLE PIP_OUT
        )
    
    message(VERBOSE "Results from pip check: ${PIP_OUT}")

    if (NOT ${PIP_RESULT} EQUAL 0)
        message(WARNING "PIP reports unmet dependencies: ${PIP_OUT}")
    endif()

    if (REMOVE_USERSITE)
        unset(ENV{PYTHONNOUSERSITE})
    endif()

endfunction()

# allows you to install a folder into your site packages as an 'editable' package
# meaning, it will show up in python but not actually be copied to your site-packages
# folder, instead, it will be linked from there.
# the pip_package_name should be the name given to the package in setup.py so that
# any old versions may be uninstalled using setuptools before we install the new one.
function(ly_pip_install_local_package_editable package_folder_path pip_package_name)
    set(stamp_file ${LY_PYTHON_VENV_PATH}/packages/pip_installs/${pip_package_name}.stamp)
    get_filename_component(stamp_file_directory ${stamp_file} DIRECTORY)
    file(MAKE_DIRECTORY ${stamp_file_directory})
    
    # for the first release of the o3de snap we will only use packages shipped with o3de
    if ($ENV{O3DE_SNAP})
        file(TOUCH ${stamp_file})
    endif()
   
    # we only ever need to do this once per runtime install, since its a link
    # not an actual install:

    # If setup.py changes we must reinstall the package in case its dependencies changed
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${package_folder_path}/setup.py)

    if(EXISTS ${stamp_file} AND ${stamp_file} IS_NEWER_THAN ${package_folder_path}/setup.py)
        ly_package_is_newer_than(${LY_PYTHON_PACKAGE_NAME} ${stamp_file} package_is_newer)
        if (NOT package_is_newer)
            # no need to run the command again, as the package is older than the stamp file
            # and the stamp file exists.
            return()
        endif()
    endif()

    message(CHECK_START "Python: linking ${package_folder_path} into python site-packages...")
    
    # dont allow or use installs in python %USER% location.
    if (NOT DEFINED ENV{PYTHONNOUSERSITE})
        set(REMOVE_USERSITE TRUE)
    endif()

    set(ENV{PYTHONNOUSERSITE} 1)

    # uninstall any old versions of this package first:
    execute_process(COMMAND 
        ${LY_PYTHON_CMD} -m pip uninstall ${pip_package_name} -y  --disable-pip-version-check 
        WORKING_DIRECTORY ${LY_ROOT_FOLDER}/python
        RESULT_VARIABLE PIP_RESULT
        OUTPUT_VARIABLE PIP_OUT 
        ERROR_VARIABLE PIP_OUT
        )

    # we discard the error output of above, since it might not be installed, which is ok
    message(VERBOSE "pip uninstall result: ${PIP_RESULT}")
    message(VERBOSE "pip uninstall output: ${PIP_OUT}")

    # now install the new one:

    execute_process(COMMAND 
        ${LY_PYTHON_CMD} -m pip install -e ${package_folder_path} --no-deps --disable-pip-version-check  --no-warn-script-location 
        WORKING_DIRECTORY ${LY_ROOT_FOLDER}/python
        RESULT_VARIABLE PIP_RESULT
        OUTPUT_VARIABLE PIP_OUT 
        ERROR_VARIABLE PIP_OUT
        )

    message(VERBOSE "pip install result: ${PIP_RESULT}")
    message(VERBOSE "pip install output: ${PIP_OUT}")

    if (NOT ${PIP_RESULT} EQUAL 0)
        message(CHECK_FAIL "Failed to install ${package_folder_path}: ${PIP_OUT} - use CMAKE_MESSAGE_LOG_LEVEL to VERBOSE for more information")
        message(FATAL_ERROR "Failure to install a python package will likely cause errors further down the line, stopping!")
    else()
        file(TOUCH ${stamp_file})
    endif()

    if (REMOVE_USERSITE)
        unset(ENV{PYTHONNOUSERSITE})
    endif()
endfunction()

# python is a special case of third party:
#  * We download it into a folder in the build tree
#  * The package is modified as time goes on (for example, PYC files appear)
#  * we add pip-packages to the site-packages folder
# Because of this, we want a strict verification the first time we download it
# But we don't want to full verify using hashes after we successfully get it the
# first time.


# We need to download the associated Python package early and install the venv 
ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
ly_set_package_download_location(${LY_PYTHON_PACKAGE_NAME} ${PYTHON_PACKAGES_ROOT_PATH})
ly_set_package_download_cache_location(${LY_PYTHON_PACKAGE_NAME} ${PYTHON_PACKAGE_CACHE_ROOT_PATH})
ly_download_associated_package(Python)
ly_setup_python_venv()

if (NOT CMAKE_SCRIPT_MODE_FILE)


    # note - if you want to use a normal python via FindPython instead of the LY package above,
    # you may have to declare the below variables after find_package, as the project scripts are 
    # looking for the below variables specifically.
    find_package(Python ${LY_PYTHON_VERSION} REQUIRED)

    # verify the required variables are present:
    if (NOT EXISTS "${PYTHON_VENV_PATH}/" OR NOT Python_EXECUTABLE OR NOT Python_HOME OR NOT Python_PATHS)
        message(SEND_ERROR "Python installation not valid expected to find all of the following variables set:")
        message(STATUS "    Python_EXECUTABLE:  ${Python_EXECUTABLE}")
        message(STATUS "    Python_HOME:        ${Python_HOME}")
        message(STATUS "    Python_PATHS:       ${Python_PATHS}")
    else()
        LIST(APPEND CMAKE_PROGRAM_PATH "${LY_ROOT_FOLDER}/python")

        # those using python should call it via LY_PYTHON_CMD - it adds the extra "-s" param
        # this param causes python to ignore the users profile folder which can have bogus
        # pip installation modules and lead to machine-specific config problems
        # it also uses our wrapper python, which can add additional paths to the python path
        if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
            set(LY_PYTHON_CMD "${LY_ROOT_FOLDER}/python/python.cmd" "-s")
        else()
            set(LY_PYTHON_CMD "${LY_ROOT_FOLDER}/python/python.sh" "-s")
        endif()

        update_pip_requirements(${LY_PYTHON_DEFAULT_REQUIREMENTS_TXT} default_requirements)

        # we also need to make sure any custom packages are installed.
        # this costs a moment of time though, so we'll only do it based on stamp files.
        if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND NOT INSTALLED_ENGINE)
            ly_pip_install_local_package_editable(${LY_ROOT_FOLDER}/Tools/LyTestTools ly-test-tools)
            ly_pip_install_local_package_editable(${LY_ROOT_FOLDER}/Tools/RemoteConsole/ly_remote_console ly-remote-console)
            ly_pip_install_local_package_editable(${LY_ROOT_FOLDER}/AutomatedTesting/Gem/PythonTests/EditorPythonTestTools editor-python-test-tools)
        endif()

        ly_pip_install_local_package_editable(${LY_ROOT_FOLDER}/scripts/o3de o3de)
    endif()
endif()

