#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ResourceCompilerXML MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY rc_plugins
    FILES_CMAKE
        resourcecompilerxml_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            PCH
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            Legacy::ResourceCompiler.Static
            Legacy::CryCommonTools
            Legacy::CryXML
            Legacy::CrySystem.XMLBinary
)

ly_add_dependencies(RC Legacy::ResourceCompilerXML)
