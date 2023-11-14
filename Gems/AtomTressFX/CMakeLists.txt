#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Query the gem name from the gem.json file if possible
# otherwise fallback to using AtomTressFX
o3de_find_ancestor_gem_root(gem_path gem_name "${CMAKE_CURRENT_SOURCE_DIR}")
if (NOT gem_name)
    set(gem_name "AtomTressFX")
endif()

# Fallback to using the current source CMakeLists.txt directory as the gem root path
if (NOT gem_path)
    set(gem_path ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(gem_json ${gem_path}/gem.json)

# Read the version field from the gem.json
set(gem_version, "0.0.0")
o3de_read_json_key(gem_version ${gem_json} "version")

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    # Declare ATOMTRESSFX_EDITOR only when tools pipeline is built
    # This is in order to allow AzToolsFramework to properly link across
    # platforms that support editor mode, while in the rest,
    # this module is not included to avoid compiling the editor components.
    set_source_files_properties(
        Code/HairModule.cpp
        Code/Components/EditorHairComponent.h
        Code/Components/EditorHairComponent.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                ATOMTRESSFX_EDITOR
    )

    ly_add_target(
        NAME ${gem_name}.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            Hair_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::LmbrCentral
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::CommonFeaturesAtom.Static
                Gem::EMotionFXStaticLib
    )

    ly_add_target(
        NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            Hair_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code/AssetPipeline
                Code
                External
                External/Code/src
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::Atom_Utils.Static
                Gem::EMotionFXStaticLib
                Gem::${gem_name}.Static
    )

else()

    ly_add_target(
        NAME ${gem_name}.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            Hair_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::LmbrCentral
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::CommonFeaturesAtom.Static
                Gem::EMotionFXStaticLib
    )

    ly_add_target(
        NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            Hair_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code/AssetPipeline
                Code
                External
                External/Code/src
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::Atom_Utils.Static
                Gem::EMotionFXStaticLib
                Gem::${gem_name}.Static
    )
endif()

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Code/HairModule.h
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Clients and servers use the ${gem_name} module
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME ${gem_name}.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            Hair_builders_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AssetBuilderSDK
                Gem::${gem_name}.Static
    )

    ly_add_target(
        NAME ${gem_name}.Builders GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            Hair_builders_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Code
                External
                External/Code/src
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Builders.Static
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Code/Builders/HairBuilderModule.h
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})
    
    # builders and tools use the ${gem_name}.Builders and ${gem_name} modules
    ly_create_alias(NAME ${gem_name}.Tools   NAMESPACE Gem TARGETS Gem::${gem_name} Gem::${gem_name}.Builders)

endif()
