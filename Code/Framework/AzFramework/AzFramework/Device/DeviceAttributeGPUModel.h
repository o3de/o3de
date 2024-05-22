/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Device/DeviceAttributeInterface.h>
#include <AzCore/std/containers/vector.h>

namespace AzFramework
{
    //! Device attribute for getting the GPU(s) model name.
    //! It will match against any of the GPUs being used.
    class DeviceAttributeGPUModel : public DeviceAttribute
    {
    public:
        DeviceAttributeGPUModel(const AZStd::vector<AZStd::string_view>& value);
        AZStd::string_view GetName() const override;
        AZStd::string_view GetDescription() const override;
        AZStd::any GetValue() const override;
        bool Evaluate(AZStd::string_view rule) const override;

    protected:
        AZStd::string m_name = "GPUModel";
        AZStd::string m_description = "Model of the GPU(s) being used.";
        AZStd::vector<AZStd::string> m_value; //! List of GPU models
    };
} // namespace AzFramework
