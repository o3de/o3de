/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameterEditor.h"
#include <AzCore/Math/Vector2.h>

namespace EMStudio
{
    class Vector2ParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(Vector2ParameterEditor, "{D956C877-4DF1-4A08-BD27-BD79E88B24EE}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        Vector2ParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0.0f, 0.0f)
        {}

        Vector2ParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes);

        ~Vector2ParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        AZ::Vector2 GetMinValue() const;
        AZ::Vector2 GetMaxValue() const;

        AZ::Vector2 m_currentValue;
    };
}
