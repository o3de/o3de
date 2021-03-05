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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    //! Holds a collection of generic settings objects to be used by custom serializers.
    class JsonSerializationMetadata final
    {
    public:
        //! Adds a new settings object to the metadata collection.
        //! Only one object of the same type can be added.
        //! Returns false if an object of this type was already added.
        template<typename MetadataT>
        bool Add(MetadataT&& data);

        //! Adds a new settings object to the metadata collection.
        //! Only one object of the same type can be added.
        //! Returns false if an object of this type was already added.
        template<typename MetadataT>
        bool Add(const MetadataT& data);

        //! Returns settings of the type MetadataT, or null if no settings of that type exists.
        template<typename MetadataT>
        MetadataT* Find();
        //! Returns settings of the type MetadataT, or null if no settings of that type exists.
        template<typename MetadataT>
        const MetadataT* Find() const;
        
    private:
        AZStd::unordered_map<AZ::TypeId, AZStd::any> m_data;
    };
} // namespace AZ

#include <AzCore/Serialization/Json/JsonSerializationMetadata.inl>
