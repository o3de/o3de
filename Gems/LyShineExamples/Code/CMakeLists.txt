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
        PUBLIC
            Legacy::CryCommon
            Gem::LmbrCentral.Static
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
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# if enabled, LyShineExamples is used by all kinds of applications
ly_create_alias(NAME LyShineExamples.Builders NAMESPACE Gem TARGETS Gem::LyShineExamples)
ly_create_alias(NAME LyShineExamples.Tools    NAMESPACE Gem TARGETS Gem::LyShineExamples)
ly_create_alias(NAME LyShineExamples.Clients  NAMESPACE Gem TARGETS Gem::LyShineExamples)
ly_create_alias(NAME LyShineExamples.Servers  NAMESPACE Gem TARGETS Gem::LyShineExamples)

