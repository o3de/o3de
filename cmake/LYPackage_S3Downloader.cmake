#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

include_guard()

# You can download files from S3 if you use a url like s3:// instead of https://
# in order for this to work, 'aws' CLI must be on your command line PATH, or you
# can set LY_AWS_COMMAND to override the searching on your path and specify manually.
# Note:  it must be a real executable, not a script!
# see: https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html
set(LY_AWS_COMMAND "" CACHE FILEPATH 
    "Overrides the location of the 'aws' program, leave blank to search the path for 'aws'.  You can also use LY_AWS_COMMAND environment variable.")

# You must have the appropriate credentials for it to actually function
# see: https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-quickstart.html
# to configure which credentials to use, you can either set the machine environment var
# LY_AWS_PROFILE, which would only apply to this script,
# or, AWS_PROFILE, which would apply to other Amazon tools as well, or you can use the LY_AWS_PROFILE 
# CMake variable instead which would override the above two.
# Specifying no profile will cause it to run the 'aws' command with no profile argument.

# note that we leave it blank unless specified, so that the 'default' above doesn't actually
# save / overwrite the environment until the user explicitly sets it.
set(LY_AWS_PROFILE "" CACHE STRING 
        "Profile to use for 'aws' CLI.  Blank means do not use a profile.  You can also use LY_AWS_PROFILE or AWS_PROFILE environment variables.")

if (LY_AWS_PROFILE)
    ly_set(LY_PACKAGE_AWS_PROFILE_COMMAND "--profile" ${LY_AWS_PROFILE})
elseif (DEFINED ENV{LY_AWS_PROFILE}) # if the CMake var isn't set (from the above cache), try the environment.
    ly_set(LY_PACKAGE_AWS_PROFILE_COMMAND "--profile" $ENV{LY_AWS_PROFILE})
elseif (DEFINED ENV{AWS_PROFILE})
    ly_set(LY_PACKAGE_AWS_PROFILE_COMMAND "--profile" $ENV{AWS_PROFILE})
endif()

# not a cache variable, but a tweakable.  We can use really short-lived credentials
# and urls becuase we intend to presign and then immediately fetch.
ly_set(LY_PACKAGE_PRESIGN_URL_EXPIRATION_TIME 120)

# ly_is_s3_url
# if the given URL is a s3 url of thr form "s3://(stuff)" then sets
# the output_variable_name to TRUE otherwise unsets it.
function (ly_is_s3_url download_url output_variable_name)
    if ("${download_url}" MATCHES "s3://.*")
        set(${output_variable_name} TRUE PARENT_SCOPE)
    else()
        unset(${output_variable_name} PARENT_SCOPE)
    endif()
endfunction()

# values set on command line or in cache override env.
# however, we dont want to cache the path in case the env var changes:
if (NOT LY_AWS_COMMAND)
    if (DEFINED ENV{LY_AWS_COMMAND})
        ly_set(LY_AWS_COMMAND $ENV{LY_AWS_COMMAND})
    else()
        find_program(LY_AWS_COMMAND_FIND "aws")
        if (NOT "${LY_AWS_COMMAND_FIND}" STREQUAL "LY_AWS_COMMAND_FIND-NOTFOUND")
            ly_set(LY_AWS_COMMAND ${LY_AWS_COMMAND_FIND})
        endif()
        unset(LY_AWS_COMMAND_FIND CACHE)
    endif()
endif()

# ly_s3_download
# uses AWS CLI to pre-sign a s3://(bucket)/(object-key) url and turn it into a full https url.
# then downloads the file using that url.
# returns the download status object (which is a tuple of code;message) 
# just like file(DOWNLOAD ... does)
function(ly_s3_download download_url download_target output_variable_name)
    if (NOT LY_AWS_COMMAND)
        set(${output_variable_name} "1;Cannot find the 'aws' CLI installed, either configure its location in cmake GUI, set LY_AWS_COMMAND or add it to your PATH." PARENT_SCOPE)
        message(FATAL_ERROR "Unable to get a required package from an s3 bucket - Cannot find the 'aws' CLI installed, either configure its location in cmake GUI, set LY_AWS_COMMAND or add it to your PATH.")
        return()
    endif()
    set(COMMAND_TO_RUN ${LY_AWS_COMMAND} "s3" "presign" ${download_url} --expires-in ${LY_PACKAGE_PRESIGN_URL_EXPIRATION_TIME} ${LY_PACKAGE_AWS_PROFILE_COMMAND})
    execute_process(COMMAND ${COMMAND_TO_RUN}
        RESULT_VARIABLE PRESIGN_RESULT 
        ERROR_VARIABLE PRESIGN_ERRORS 
        OUTPUT_VARIABLE SIGNED_URL
        OUTPUT_STRIP_TRAILING_WHITESPACE  # the aws command can result in whitespace, trim it
        ERROR_STRIP_TRAILING_WHITESPACE
        )
    if (NOT ${PRESIGN_RESULT} EQUAL 0)
        # note that presign works even on non-existent URLS.  It does not check
        # to see if the file is actually present or whether you have permission to pre-sign.
        # So the cause for pre-signing to fail are generally to do with your configuration of the
        # 'aws' command itself being wrong (ie, it doesn't exist, its not a real executable, 
        # you specified a profile which does not exist, etc.)
        set(${output_variable_name} "1;Failed To use the AWS CLI to pre-sign: "
                                    "ERRORS: ${PRESIGN_ERRORS}\n" 
                                    "Returned for: ${download_url}\n"
                                    "Make sure the AWS CLI is v2, and aws is available on your PATH. "
                                    "Alternatively, set LY_AWS_COMMAND in cmake or your environment to "
                                    "point at a valid AWS CLI." PARENT_SCOPE)
        return()
    endif()
    file(DOWNLOAD ${SIGNED_URL} ${download_target} STATUS result_status TLS_VERIFY ON LOG log)
    set(${output_variable_name} ${result_status} ${log} PARENT_SCOPE)    
    
endfunction()
