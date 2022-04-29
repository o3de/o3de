/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! Structure containing settings for configuring individual input and output slots on a dynamic node
    struct DynamicNodeSlotConfig final
    {
        AZ_CLASS_ALLOCATOR(DynamicNodeSlotConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicNodeSlotConfig, "{F2C95A99-41FD-4077-B9A7-B0BF8F76C2CE}");
        static void Reflect(AZ::ReflectContext* context);

        DynamicNodeSlotConfig(
            const AZStd::string& type,
            const AZStd::string& name,
            const AZStd::string& displayName,
            const AZStd::string& description,
            const AZStd::vector<AZStd::string>& supportedDataTypes,
            const AZStd::string& defaultValue);
        DynamicNodeSlotConfig() = default;
        ~DynamicNodeSlotConfig() = default;

        AZStd::string m_type = "Data";
        AZStd::string m_name = "Unnamed";
        AZStd::string m_displayName = "Unnamed";
        AZStd::string m_description;
        AZStd::vector<AZStd::string> m_supportedDataTypes;
        AZStd::string m_defaultValue;
    };
} // namespace AtomToolsFramework
