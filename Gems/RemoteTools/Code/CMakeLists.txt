#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/RemoteTools/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/RemoteTools/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_name}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, that's where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the RemoteTools.Private.Static target
# Note: We include the common files and the platform specific files which are set in remotetools_common_files.cmake
# and in ${pal_dir}/remotetools_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME RemoteTools.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        remotetools_files.cmake
        remotetools_autogen_files.cmake
        ${pal_dir}/remotetools_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            AZ::AzNetworking
    AUTOGEN_RULES
        *.AutoPackets.xml,AutoPackets_Header.jinja,$path/$fileprefix.AutoPackets.h
        *.AutoPackets.xml,AutoPackets_Inline.jinja,$path/$fileprefix.AutoPackets.inl
        *.AutoPackets.xml,AutoPackets_Source.jinja,$path/$fileprefix.AutoPackets.cpp
        *.AutoPackets.xml,AutoPacketDispatcher_Header.jinja,$path/$fileprefix.AutoPacketDispatcher.h
        *.AutoPackets.xml,AutoPacketDispatcher_Inline.jinja,$path/$fileprefix.AutoPacketDispatcher.inl
)

# Here add RemoteTools target, it depends on the RemoteTools.Private.Static
ly_add_target(
    NAME RemoteTools ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        remotetools_shared_files.cmake
        ${pal_dir}/remotetools_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::RemoteTools.Private.Static
)

# By default, we will specify that the above target RemoteTools would be used by
# Client, Server and Tools type targets when this gem is enabled.  If you don't want it
# active in Clients, Servers and Tools by default, delete one or all of the following lines:
ly_create_alias(NAME RemoteTools.Clients NAMESPACE Gem TARGETS Gem::RemoteTools)
ly_create_alias(NAME RemoteTools.Servers NAMESPACE Gem TARGETS Gem::RemoteTools)
ly_create_alias(NAME RemoteTools.Unified NAMESPACE Gem TARGETS Gem::RemoteTools)
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME RemoteTools.Tools   NAMESPACE Gem TARGETS Gem::RemoteTools)
    ly_create_alias(NAME RemoteTools.LuaTools NAMESPACE Gem TARGETS Gem::RemoteTools)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for RemoteTools.Tests
    if(PAL_TRAIT_REMOTETOOLS_TEST_SUPPORTED)
        # We support RemoteTools.Tests on this platform, add RemoteTools.Tests target which depends on RemoteTools.Private.Static
        ly_add_target(
            NAME RemoteTools.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                remotetools_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::RemoteTools.Private.Static
        )

        # Add RemoteTools.Tests to googletest
        ly_add_googletest(
            NAME Gem::RemoteTools.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()
