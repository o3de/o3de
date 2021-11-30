#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME LyShineExamples.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        lyshineexamples_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::LmbrCentral
        PUBLIC
            Legacy::CryCommon
            Gem::LyShine.Static
)

ly_add_target(
    NAME LyShineExamples ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        lyshineexamples_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::LyShineExamples.Static
            Gem::LmbrCentral.API
)

# if enabled, LyShineExamples is used by all kinds of applications, however, the dependency to LmbrCentral is different
# per application type
ly_create_alias(NAME LyShineExamples.Builders NAMESPACE Gem TARGETS Gem::LyShineExamples Gem::LmbrCentral.Editor)
ly_create_alias(NAME LyShineExamples.Tools    NAMESPACE Gem TARGETS Gem::LyShineExamples Gem::LmbrCentral.Editor)
ly_create_alias(NAME LyShineExamples.Clients  NAMESPACE Gem TARGETS Gem::LyShineExamples Gem::LmbrCentral)
ly_create_alias(NAME LyShineExamples.Servers  NAMESPACE Gem TARGETS Gem::LyShineExamples Gem::LmbrCentral)

