/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <ScriptEvents/ScriptEventDefinition.h>

namespace AZ
{
    class BehaviorContext;
    struct BehaviorArgument;
}

namespace ScriptEvents
{
    class ScriptEventBroadcast
        : public AZ::BehaviorMethod
    {
    public:

        AZ_TYPE_INFO(ScriptEventBroadcast, "{7C3DDD76-BECA-4A1D-8605-A72D6CF91051}");
        AZ_CLASS_ALLOCATOR(ScriptEventBroadcast, AZ::SystemAllocator);

        ScriptEventBroadcast(AZ::BehaviorContext* behaviorContext, const ScriptEvent& definition, AZStd::string eventName);

        bool Call(AZStd::span<AZ::BehaviorArgument> params, AZ::BehaviorArgument* returnValue) const override;
        ResultOutcome IsCallable(AZStd::span<AZ::BehaviorArgument> params, AZ::BehaviorArgument* returnValue) const override;
        bool HasResult() const override { return !m_returnType.IsNull(); }
        bool IsMember() const override { return false; }

        void ReserveArguments(size_t numArguments);

        size_t GetNumArguments() const override { return m_behaviorParameters.size(); }
        const AZ::BehaviorParameter* GetArgument(size_t index) const override
        {
            if (index >= m_behaviorParameters.size())
            {
                AZ_Warning("Script Events", false, "Index out of bounds while trying to get method argument (%s, %d)", m_name.c_str(), index);
                return nullptr;
            }

            return &m_behaviorParameters[index];
        }

        const AZStd::string* GetArgumentName(size_t index) const override { return &m_argumentNames[index]; }
        void SetArgumentName(size_t index, AZStd::string name) override;

        const AZ::BehaviorParameter* GetResult() const override { return &m_result; }
        bool HasBusId() const override { return false; }

        const AZStd::string* GetArgumentToolTip(size_t index) const override { return &m_argumentToolTips[index]; }
        void SetArgumentToolTip(size_t index, AZStd::string tooltip) override
        {
            if (index >= m_argumentToolTips.size())
            {
                m_argumentToolTips.resize(index + 1);
            }

            m_argumentToolTips[index] = tooltip;
        }

        const AZ::BehaviorParameter* GetBusIdArgument() const override { return nullptr; }
        size_t GetMinNumberOfArguments() const override;

        AZ::BehaviorDefaultValuePtr  GetDefaultValue(size_t) const override;

        void OverrideParameterTraits(size_t, AZ::u32, AZ::u32) override {}
        void SetDefaultValue(size_t, AZ::BehaviorDefaultValuePtr) override {}

    private:

        AZ::Uuid m_returnType;
        AZ::BehaviorArgument m_result;

        AZStd::vector<AZStd::string> m_argumentNames;
        AZStd::vector<AZStd::string> m_argumentToolTips;

        AZStd::vector<AZ::BehaviorParameter> m_behaviorParameters;

        AZ::Uuid m_busBindingId;

    };
}
