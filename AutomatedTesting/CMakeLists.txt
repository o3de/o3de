#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.22)
    include(cmake/EngineFinder.cmake OPTIONAL)
    find_package(o3de REQUIRED)
    project(AutomatedTesting
        LANGUAGES C CXX
        VERSION 1.0.0.0
    )
    o3de_initialize()
endif()