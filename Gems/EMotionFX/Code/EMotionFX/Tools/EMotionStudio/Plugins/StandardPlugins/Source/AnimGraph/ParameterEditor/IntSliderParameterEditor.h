/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameterEditor.h"

namespace EMStudio
{
    class IntSliderParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(IntSliderParameterEditor, "{95295E4B-2D0F-4462-B107-22D124A8B845}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        IntSliderParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0)
        {}

        IntSliderParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes);

        ~IntSliderParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        int GetMinValue() const;
        int GetMaxValue() const;

        int m_currentValue;
    };
} 
