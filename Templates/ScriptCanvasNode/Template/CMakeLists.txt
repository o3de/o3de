# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# Query the gem name from the gem.json file if possible
# otherwise fallback to using ${GemName}
o3de_find_ancestor_gem_root(gem_path gem_name "${CMAKE_CURRENT_SOURCE_DIR}")
if (NOT gem_name)
    set(gem_name "${GemName}")
endif()

ly_add_target(
    NAME ${Name}.Object OBJECT
    NAMESPACE ScriptCanvasNodes
    FILES_CMAKE
        ${Name}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptCanvas
            Gem::ScriptCanvas.Extensions
    AUTOGEN_RULES
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Header.jinja,$path/$fileprefix.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeable_Source.jinja,$path/$fileprefix.generated.cpp
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeableRegistry_Header.jinja,$path/$fileprefix_Nodeables.generated.h
        *.ScriptCanvasNodeable.xml,ScriptCanvasNodeableRegistry_Source.jinja,$path/$fileprefix_Nodeables.generated.cpp
)

set (dependent_targets ${gem_name}.Private.Object;${gem_name};${gem_name}.Editor)

foreach (dependent_target IN LISTS dependent_targets)
    if (TARGET ${dependent_target})
        target_link_libraries(${dependent_target} PRIVATE ScriptCanvasNodes::${Name}.Object)
    else()
        message(FATAL_ERROR "The target ${dependent_target} was not found, if the gem was not created using scripts/o3de create-gem"
            " you may update dependent_targets to match the corresponding targets in your gem")
    endif()
endforeach()
