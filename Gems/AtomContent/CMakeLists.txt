#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
add_subdirectory(ReferenceMaterials)
add_subdirectory(Sponza)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME AtomContent.Builders NAMESPACE Gem TARGETS Gem::AtomContent_ReferenceMaterials.Builders Gem::AtomContent_Sponza.Builders)
endif()
