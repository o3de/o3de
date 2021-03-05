/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/string/string.h>

namespace AZ
{
    template<typename MetadataT>
    bool JsonSerializationMetadata::Add(MetadataT&& data)
    {
        auto typeId = azrtti_typeid<MetadataT>();
        auto iter = m_data.find(typeId);
        if (iter != m_data.end())
        {
            AZ_Warning("JsonSerializationMetadata", false, "Metadata object of type %s already added",
                typeId.template ToString<AZStd::string>().c_str());
            return false;
        }

        m_data[typeId] = AZStd::any{ AZStd::forward<MetadataT>(data) };
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
        const auto& typeId = azrtti_typeid<MetadataT>();
        auto iter = m_data.find(typeId);
        return iter != m_data.end() ? AZStd::any_cast<MetadataT>(&iter->second) : nullptr;
    }
    template<typename MetadataT>
    const MetadataT* JsonSerializationMetadata::Find() const
    {
        return const_cast<JsonSerializationMetadata*>(this)->Find<MetadataT>();
    }
} // namespace AZ

#include <AzCore/Serialization/Json/JsonSerializationMetadata.inl>
