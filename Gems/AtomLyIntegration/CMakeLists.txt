#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The "AtomLyIntegration" Gem will alias the real Atom_AtomBridge target variants
ly_create_alias(NAME AtomLyIntegration.Clients NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Clients)
ly_create_alias(NAME AtomLyIntegration.Servers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Servers)
ly_create_alias(NAME AtomLyIntegration.HeadlessServers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.HeadlessServers)
ly_create_alias(NAME AtomLyIntegration.Unified NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Unified)
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME AtomLyIntegration.Builders NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Builders)
    ly_create_alias(NAME AtomLyIntegration.Tools    NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Tools)
endif()
