#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME CertificateManager.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        CertificateManager_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Legacy::CryCommon
)

ly_add_target(
    NAME CertificateManager ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        certificatemanager_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::CertificateManager.Static
)

# we'll load the above "Gem::CertificateManager" module in Clients and Servers
ly_create_alias(NAME CertificateManager.Clients NAMESPACE Gem TARGETS Gem::CertificateManager)
ly_create_alias(NAME CertificateManager.Servers NAMESPACE Gem TARGETS Gem::CertificateManager)
