
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/ToolingConnection/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/ToolingConnection/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_name}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the ToolingConnection.Private.Static target
# Note: We include the common files and the platform specific files which are set in toolingconnection_common_files.cmake
# and in ${pal_dir}/toolingconnection_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME ToolingConnection.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        toolingconnection_files.cmake
        toolingconnection_autogen_files.cmake
        ${pal_dir}/toolingconnection_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
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

# Here add ToolingConnection target, it depends on the ToolingConnection.Static
ly_add_target(
    NAME ToolingConnection ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        toolingconnection_shared_files.cmake
        ${pal_dir}/toolingconnection_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ToolingConnection.Private.Static
)

# By default, we will specify that the above target ToolingConnection would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ToolingConnection.Clients NAMESPACE Gem TARGETS Gem::ToolingConnection)
ly_create_alias(NAME ToolingConnection.Servers NAMESPACE Gem TARGETS Gem::ToolingConnection)

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ToolingConnection.Static
    if(PAL_TRAIT_TOOLINGCONNECTION_TEST_SUPPORTED)
        # We support ToolingConnection.Tests on this platform, add ToolingConnection.Tests target which depends on ToolingConnection.Static
        ly_add_target(
            NAME ToolingConnection.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                toolingconnection_files.cmake
                toolingconnection_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::ToolingConnection.Private.Static
        )

        # Add ToolingConnection.Tests to googletest
        ly_add_googletest(
            NAME Gem::ToolingConnection.Tests
        )
    endif()
endif()
