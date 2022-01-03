#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard()

include(cmake/LySet.cmake)

# this script exists to make sure a python interpreter is immediately available
# it will both locate and run pip on python for our requirements.txt
# but you can also call update_pip_requirements(filename) at any time after.

# this is different from the usual package usage, because even if we are targetting
# android, for example, we may still be doing so on a windows HOST pc, and the
# python interpreter we want to use is for the windows HOST pc, not the PAL platform:

# CMAKE_HOST_SYSTEM_NAME is  "Windows", "Darwin", or "Linux" in our cases..
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux" )
    ly_set(LY_PYTHON_VERSION 3.7.10)
    ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.7)
    ly_set(LY_PYTHON_PACKAGE_NAME python-3.7.10-rev2-linux)
    ly_set(LY_PYTHON_PACKAGE_HASH 6b9cf455e6190ec38836194f4454bb9db6bfc6890b4baff185cc5520aa822f05)
elseif  (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin" )
    ly_set(LY_PYTHON_VERSION 3.7.10)
    ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.7)
    ly_set(LY_PYTHON_PACKAGE_NAME python-3.7.10-rev1-darwin)
    ly_set(LY_PYTHON_PACKAGE_HASH 3f65801894e4e44b5faa84dd85ef80ecd772dcf728cdd2d668a6e75978a32695)
elseif  (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows" )
    ly_set(LY_PYTHON_VERSION 3.7.10)
    ly_set(LY_PYTHON_VERSION_MAJOR_MINOR 3.7)
    ly_set(LY_PYTHON_PACKAGE_NAME python-3.7.10-rev2-windows)
    ly_set(LY_PYTHON_PACKAGE_HASH 06d97488a2dbabe832ecfa832a42d3e8a7163ba95e975f032727331b0f49d280)
endif()

# settings and globals
ly_set(LY_PYTHON_DEFAULT_REQUIREMENTS_TXT "${LY_ROOT_FOLDER}/python/requirements.txt")

include(cmake/3rdPartyPackages.cmake)

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
    set(stamp_file ${CMAKE_BINARY_DIR}/packages/requirements_files/${unique_name}.stamp)
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
        WORKING_DIRECTORY ${Python_BINFOLDER}
        RESULT_VARIABLE PIP_RESULT
        OUTPUT_VARIABLE PIP_OUT 
        ERROR_VARIABLE PIP_OUT
        )

    message(VERBOSE "pip result: ${PIP_RESULT}")
    message(VERBOSE "pip output: ${PIP_OUT}")

    if (NOT ${PIP_RESULT} EQUAL 0)
        message(CHECK_FAIL "Failed to fetch / update python dependencies: ${PIP_OUT} - use CMAKE_MESSAGE_LOG_LEVEL to VERBOSE for more information")
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
        WORKING_DIRECTORY ${Python_BINFOLDER} 
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
    set(stamp_file ${CMAKE_BINARY_DIR}/packages/pip_installs/${pip_package_name}.stamp)
    get_filename_component(stamp_file_directory ${stamp_file} DIRECTORY)
    file(MAKE_DIRECTORY ${stamp_file_directory})
   
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
        WORKING_DIRECTORY ${Python_BINFOLDER}
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
        WORKING_DIRECTORY ${Python_BINFOLDER}
        RESULT_VARIABLE PIP_RESULT
        OUTPUT_VARIABLE PIP_OUT 
        ERROR_VARIABLE PIP_OUT
        )

    message(VERBOSE "pip install result: ${PIP_RESULT}")
    message(VERBOSE "pip install output: ${PIP_OUT}")

    if (NOT ${PIP_RESULT} EQUAL 0)
        message(CHECK_FAIL "Failed to install ${package_folder_path}: ${PIP_OUT} - use CMAKE_MESSAGE_LOG_LEVEL to VERBOSE for more information")
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

set(temp_LY_PACKAGE_VALIDATE_PACKAGE ${LY_PACKAGE_VALIDATE_PACKAGE})
if (EXISTS  ${LY_ROOT_FOLDER}/python/runtime/${LY_PYTHON_PACKAGE_NAME})
    # we will not validate the hash of every file, just that it is present
    # this is not just an optimization, see comment above.
    set(LY_PACKAGE_VALIDATE_PACKAGE FALSE)
endif()

ly_associate_package(PACKAGE_NAME ${LY_PYTHON_PACKAGE_NAME} TARGETS "Python" PACKAGE_HASH ${LY_PYTHON_PACKAGE_HASH})
ly_set_package_download_location(${LY_PYTHON_PACKAGE_NAME} ${LY_ROOT_FOLDER}/python/runtime)
ly_download_associated_package(Python)

ly_set(LY_PACKAGE_VALIDATE_CONTENTS ${temp_LY_PACKAGE_VALIDATE_CONTENTS})
ly_set(LY_PACKAGE_VALIDATE_PACKAGE ${temp_LY_PACKAGE_VALIDATE_PACKAGE})

if (NOT CMAKE_SCRIPT_MODE_FILE)
    # if we're in script mode, we dont want to actually try to find package or anything else

    find_package(Python ${LY_PYTHON_VERSION} REQUIRED)

    # note - if you want to use a normal python via FindPython instead of the LY package above,
    # you may have to declare the below variables after find_package, as the project scripts are 
    # looking for the below variables specifically.

    # verify the required variables are present:
    if (NOT Python_EXECUTABLE OR NOT Python_HOME OR NOT Python_PATHS)
        message(SEND_ERROR "Python installation not valid expected to find all of the following variables set:")
        message(STATUS "    Python_EXECUTABLE:  ${Python_EXECUTABLE}")
        message(STATUS "    Python_HOME:        ${Python_HOME}")
        message(STATUS "    Python_PATHS:       ${Python_PATHS}")
    else()
        message(STATUS "Using Python ${Python_VERSION} at ${Python_EXECUTABLE}")
        message(VERBOSE "    Python_EXECUTABLE:  ${Python_EXECUTABLE}")
        message(VERBOSE "    Python_HOME:        ${Python_HOME}")
        message(VERBOSE "    Python_PATHS:       ${Python_PATHS}")

        get_filename_component(Python_BINFOLDER ${Python_EXECUTABLE} DIRECTORY)

        # make sure other utils can find python / pip / etc
        # using find_program:
        LIST(APPEND CMAKE_PROGRAM_PATH "${Python_BINFOLDER}")
        
        # some platforms have this scripts dir, its harmless to add for those that do not.
        LIST(APPEND CMAKE_PROGRAM_PATH "${Python_BINFOLDER}/Scripts")   

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

