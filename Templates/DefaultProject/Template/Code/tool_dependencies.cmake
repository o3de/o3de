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
    Gem::Maestro.Editor
    Gem::TextureAtlas
    Gem::LmbrCentral.Editor
    Gem::NvCloth.Editor
    Gem::LyShine.Editor
    Gem::SceneProcessing.Editor
    Gem::EditorPythonBindings.Editor
    Gem::Camera.Editor
    Gem::CameraFramework
    Gem::Atom_RHI.Private
    Gem::EMotionFX.Editor
    Gem::Atom_RPI.Builders
    Gem::Atom_RPI.Editor
    Gem::Atom_Feature_Common.Builders
    Gem::Atom_Feature_Common.Editor
    Gem::ImGui.Editor
    Gem::Atom_Bootstrap
    Gem::Atom_Asset_Shader.Builders
    Gem::Atom_Component_DebugCamera
    Gem::AtomImGuiTools
    Gem::AtomLyIntegration_CommonFeatures.Editor
    Gem::EMotionFX_Atom.Editor
    Gem::ImageProcessingAtom.Editor
    Gem::Atom_AtomBridge.Editor
    Gem::ImguiAtom
    Gem::AtomFont
    Gem::AtomToolsFramework.Editor
    Gem::GradientSignal.Editor
    Gem::WhiteBox.Editor
)
