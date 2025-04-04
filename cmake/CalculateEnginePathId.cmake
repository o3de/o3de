#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This script will generate a unique ID of the current engine path by using the first 9 characters of the SHA1 hash of the absolute engine path
# and write the results to STDOUT


if(${CMAKE_ARGC} LESS 3)
    message(FATAL_ERROR "Missing required engine path argument to calculate id from.")
endif()

# Get and normalize the path passed into this script as the main argument
set(PATH_TO_HASH ${CMAKE_ARGV3})
cmake_path(NORMAL_PATH PATH_TO_HASH)

# Sanity check to make sure this is the path to the engine
set(ENGINE_SANITY_CHECK_FILE "${PATH_TO_HASH}/engine.json")
if (NOT EXISTS "${ENGINE_SANITY_CHECK_FILE}")
    message(FATAL_ERROR "Path ${PATH_TO_HASH} is not a valid engine path.")
endif()

string(TOLOWER ${PATH_TO_HASH} PATH_TO_HASH)

# Calculate the path id based on the first 8 characters of the SHA1 hash of the normalized path
string(SHA1 ENGINE_SOURCE_PATH_HASH "${PATH_TO_HASH}")
string(SUBSTRING ${ENGINE_SOURCE_PATH_HASH} 0 8 ENGINE_SOURCE_PATH_ID)

# Note: using 'message(STATUS ..' will print to STDOUT, but will always include a double hyphen '--'. Instead we will 
# use the cmake echo command directly to do this
execute_process(COMMAND ${CMAKE_COMMAND} -E echo ${ENGINE_SOURCE_PATH_ID})
