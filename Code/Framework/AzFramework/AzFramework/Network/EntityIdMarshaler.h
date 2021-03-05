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
#ifndef AZFRAMEWORK_NETWORK_ENTITYIDMARSHALER_H
#define AZFRAMEWORK_NETWORK_ENTITYIDMARSHALER_H

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/NamedEntityId.h>

#include <GridMate/Serialize/ContainerMarshal.h>
#include <GridMate/Serialize/DataMarshal.h>

namespace GridMate
{
    template<>
    class Marshaler<AZ::EntityId>
    {
    public:
        AZ_TYPE_INFO_LEGACY( Marshaler, "{23F4722F-D104-4E30-9342-43F4DDD1894D}", AZ::EntityId );

        void Marshal(GridMate::WriteBuffer& wb, const AZ::EntityId& source) const
        {
            Marshaler<AZ::u64> idMarshaler;
            idMarshaler.Marshal(wb,static_cast<AZ::u64>(source));
        }
        
        void Unmarshal(AZ::EntityId& target, GridMate::ReadBuffer& rb) const
        {
            AZ::u64 id = 0;
            
            Marshaler<AZ::u64> idMarshaler;
            idMarshaler.Unmarshal(id,rb);
            
            target = AZ::EntityId(id);
        }
    };

    template<>
    class Marshaler<AZ::NamedEntityId>
    {
    public:
        void Marshal(GridMate::WriteBuffer& wb, const AZ::NamedEntityId& source) const
        {
            Marshaler<AZ::u64> idMarshaler;
            idMarshaler.Marshal(wb, static_cast<AZ::u64>(source));

            Marshaler<AZStd::string> stringMarshaler;
            stringMarshaler.Marshal(wb, source.GetName());
        }

        void Unmarshal(AZ::NamedEntityId& target, GridMate::ReadBuffer& rb) const
        {
            AZ::u64 id = 0;

            Marshaler<AZ::u64> idMarshaler;
            idMarshaler.Unmarshal(id, rb);

            AZStd::string name;
            Marshaler<AZStd::string> stringMarshaler;
            stringMarshaler.Unmarshal(name, rb);

            target = AZ::NamedEntityId(AZ::EntityId(id), name);
        }
    };
}

#endif