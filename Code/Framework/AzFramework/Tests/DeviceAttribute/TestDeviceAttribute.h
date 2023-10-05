/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Device/DeviceAttributesSystemComponent.h>

namespace UnitTest
{
    class TestDeviceAttribute
        : public AzFramework::DeviceAttribute
    {
    public:
        using EvalFunc = AZStd::function<bool(AZStd::string_view)>;

        TestDeviceAttribute(AZStd::string_view name, AZStd::string_view description, AZStd::any value, EvalFunc eval);
        ~TestDeviceAttribute() = default;

        AZStd::string_view GetName() const override;
        AZStd::string_view GetDescription() const override;
        bool Evaluate(AZStd::string_view rule) const override;
        AZStd::any GetValue() const override;

    private:
        AZStd::string m_name;
        AZStd::string m_description;
        EvalFunc m_eval;
        AZStd::any m_value;
    };

} // namespace UnitTest

