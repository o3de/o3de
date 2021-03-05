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

#include "GradientSignal_precompiled.h"
#include "EditorMixedGradientComponent.h"

namespace GradientSignal
{
    void EditorMixedGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorMixedGradientComponent, BaseClassType>(context);
    }

    void EditorMixedGradientComponent::Init()
    {
        ForceOneEntry();
        BaseClassType::Init();
    }

    void EditorMixedGradientComponent::Activate()
    {
        ForceOneEntry();
        BaseClassType::Activate();
    }

    AZ::u32 EditorMixedGradientComponent::ConfigurationChanged()
    {
        ForceOneEntry();
        SetSamplerOwnerEntity(m_configuration, GetEntityId());
        return EditorGradientComponentBase::ConfigurationChanged();
    }

    void EditorMixedGradientComponent::ForceOneEntry()
    {
        if (m_configuration.m_layers.empty())
        {
            m_configuration.m_layers.push_back();
            m_configuration.m_layers.front().m_operation = MixedGradientLayer::MixingOperation::Initialize;
            SetDirty();
        }
    }
}
