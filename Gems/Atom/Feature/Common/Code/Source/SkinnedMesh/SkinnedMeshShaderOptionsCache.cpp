/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <SkinnedMesh/SkinnedMeshShaderOptionsCache.h>

#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace Render
    {
        void CachedSkinnedMeshShaderOptions::SetShader(Data::Instance<RPI::Shader> shader)
        {
            m_shader = shader;

            // Cache the different possible indices and values so they are efficent to set later
            const RPI::ShaderOptionGroupLayout* layout = m_shader->GetAsset()->GetShaderOptionGroupLayout();
            m_skinningMethodOptionIndex = layout->FindShaderOptionIndex(AZ::Name("o_skinningMethod"));
            m_skinningMethodLinearSkinningValue = layout->FindValue(m_skinningMethodOptionIndex, AZ::Name("SkinningMethod::LinearSkinning"));
            m_skinningMethodDualQuaternionValue = layout->FindValue(m_skinningMethodOptionIndex, AZ::Name("SkinningMethod::DualQuaternion"));

            m_applyMorphTargetOptionIndex = layout->FindShaderOptionIndex(AZ::Name("o_applyMorphTargets"));
            m_applyMorphTargetFalseValue = layout->FindValue(m_applyMorphTargetOptionIndex, AZ::Name("false"));
            m_applyMorphTargetTrueValue = layout->FindValue(m_applyMorphTargetOptionIndex, AZ::Name("true"));

            m_applyColorMorphTargetOptionIndex = layout->FindShaderOptionIndex(AZ::Name("o_applyColorMorphTargets"));
            m_applyColorMorphTargetFalseValue = layout->FindValue(m_applyColorMorphTargetOptionIndex, AZ::Name("false"));
            m_applyColorMorphTargetTrueValue = layout->FindValue(m_applyColorMorphTargetOptionIndex, AZ::Name("true"));

            SkinnedMeshShaderOptionNotificationBus::Event(this, &SkinnedMeshShaderOptionNotificationBus::Events::OnShaderReinitialized, this);
        }

        void CachedSkinnedMeshShaderOptions::ConnectToShaderReinitializedEvent(SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedEventHandler)
        {
            shaderReinitializedEventHandler.BusConnect(this);
        }

        RPI::ShaderOptionGroup CachedSkinnedMeshShaderOptions::CreateShaderOptionGroup(const SkinnedMeshShaderOptions& shaderOptions) const
        {
            RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
            switch (shaderOptions.m_skinningMethod)
            {
            case SkinningMethod::DualQuaternion:
                shaderOptionGroup.SetValue(m_skinningMethodOptionIndex, m_skinningMethodDualQuaternionValue);
                break;
            case SkinningMethod::LinearSkinning:
            default:
                shaderOptionGroup.SetValue(m_skinningMethodOptionIndex, m_skinningMethodLinearSkinningValue);
            }

            if (shaderOptions.m_applyMorphTargets)
            {
                shaderOptionGroup.SetValue(m_applyMorphTargetOptionIndex, m_applyMorphTargetTrueValue);
            }
            else
            {
                shaderOptionGroup.SetValue(m_applyMorphTargetOptionIndex, m_applyMorphTargetFalseValue);
            }

            if (shaderOptions.m_applyColorMorphTargets)
            {
                shaderOptionGroup.SetValue(m_applyColorMorphTargetOptionIndex, m_applyColorMorphTargetTrueValue);
            }
            else
            {
                shaderOptionGroup.SetValue(m_applyColorMorphTargetOptionIndex, m_applyColorMorphTargetFalseValue);
            }

            shaderOptionGroup.SetUnspecifiedToDefaultValues();

            return shaderOptionGroup;
        }
    } // namespace Render
} // namespace AZ
