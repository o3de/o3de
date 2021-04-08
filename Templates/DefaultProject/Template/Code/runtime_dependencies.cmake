# {BEGIN_LICENSE}
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# {END_LICENSE}

set(GEM_DEPENDENCIES
    Project::${Name}
    Gem::Maestro
    Gem::TextureAtlas
    Gem::LmbrCentral
    Gem::NvCloth
    Gem::LyShine
    Gem::Camera
    Gem::CameraFramework
    Gem::Atom_RHI.Private
    Gem::EMotionFX
    Gem::Atom_RPI.Private
    Gem::Atom_Feature_Common
    Gem::ImGui
    Gem::Atom_Bootstrap
    Gem::Atom_Component_DebugCamera
    Gem::AtomImGuiTools
    Gem::AtomLyIntegration_CommonFeatures
    Gem::EMotionFX_Atom
    Gem::ImguiAtom
    Gem::Atom_AtomBridge
    Gem::GradientSignal
    Gem::AtomFont
    Gem::WhiteBox
)
