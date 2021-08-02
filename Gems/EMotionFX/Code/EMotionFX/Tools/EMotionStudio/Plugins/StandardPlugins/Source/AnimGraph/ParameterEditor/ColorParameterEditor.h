/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ValueParameterEditor.h"
#include <AzCore/Math/Color.h>

namespace EMStudio
{
    class ColorParameterEditor
        : public ValueParameterEditor
    {
    public:
        AZ_RTTI(ColorParameterEditor, "{FA7FC2C0-AA4E-490A-B46E-B2FCF755BA58}", ValueParameterEditor)
        AZ_CLASS_ALLOCATOR_DECL

        // Required for serialization
        ColorParameterEditor()
            : ColorParameterEditor(nullptr, nullptr, AZStd::vector<MCore::Attribute*>())
        {}

        ColorParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes);

        ~ColorParameterEditor() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void UpdateValue() override;

    private:
        void OnValueChanged();

        AZ::Color GetMinValue() const;
        AZ::Color GetMaxValue() const;

        AZ::Color m_currentValue;
    };
}
