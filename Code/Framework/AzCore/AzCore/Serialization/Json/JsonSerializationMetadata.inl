/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace AZ
{
    template<typename MetadataT, typename... Args>
    bool JsonSerializationMetadata::Create(Args&&... args)
    {
        const Uuid& typeId = azrtti_typeid<MetadataT>();
        if (m_data.find(typeId) != m_data.end())
        {
            AZ_Assert(false, "Metadata object of type %s already added", typeId.template ToString<AZStd::string>().c_str());
            return false;
        }

        m_data.emplace(typeId, MetadataT{AZStd::forward<Args>(args)...});
        return true;
    }

    template<typename MetadataT>
    bool JsonSerializationMetadata::Add(MetadataT&& data)
    {
        const Uuid& typeId = azrtti_typeid<MetadataT>();
        if (m_data.find(typeId) != m_data.end())
        {
            AZ_Assert(false, "Metadata object of type %s already added", typeId.template ToString<AZStd::string>().c_str());
            return false;
        }

        m_data.emplace(typeId, AZStd::forward<MetadataT>(data));
        return true;
    }

    template<typename MetadataT>
    bool JsonSerializationMetadata::Add(const MetadataT& data)
    {
        return Add(MetadataT{ data });
    }

    template<typename MetadataT>
    MetadataT* JsonSerializationMetadata::Find()
    {
        const Uuid& typeId = azrtti_typeid<MetadataT>();
        auto iter = m_data.find(typeId);
        return iter != m_data.end() ? AZStd::any_cast<MetadataT>(&iter->second) : nullptr;
    }
    template<typename MetadataT>
    const MetadataT* JsonSerializationMetadata::Find() const
    {
        return const_cast<JsonSerializationMetadata*>(this)->Find<MetadataT>();
    }
} // namespace AZ
