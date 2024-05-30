#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
#     return()
# endif()


o3de_gem_setup("Perforce")

add_subdirectory(Code)

#o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
#set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)
#
#include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
#
#if(NOT PAL_TRAIT_BUILD_P4PLUGIN_SUPPORTED)
#
#if(PAL_TRAIT_BUILD_HOST_TOOLS)
#
#    ly_add_target(
#        NAME ${gem_name}.Editor GEM_MODULE
#        NAMESPACE Gem
#        FILES_CMAKE
#            perforce_editor_shared_files.cmake
#        INCLUDE_DIRECTORIES
#            PRIVATE
#                Source
#            PUBLIC
#                Include
#        BUILD_DEPENDENCIES
#            PRIVATE
#                3rdParty::Qt::Core
#                3rdParty::Qt::Widgets
#                AZ::AzCore
#                Legacy::CryCommon
#                Legacy::EditorLib
#    )
#
#    # Inject the gem name into the Module source file
#    ly_add_source_properties(
#        SOURCES
#            Source/${gem_name}EditorModule.cpp
#        PROPERTY COMPILE_DEFINITIONS
#            VALUES
#                O3DE_GEM_NAME=${gem_name}
#                O3DE_GEM_VERSION=${gem_version})
#
#    # use the above ${gem_name}.Editor module in dev tools:
#    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
#    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
#endif()
#
#endif()
#
#
## ly_add_target(
##     NAME PerforcePlugin MODULE
##     NAMESPACE Legacy
##     OUTPUT_SUBDIRECTORY EditorPlugins
##     AUTOMOC
##     AUTOUIC
##     FILES_CMAKE
##         perforceplugin_files.cmake
##     COMPILE_DEFINITIONS
##         PRIVATE
##             PLUGIN_EXPORTS
##     INCLUDE_DIRECTORIES
##         PUBLIC
##             .
##     BUILD_DEPENDENCIES
##         PRIVATE
##             3rdParty::Qt::Core
##             3rdParty::Qt::Widgets
##             AZ::AzCore
##             Legacy::CryCommon
##             Legacy::EditorLib
## )
#
## ly_add_dependencies(Editor PerforcePlugin)
## set_property(GLOBAL APPEND PROPERTY LY_EDITOR_PLUGINS $<TARGET_FILE_NAME:Legacy::PerforcePlugin>)
