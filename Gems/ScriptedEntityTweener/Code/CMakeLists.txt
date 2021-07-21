#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ScriptedEntityTweener.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptedentitytweener_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

ly_add_target(
    NAME ScriptedEntityTweener ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        scriptedentitytweener_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ScriptedEntityTweener.Static
            AZ::AzCore
            Legacy::CryCommon
)

# the above module is for use in all application types:
ly_create_alias(NAME ScriptedEntityTweener.Tools    NAMESPACE Gem TARGETS Gem::ScriptedEntityTweener)
ly_create_alias(NAME ScriptedEntityTweener.Clients  NAMESPACE Gem TARGETS Gem::ScriptedEntityTweener)
ly_create_alias(NAME ScriptedEntityTweener.Builders NAMESPACE Gem TARGETS Gem::ScriptedEntityTweener)
ly_create_alias(NAME ScriptedEntityTweener.Servers  NAMESPACE Gem TARGETS Gem::ScriptedEntityTweener)