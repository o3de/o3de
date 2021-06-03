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
    NAME DccScriptingInterface.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        dccscriptinginterface_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
)

ly_add_target(
    NAME DccScriptingInterface.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        dccscriptinginterface_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::DccScriptingInterface.Static
)

# Any 'tool' type applications should use Gem::DccScriptingInterface.Editor:
ly_create_alias(NAME DccScriptingInterface.Tools    NAMESPACE Gem TARGETS Gem::DccScriptingInterface.Editor)
# Add an empty 'builders' alias to allow the DccScriptInterface root gem path to be added to the generated
# cmake_dependencies.<project>.assetprocessor.setreg to allow the asset scan folder for it to be added
ly_create_alias(NAME DccScriptingInterface.Builders NAMESPACE Gem)
