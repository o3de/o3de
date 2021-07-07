/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        //! Creates a new settings object in the metadata collection.
        //! Only one object of the same type can be added  or created.
        //! Returns false if an object of this type was already added.
        template<typename MetadataT, typename... Args>
        bool Create(Args&&... args);

        //! Adds a new settings object to the metadata collection.
        //! Only one object of the same type can be added or created.
        //! Returns false if an object of this type was already added.
        template<typename MetadataT>
        bool Add(MetadataT&& data);

        //! Adds a new settings object to the metadata collection.
        //! Only one object of the same type can be added  or created.
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
