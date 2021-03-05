#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

include_guard()

# OVERVIEW:
# this is the Lumberyard Package system.
# It allows you to host a package on a server and download it as needed when a target
# requests that specific package, or manually whenever you want to do so.
# Most users will just call ly_associate_package(...) to associate a package with a target
# and the system will automatically get that package when a target asks for it as a 
# dependency.  If you want to manually activate a package, you can use ly_download_associated_package
# to bring its find* scripts into scope (and download if needs be)
# and you can also use ly_set_package_download_location(..) to change it to install
# somewhere besides the default install location.

# cache variables:

# LY_PACKAGE_SERVER_URLS:
# missing packages are downloaded from this list of URLS, first one to match wins.
# Besides normal http and https URLs, you can also use FILE urls,
# for example "file:///mnt/d/lyengine/packageSource/packages;file:///d:/lyengine/packageSource/packages"
# also allowed:
# "s3://bucketname" (it will use LYPackage_S3Downloader.cmake to download it from a s3 bucket)

set(LY_PACKAGE_SERVER_URLS "" CACHE STRING "Server URLS to fetch packages from")
# Note: if you define the "LY_PACKAGE_SERVER_URLS" environment variable
# it will be added to this value in the front, so that users can set
# an env var and use that as an "additional" set of servers beyond the default set.
if (DEFINED ENV{LY_PACKAGE_SERVER_URLS})
    set(LY_PACKAGE_SERVER_URLS $ENV{LY_PACKAGE_SERVER_URLS} ${LY_PACKAGE_SERVER_URLS})
endif()

# If you keep packages after downloading, then they can be moved to a network share
# or checked into source control so that others on the same project can avoid re-downloading
set(LY_PACKAGE_KEEP_AFTER_DOWNLOADING TRUE CACHE BOOL "If enabled, packages will be kept after downloading them for later re-use")
set(LY_PACKAGE_DOWNLOAD_CACHE_LOCATION ${LY_3RDPARTY_PATH}/downloaded_packages CACHE PATH "You can make it store the packages in a folder of your choosing")

# LY_PACKAGE_UNPACK_LOCATION - you can change this to any path reachable.
set(LY_PACKAGE_UNPACK_LOCATION ${LY_3RDPARTY_PATH}/packages CACHE PATH "Location to unpack downloaded packages to")

# note that sometimes the user configures first without populating LY_3RDPARTY_PATH
# in that case, we'll try overwriting the cache value, only if it is blank:
if (NOT LY_PACKAGE_UNPACK_LOCATION)
    set(LY_PACKAGE_UNPACK_LOCATION ${LY_3RDPARTY_PATH}/packages CACHE PATH "Location to unpack downloaded packages to" FORCE )
endif()

# while developing you can set one or both to true to force auto downloads from your local cache
set(LY_PACKAGE_VALIDATE_CONTENTS FALSE CACHE BOOL "If enabled, will fully validate every file in every package")
set(LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER FALSE CACHE BOOL "If enabled, will fetch the manifest from the server every time instead of using local cache when found")

# you can also enable verbose/debug logging from the package system.
set(LY_PACKAGE_DEBUG FALSE CACHE BOOL "If enabled, will output detailed information during package operations" )

# ---- below this line, no cache variables or tweakables ---------

set(LY_PACKAGE_EXT              ".tar.xz")
set(LY_PACKAGE_HASH_EXT         ".tar.xz.SHA256SUMS")
set(LY_PACKAGE_CONTENT_HASH_EXT ".tar.xz.content.SHA256SUMS")

# accounts for it being undefined or blank
if ("${LY_PACKAGE_DOWNLOAD_CACHE_LOCATION}" STREQUAL "")
    message(FATAL_ERROR "ly_package: LY_PACKAGE_DOWNLOAD_CACHE_LOCATION must be defined.")
    return()
endif()

# used to send messages and hide them unless the LY_PACKAGE_DEBUG var is true
macro(ly_package_message)
    if (LY_PACKAGE_DEBUG)
        message(${ARGN})
    endif()
endmacro()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/packages)

include(cmake/LYPackage_S3Downloader.cmake)

# Downloads a file.
# currently works on file://, s3://, ftp://, http://, https:// ... and whatever
# else the file(DOWNLOAD ..) function supports.
# if you add another downloader, see that it returns a list, where the first element
# in the list is the error code, and then the rest of the list is the error(s), just like
# the file(DOWNLOAD ... ) function does.
function(download_file full_file_url download_target_file results_array)
    set(${results_array} "-1;unknown_error" PARENT_SCOPE)
    ly_is_s3_url(${server_url} result_is_s3_bucket)
    if (result_is_s3_bucket)
        ly_s3_download("${download_url}" ${download_target_file} results)
    else()
        file(DOWNLOAD "${download_url}" ${download_target_file} STATUS results TLS_VERIFY ON LOG log)
        list(APPEND results ${log})
    endif()

    list(GET results 0 code_returned)
    if(code_returned) 
        # non zero means it failed to download
        # however, cmake will leave the file open, and zero bytes.
        file(REMOVE ${download_target_file})

        # parse the error.  If its 22, it means it was an HTTP error from curl.
        # we'll go to the bother of parsing the error and replacing it with the actual error code.
        # since the curl error is pointless.
        # note that the following code is similar to the code that's build into CMake
        # in its ExternalData.cmake file that ships.
        if (${code_returned} EQUAL 22)
            # extract the http response code if possible!
            string(REGEX MATCH "The requested URL returned error\\:([^\n]*)[\n]" found_string "${results}")
            if (found_string)
                # message the log before you replace it for debugging
                ly_package_message("${results}")
                # replace it.
                set(results ${code_returned} ${found_string})
            endif()
        endif()
    endif()


    set(${results_array} ${results} PARENT_SCOPE)
endfunction()


#! ly_package_find_package_server - note, the list of 3rd party urls is a list!
# given a package name, finds the first server in the list that has the hash file
# for that package, downloads it to ${LY_PACKAGE_DOWNLOAD_CACHE_LOCATION}/${package_name}.tgz.SHA256SUMS
#  - sets the url_variable to be the server URL that it found it at
function(ly_package_find_package_server package_name url_variable)
    unset(${url_variable} PARENT_SCOPE)

    unset(error_messages)
    
    # to avoid spamming with useless repeated warnings, we save
    # a global that indicates we already failed to find it
    get_property(already_failed_this_package GLOBAL PROPERTY LY_${package_name}_NO_SERVERS SET)
    if (already_failed_this_package)
        return() # no point in checking again
    endif()
    if (NOT LY_PACKAGE_SERVER_URLS)
        message(SEND_ERROR "ly_package:     - LY_PACKAGE_SERVER_URLS is empty, cannot download packages.  enable LY_PACKAGE_DEBUG for details")
        return()
    endif()
    foreach(server_url ${LY_PACKAGE_SERVER_URLS})
        set(download_url ${server_url}/${package_name}${LY_PACKAGE_HASH_EXT})
        set(download_target ${LY_PACKAGE_DOWNLOAD_CACHE_LOCATION}/${package_name}${LY_PACKAGE_HASH_EXT})
        if (EXISTS ${download_target})
            file(REMOVE ${download_target})
        endif()
        ly_package_message(STATUS "ly_package: trying to download ${download_url} to ${download_target}")
        
        download_file(${download_url} ${download_target} results)
        list(GET results 0 status_code)
        
        if (${status_code} EQUAL 0 AND EXISTS ${download_target})
            set(${url_variable} ${server_url} PARENT_SCOPE)
            ly_package_message(STATUS "ly_package:     - Selecting server ${server_url} for package ${package_name}")
            return()
        else()
            # remove the status code and treat the rest of the list as the error.
            list(REMOVE_AT results 0)
            set(current_error_message "Error from server ${server_url} - ${status_code} - ${results}")
            list(APPEND error_messages "${current_error_message}")

            if (EXISTS ${download_target})
                # we can't keep the file, sometimes it makes a zero-byte file!
                file(REMOVE ${download_target})
            endif()
            ly_package_message(${current_error_message})
        endif()
    endforeach()
    message(SEND_ERROR "ly_package:     - Unable to find package ${package_name} on any download server.  Enable LY_PACKAGE_DEBUG to debug")
    # only output errors if all servers failed and we found it nowhere:
    foreach(error_message ${error_messages})
        message(STATUS "${error_message}")
    endforeach()

    set_property(GLOBAL PROPERTY LY_${package_name}_NO_SERVERS TRUE)
endfunction()

# parse_sha256sums_line  
# given a line of text that is in the SHA256SUMS format, digest it and output it
# as a pair of variables of your choice.
# will set output_hash to be the hex digest string
# will set output_file_name to be the filename that must hash to that value
# will unset them if an error occurs.
function(parse_sha256sums_line input_hash_line output_hash output_file_name) 
    # the official SHA256SUMS format is actually
    # the hash, followed by exactly one space character, followed
    # by either another space or an asterisk (indicating text or binary, space is text)
    # followed by the name of the file for the rest of the line (may contain more spaces)
    # the actual GNU implementations always binary hash, and so will we, so we'll ignore
    # the hash type character (text or binary)
    # this is why we capture three groups (hash, type, filename)
    # but only output 2 groups (1 and 3), hash and filename
    set(REGEX_EXPRESSION "^([A-Za-z0-9]*) ( |\\*)(.*)$")
    string(REGEX REPLACE "${REGEX_EXPRESSION}" "\\1;\\3" temp_list "${input_hash_line}")  
    # hash list is now: expected_file_hash;file_name"
    
    list(LENGTH temp_list hash_length)
    if (NOT ${hash_length} EQUAL 2)
        unset(${output_hash} PARENT_SCOPE)
        unset(${output_file_name} PARENT_SCOPE)
    else()
        list(GET temp_list 0 temp_hash)
        list(GET temp_list 1 temp_filename)
        set(${output_hash} ${temp_hash} PARENT_SCOPE)
        set(${output_file_name} ${temp_filename} PARENT_SCOPE)
    endif()
endfunction()

# ly_validate_sha256sums_file -- internal function
# given the path to a SHA256SUMS file and a working directory,
# verifies the hashes based on the current settings.
# --- sets HASH_WAS_VALID TRUE on parent scope if valid, FALSE otherwise.
# Note that it does not currently check if extra files are present, only that
# each file that is supposed to be there, is there and has the right hash.
function(ly_validate_sha256sums_file working_directory path_to_sha256sums_file)
    set(HASH_WAS_VALID FALSE PARENT_SCOPE)

    if (NOT EXISTS ${path_to_sha256sums_file})
        ly_package_message(STATUS "ly_package: Could not find SHA256SUMS file: ${path_to_sha256sums_file}")
        return()
    endif()

    # lines in a SHA256SUMS file take the form of "hash *filename"
    # that is, the actual 256hash in hex decimals, whitespace, then an asterisk
    # and then the file name that must have that hash.
    set(ANY_HASH_MISMATCHES FALSE)

    # we only try to do any kind of hashing if the VALIDATE_CONTENTS flag is on
    # otherwise we just check for the presence of required files.

    file(STRINGS ${path_to_sha256sums_file} hash_data ENCODING UTF-8)
    foreach(hash_line IN ITEMS ${hash_data})
        parse_sha256sums_line("${hash_line}" expected_file_hash file_name)

        if (NOT expected_file_hash OR NOT file_name)
            message(SEND_ERROR "ly_package: Invalid format SHA256SUMS file: ${path_to_sha256sums_file} line ${hash_line} - cannot verify hashes.  Enable LY_PACKAGE_DEBUG to debug.")
            return()
        endif() 

        if (EXISTS ${working_directory}/${file_name})
            if (LY_PACKAGE_VALIDATE_CONTENTS)
                file(SHA256 ${working_directory}/${file_name} existing_hash)
                if (NOT "${existing_hash}" STREQUAL "${expected_file_hash}" )
                    ly_package_message(STATUS "ly_package: File hash mismatch: ${working_directory}/${file_name}")
                    set(ANY_HASH_MISMATCHES TRUE)
                else()
                    ly_package_message(STATUS "ly_package: File hash matches: ${expected_file_hash} - ${file_name}")
                endif()
            endif()
        else()
            ly_package_message(STATUS "ly_package: Expected file was not found: ${working_directory}/${file_name}")
            set(ANY_HASH_MISMATCHES TRUE)
        endif()
    endforeach()

    if (${ANY_HASH_MISMATCHES})
        ly_package_message(STATUS "ly_package: Validation failed - files were missing, or had hash mismatches.")
    else()
        set(HASH_WAS_VALID TRUE PARENT_SCOPE)
    endif()
endfunction()

function(ly_package_get_target_folder package_name output_variable_name)
    # is it grafted onto the tree elsewhere?
    get_property(overridden_location GLOBAL PROPERTY LY_PACKAGE_DOWNLOAD_LOCATION_${package_name})
    if (overridden_location)
        set(${output_variable_name} ${overridden_location} PARENT_SCOPE)
    elseif(NOT "${LY_PACKAGE_UNPACK_LOCATION}" STREQUAL "")
        set(${output_variable_name} ${LY_PACKAGE_UNPACK_LOCATION} PARENT_SCOPE)
    else()
        message(WARNING "ly_package: Could not locate the LY_PACKAGE_UNPACK_LOCATION variable"
                    "'${LY_PACKAGE_UNPACK_LOCATION}' please fill it in!"
                    " To compensate, this script will unpack into the build folder")
        set(${output_variable_name} ${CMAKE_BINARY_DIR})
    endif()
endfunction()

#! given the name of a package, validate that all files are present and match as appropriate
function(ly_validate_package package_name)
    ly_package_message(STATUS "ly_package: Validating ${package_name}...")
    unset(${package_name}_VALIDATED PARENT_SCOPE)

    ly_package_get_target_folder(${package_name} DOWNLOAD_LOCATION)

    if (NOT EXISTS "${DOWNLOAD_LOCATION}/${package_name}")
        ly_package_message(STATUS "ly_package:     - ${package_name} is missing from ${DOWNLOAD_LOCATION}")
        return()
    endif()

    if (${LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER})
        # replace the existing SHASUMS file with the one from the server before validating.
        # note that there's no real point in validating the SHASUMS of the SHASUMS file
        # as this would then need a SHASUMS file to validate that, ad infinitum.  Instead,
        # we ensure that we dont' download something bogus by verifying the hash of the actual
        # package itself, as dicated in the actual download command.
        ly_package_message(STATUS "ly_package:     - re-downloading manifest from server because LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER is ${LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER}") 
        ly_package_find_package_server(${package_name} server_url)
        if (NOT server_url)
            return() # an error would have already been emitted by the find package function
        endif()

        set(download_url ${server_url}/${package_name}${LY_PACKAGE_CONTENT_HASH_EXT})
        set(temp_download_target ${DOWNLOAD_LOCATION}/${package_name}/SHA256SUMS)
        download_file(${download_url} ${temp_download_target} results)
        list(GET results 0 status_code)
        if (NOT ${status_code} EQUAL 0)
            return() # an error message would have been emitted from the download_file function
        endif() 
    endif()

    set(hash_file_name ${DOWNLOAD_LOCATION}/${package_name}/SHA256SUMS)
    set(json_file_name ${DOWNLOAD_LOCATION}/${package_name}/PackageInfo.json)
    if (NOT EXISTS ${hash_file_name})
        ly_package_message(STATUS "Hash file missing from package: ${hash_file_name}")
        return()
    endif()
    if (NOT EXISTS ${json_file_name})
        ly_package_message(STATUS "Package info file missing from package: ${json_file_name}")
        return()
    endif()

    set(package_stamp_file_name ${DOWNLOAD_LOCATION}/${package_name}.stamp)
    if (NOT EXISTS ${package_stamp_file_name})
        # This can happen because of a previous version not making these stamp files in the correct place.
        # In order to avoid re-downloading the package we react to a missing stamp file by creating a new one.
        # This will cause any logic that wants to do things if the package is 'newer' to re-run, which is
        # safer than the alternative of not running things that do need to run when packages are downloaded.
        ly_package_message(STATUS "ly_package: Stamp file was missing, restoring: ${package_stamp_file_name}")
        file(TOUCH ${package_stamp_file_name})
    endif()

    if (NOT LY_PACKAGE_VALIDATE_CONTENTS)
        ly_package_message(STATUS "Basic validation checks performed only becuase LY_PACKAGE_VALIDATE_CONTENTS is not enabled.")
        set(${package_name}_VALIDATED TRUE PARENT_SCOPE)
        ly_package_message(STATUS "ly_package: Validated ${package_name} - Basic Validation OK")
        return()
    endif()

    ly_validate_sha256sums_file(
            ${DOWNLOAD_LOCATION}/${package_name} 
            ${DOWNLOAD_LOCATION}/${package_name}/SHA256SUMS)
    
    if (HASH_WAS_VALID)
        set(${package_name}_VALIDATED TRUE PARENT_SCOPE)
        ly_package_message(STATUS "ly_package: Validated ${package_name} - Full Validation OK")
    endif()
endfunction()

# ly_force_download_package 
# forces the download of a third party library regardless of current situation
# package_name is like 'zlib-1.2.8-platform', not a file name or URL.
# ---> Sets ${package_name}_VALIDATED on parent scope.  TRUE only if the package
# was successfully validated, including hash of contents.

function(ly_force_download_package package_name)
    unset(${package_name}_FOUND PARENT_SCOPE)
    unset(${package_name}_VALIDATED PARENT_SCOPE)

    ly_package_get_target_folder(${package_name} DOWNLOAD_LOCATION)

    # this function contains a REMOVE_RECURSE.  Because of that, we're going to do extra
    # validation on the inputs.
    # its not good enough for the variable to just exist but be empty, so we build strings
    if ("${package_name}" STREQUAL "" OR "${DOWNLOAD_LOCATION}" STREQUAL "")
        message(FATAL_ERROR "ly_package: ly_force_download_package called with invalid params!  Enable LY_PACKAGE_DEBUG to debug.")
        return()
    endif()

    set(temp_download_target           ${LY_PACKAGE_DOWNLOAD_CACHE_LOCATION}/${package_name}${LY_PACKAGE_EXT})
    set(hashfile_temp_download_target  ${LY_PACKAGE_DOWNLOAD_CACHE_LOCATION}/${package_name}${LY_PACKAGE_HASH_EXT})
    set(final_folder ${DOWNLOAD_LOCATION}/${package_name})
        
    # we print this message unconditionally because downloading a package
    # can take time and we only get here if its missing in the first place, so 
    # this should happen once on the very first configure
    message(STATUS "Downloading package to ${final_folder}")
    
    # if you have a package download cache, we will prefer to use that
    # unless you have set LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER to be TRUE
    # if you have that set to FALSE and the package AND its hash file are already present
    # we will not re-download.

    # download it into the third party folder from the established source server
    # if we didn't find it, we will download and overwrite.

    if (NOT LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER AND EXISTS ${hashfile_temp_download_target})
        # the hash file already exists at that location.  We won't try to download it.
        ly_package_message(STATUS "ly_package:     - Found cached package hash at ${hashfile_temp_download_target} and LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER is not set, so reusing it")
        set(used_local_hash_file TRUE)
    else()
        # this both finds a server and downloads the hash.
        unset(used_local_hash_file)
        
        if (LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER)
            ly_package_message(STATUS "ly_package:     - Downloading package hash for ${package_name} from server (LY_PACKAGE_VALIDATE_CONTENTS_FROM_SERVER is set)")
        endif()

        ly_package_find_package_server(${package_name} server_url)
        if(NOT server_url)
            return()
        endif()
    endif()

    file(STRINGS ${hashfile_temp_download_target} hash_data ENCODING UTF-8 LIMIT_COUNT 1)
    parse_sha256sums_line("${hash_data}" expected_package_hash expected_package_name)
    
    # we also verify that the package that the hash file refers to is the right one by checking
    # the name.
    if (NOT expected_package_hash OR NOT expected_package_hash OR NOT "${package_name}${LY_PACKAGE_EXT}" STREQUAL "${expected_package_name}")
        message(SEND_ERROR "ly_package: The server had invalid file hash data for package ${package_name} - hash data is '${hash_data}' - cannot trust the download.  Use LY_PACKAGE_DEBUG to debug")
        return()
    endif() 

    # can we reuse a file that we downloaded last time?
    if (EXISTS ${temp_download_target})
        file(SHA256 ${temp_download_target} existing_hash)
    endif()

    if (NOT "${existing_hash}" STREQUAL "${expected_package_hash}" )
        file(REMOVE ${temp_download_target})

        # if we're going to download the package from the server
        # we may have used a cached file above.  If thats the case, we need to
        # set the server url and recalculate the hash
        if (NOT server_url)
            ly_package_find_package_server(${package_name} server_url)
            if(NOT server_url)
                return()
            endif()

            file(STRINGS ${hashfile_temp_download_target} hash_data ENCODING UTF-8 LIMIT_COUNT 1)
            parse_sha256sums_line(${hash_data} expected_package_hash expected_package_name)
        endif()

        set(download_url ${server_url}/${package_name}${LY_PACKAGE_EXT})
        set(hashfile_download_url ${download_url}${LY_PACKAGE_HASH_EXT})
        ly_package_message(STATUS "ly_package:     - downloading package '${download_url}' to '${final_folder}'")

        download_file(${download_url} ${temp_download_target} results)

        list(GET results 0 status_code)
        # ensure the downloaded file actually has the correct hash:
        if (${status_code} EQUAL 0 AND EXISTS ${temp_download_target})
            file(SHA256 ${temp_download_target} existing_hash)
            if (NOT "${existing_hash}" STREQUAL "${expected_package_hash}" )
                message(SEND_ERROR "ly_package:    - package downloaded from server had the wrong hash!  Cannot trust it.  Enable LY_PACKAGE_DEBUG to debug.")
                return()
            endif()
        endif()

        # the above call actually aborts this function if it fails, so its not necessary to check here.
    else()
        ly_package_message(STATUS "ly_package:     - package already correct hash ${temp_download_target}, re-using")
    endif()

    if (EXISTS ${DOWNLOAD_LOCATION}/${package_name})
        ly_package_message(STATUS "ly_package:     - removing folder ${DOWNLOAD_LOCATION}/${package_name} to replace it...")
        file(REMOVE_RECURSE ${DOWNLOAD_LOCATION}/${package_name})

        if (EXISTS ${DOWNLOAD_LOCATION}/${package_name})
            message(SEND_ERROR "ly_package:     -folder ${DOWNLOAD_LOCATION}/${package_name} could not be removed.  Check if some program has it open (VSCode, VS, Terminal windows, ...).   Enable LY_PACKAGE_DEBUG to debug.")
            return()
        endif()
    endif()
    
    file(MAKE_DIRECTORY ${DOWNLOAD_LOCATION}/${package_name})
    
    ly_package_message(STATUS "ly_package:    - unpacking package...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${temp_download_target} 
         WORKING_DIRECTORY ${final_folder} COMMAND_ECHO STDOUT OUTPUT_VARIABLE unpack_result)

    if (NOT ${unpack_result} EQUAL 0)
        message(SEND_ERROR "ly_package: required package {package_name} could not be unpacked.  Compile may fail!  Enable LY_PACKAGE_DEBUG to debug.")
        return()
    else()
        if (NOT LY_PACKAGE_KEEP_AFTER_DOWNLOADING)
            ly_package_message(STATUS "ly_package: Removing package after unpacking (LY_PACKAGE_KEEP_AFTER_DOWNLOADING is ${LY_PACKAGE_KEEP_AFTER_DOWNLOADING})")
            file(REMOVE ${temp_download_target})
            file(REMOVE "${temp_download_target}.SHA256SUMS")
        endif()
    endif()
    
    # because we just downloaded this file, we are going to force full hashing validation.
    # future runs will use the setting or default, which is a quicker validation
    set(LY_PACKAGE_VALIDATE_CONTENTS_old ${LY_PACKAGE_VALIDATE_CONTENTS})
    set(LY_PACKAGE_VALIDATE_CONTENTS TRUE)
    ly_validate_package(${package_name})
    set(LY_PACKAGE_VALIDATE_CONTENTS ${LY_PACKAGE_VALIDATE_CONTENTS_old})
    set(${package_name}_VALIDATED ${package_name}_VALIDATED PARENT_SCOPE)
    if (${package_name}_VALIDATED)
        # This we intentionally print out for each package that was actually downloaded from the internet, one time only:
        message(STATUS "Installed And Validated package at ${final_folder} - OK")
        # we also record a stamp file of when we did this, for use in other computations
        set(package_stamp_file_name ${DOWNLOAD_LOCATION}/${package_name}.stamp)
        file(TOUCH ${package_stamp_file_name})
    endif()
endfunction()

#! ly_enable_package:  low-level function - adds a package to the auto package download system
# Calling this will immediately make sure the package is present locally
# and will add the local cache path to the additional module path (CMAKE_MODULE_PATH)
# so that findxxxxx works
# note that package_name here is the actual package name, not the association name.
function(ly_enable_package package_name)
    # you can call this function as many times as you want, it will only try to validate the property once.
    
    # is it grafted onto the tree elsewhere?
    ly_package_get_target_folder(${package_name} DOWNLOAD_LOCATION)

    # add it to the prefixes so that we search here first
    # we add it in front so it can override any later paths, so "last one to declare" wins
    if (NOT ${DOWNLOAD_LOCATION}/${package_name} IN_LIST CMAKE_MODULE_PATH)
        set(CMAKE_MODULE_PATH ${DOWNLOAD_LOCATION}/${package_name} ${CMAKE_MODULE_PATH} PARENT_SCOPE)
    endif()

    get_property(existing_state GLOBAL PROPERTY LY_${package_name}_VALIDATED SET)

    if(NOT ${existing_state}) # note - check is for whether its SET, not whether its TRUE
        # if we get here, its not SET, so set it to FALSE pre-emptively so that
        # we don't try to download over and over, if the attempt to download fails.
        set_property(GLOBAL PROPERTY LY_${package_name}_VALIDATED FALSE)

        ly_validate_package(${package_name}) # sets VALIDATED in this scope.
        if (NOT ${package_name}_VALIDATED)
            # this will also validate it and set VALIDATED in this scope
            ly_force_download_package(${package_name})
        endif()
        
        if(NOT ${package_name}_VALIDATED)
            message(FATAL_ERROR "ly_package: Package ${package_name} could not be found or downloaded.  Expect build errors.  Enable LY_PACKAGE_DEBUG to debug.")
        else()
            set_property(GLOBAL PROPERTY LY_${package_name}_VALIDATED TRUE)
            # this message is unconditional as it will help prove that the package even was
            # attempted to be mounted using our package system.  In the absence of this message
            # its going to be difficult to know why a package is missing in the logs
            # its also consistent with cmake's other messages, like
            # 'using Windows Target SDK xxxxx' or 'using clang  xyz'
            message(STATUS "Using package ${DOWNLOAD_LOCATION}/${package_name}")
            # if the package goes missing, we will reconfigure:
            # note that this is already likely the case, if you ever use find_package, but this can help when
            # the package contains nothing cmake-related, for example, its just extra tools or assets or 
            # similar.  Packages are required to have PackageInfo.json at the root at minimum.
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOWNLOAD_LOCATION}/${package_name}/PackageInfo.json)
        endif()
    endif()        
endfunction()


#! ly_associate_package - Main public function
# - allows you to bind a package name like with any
# number of find-package names that will be used to find them later.
# this allows you to for example assocate 'zlib-1.2.8-multiplatform' with 'zlib'
# and will cause the 3rd party system to go fetch that library and ensure its present
# before trying to call find_package(zlib)
macro(ly_associate_package package_name)
    foreach(find_package_name ${ARGN})
        set_property(GLOBAL PROPERTY LY_PACKAGE_ASSOCIATION_${find_package_name} ${package_name})
    endforeach()
endmacro()

#!  Given a package find_package name (eg, 'zlib' not the actual package name)
# will set output_variable to the package id iff the package has a package
# association declared, otherwise will unset it.
macro(ly_get_package_association find_package_name output_variable)
    unset(${output_variable})
    get_property(is_associated GLOBAL PROPERTY LY_PACKAGE_ASSOCIATION_${find_package_name})
    if (is_associated)
        set(${output_variable} ${is_associated})
    endif()
endmacro()

# ly_set_package_download_location - OPTIONAL.
# by default, packages are downloaded to the package root
# at LY_PACKAGE_UNPACK_LOCATION - but if a package needs to be placed
# elsewhere, use this.
# note that package_name is expected to be the actual package name, not 
# the find_package(...)  name!
macro(ly_set_package_download_location package_name download_location)
    set_property(GLOBAL PROPERTY LY_PACKAGE_DOWNLOAD_LOCATION_${package_name} ${download_location})
endmacro()

# ly_download_associated_package  - main public function
# this just checks to see if the find_library_name (like 'zlib', not a package name)
# is associated with a package, as above.  If it is, it makes sure that the package
# is brought into scope (and if necessary, downloaded.)
macro(ly_download_associated_package find_library_name)
    ly_get_package_association(${find_library_name} package_name)
    if (package_name)
        # it is an associated package.
        ly_enable_package(${package_name})
    endif()
endmacro()

# ly_package_is_newer_than(package_name reference output_variable)
# will set output_variable to TRUE if and only if the package was downloaded
# more recently than the reference file's timestamp.
function(ly_package_is_newer_than package_name reference_file output_variable)
    unset(${output_variable} PARENT_SCOPE)
    ly_package_get_target_folder(${package_name} DOWNLOAD_LOCATION)
    set(package_stamp_file_name ${DOWNLOAD_LOCATION}/${package_name}.stamp)
    if (EXISTS ${package_stamp_file_name} AND ${package_stamp_file_name} IS_NEWER_THAN ${reference_file})
        set(${output_variable} TRUE PARENT_SCOPE)
    endif()
endfunction()

# if we're in script mode, we dont want to declare package associations
if (NOT CMAKE_SCRIPT_MODE_FILE)
    # include the built in 3rd party packages that are for every platform.
    # you can put your package associations anywhere, but this provides
    # a good starting point.
    include(cmake/3rdParty/BuiltInPackages.cmake)
endif()
