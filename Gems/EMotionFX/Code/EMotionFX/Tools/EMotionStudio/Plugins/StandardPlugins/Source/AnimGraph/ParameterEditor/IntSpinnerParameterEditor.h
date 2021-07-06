/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameterEditor.h"

namespace EMStudio
{
    class IntSpinnerParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(IntSpinnerParameterEditor, "{4DA44E56-AA78-4BEF-88E6-81AA4E65729F}", ValueParameterEditor);
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        IntSpinnerParameterEditor()
            : ValueParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
            , m_currentValue(0)
        {}

        IntSpinnerParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes);

        ~IntSpinnerParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        int GetMinValue() const;
        int GetMaxValue() const;

        int m_currentValue;
    };
} 
