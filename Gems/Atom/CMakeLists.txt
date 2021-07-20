#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(Asset)
add_subdirectory(Bootstrap)
add_subdirectory(Component)
add_subdirectory(Feature)
add_subdirectory(RHI)
add_subdirectory(RPI)
add_subdirectory(Tools)
add_subdirectory(Utils)

# The "Atom" Gem will alias the real Atom_AtomBridge target variants
# allows the enabling and disabling the "Atom" Gem to build the pre-requisite dependencies
ly_create_alias(NAME Atom.Clients NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Clients)
ly_create_alias(NAME Atom.Servers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Servers)
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME Atom.Builders NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Builders)
    ly_create_alias(NAME Atom.Tools    NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Tools)
endif()
