#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(CommonFeatures)
add_subdirectory(ImguiAtom)
add_subdirectory(AtomImGuiTools)
add_subdirectory(EMotionFXAtom)
add_subdirectory(AtomFont)
add_subdirectory(TechnicalArt)
add_subdirectory(AtomBridge)
add_subdirectory(AtomViewportDisplayInfo)
add_subdirectory(AtomViewportDisplayIcons)

# The "AtomLyIntegration" Gem will also alias the real Atom_AtomBridge target variants
# The Atom Gem does the same at the moment.
ly_create_alias(NAME AtomLyIntegration.Clients NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Clients)
ly_create_alias(NAME AtomLyIntegration.Servers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Servers)
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME AtomLyIntegration.Builders NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Builders)
    ly_create_alias(NAME AtomLyIntegration.Tools    NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Tools)
endif()
