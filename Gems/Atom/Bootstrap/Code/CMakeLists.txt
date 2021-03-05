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
    NAME Atom_Bootstrap.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        bootstrap_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
)

ly_add_target(
    NAME Atom_Bootstrap ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Atom_Bootstrap.c7ff89ad6e8b4b45b2fadef2bcf12d6e.v0.1.0
    FILES_CMAKE
        bootstrap_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
            Gem::Atom_RPI.Public
)
