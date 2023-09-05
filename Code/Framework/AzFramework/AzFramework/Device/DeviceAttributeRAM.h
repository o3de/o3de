/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Device/DeviceAttributeInterface.h>

namespace AzFramework
{
    //! Device attribute for getting RAM available to OS in GiB 
    //! which is usually less than the physically installed RAM
    class DeviceAttributeRAM : public DeviceAttribute
    {
    public:
        DeviceAttributeRAM();
        AZStd::string_view GetName() const override;
        AZStd::string_view GetDescription() const override;
        AZStd::any GetValue() const override;
        bool Evaluate(AZStd::string_view rule) const override;
    protected:
        AZStd::string m_name = "RAM";
        AZStd::string m_description = "RAM available to OS in GiB, usually less than the physically installed RAM.";
        float m_valueInGiB = 0.f;
    };
} // namespace AzFramework
