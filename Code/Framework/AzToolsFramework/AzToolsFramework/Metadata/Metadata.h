/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/any.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AzToolsFramework
{
    class Metadata
    {
    public:
        AZ_TYPE_INFO(Metadata, "{62483693-15A9-46D3-88F8-99FDFF880172}");

        static void Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<Metadata>()
                    ->Version(0)
                    ->Field("Values", &Metadata::m_values);
            }
        }

        void Set(AZStd::string_view key, AZStd::any value)
        {
            m_values[key] = AZStd::move(value);
        }
        AZStd::any Get(AZStd::string_view key)
        {
            auto itr = m_values.find(key);

            return itr != m_values.end() ? itr->second : AZStd::any{};
        }

    private:
        AZStd::unordered_map<AZStd::string, AZStd::any> m_values;
    };
}
