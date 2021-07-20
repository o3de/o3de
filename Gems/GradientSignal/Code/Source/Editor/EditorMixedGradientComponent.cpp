/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
