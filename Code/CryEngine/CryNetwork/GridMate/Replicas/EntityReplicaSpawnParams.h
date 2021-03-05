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
#ifndef INCLUDE_GRIDMATEENTITYREPLICASPAWNPARAMS_HEADER
#define INCLUDE_GRIDMATEENTITYREPLICASPAWNPARAMS_HEADER

#pragma once

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Replica/DataSet.h>

#include "../NetworkGridMateCommon.h"

namespace GridMate
{
    /*!
     * Wrapper for entity spawn params, to ensure clients are able to spawn entities
     * locally with correct parameters.
     */
    class EntitySpawnParamsStorage
    {
    public:

        enum ParamsFlags
        {
            kParamsFlag_None = 0,
            kParamsFlag_IsGameRules = BIT(0),         //! This entity represents the Game Rules instance.
            kParamsFlag_IsLevelEntity = BIT(1),       //! This entity is static, from the level.
            kParamsFlag_CreatedThroughPool = BIT(2),  //! Was the entity created through the entity pool?
            kParamsFlag_Max
        };

        enum MarshalFlags
        {
            kMarshalFlag_HasGuid = BIT(0),             //! This entity has optional GUID
            kMarshalFlag_HasArchetype = BIT(1),        //! This entity has archetype
            kMarshalFlag_OrientationNorm = BIT(2),     //! Is entity's orientation quaternion normalized
            kMarshalFlag_HasScale = BIT(3),            //! Entity is scaled
            kMarshalFlag_Max
        };

        EntitySpawnParamsStorage();
        EntitySpawnParamsStorage(const EntitySpawnParamsStorage& another);

        EntityId        m_id;                   //! The entity Id on the server.
        string m_entityName;             //! The name of the entity
        string m_className;              //! The entity's class name
        string m_archetypeName;          //! The entity's optional archetype name
        AZ::u32 m_flags;                 //! Any entity flags
        AZ::u8 m_flagsExtended;          //! Extended flags
        ChannelId       m_channelId;            //! Channel id (network owner) associated with the entity
        Vec3            m_position;             //! Starting position.
        Quat            m_orientation;          //! Starting orientation.
        Vec3            m_scale;                //! Entity scale.
        AZ::u8 m_paramsFlags;            //! Internal flags (see above ParamsFlags enum).

        bool IsCreatedThroughPool() const { return !!(m_paramsFlags & kParamsFlag_CreatedThroughPool); }
        inline bool operator == (const EntitySpawnParamsStorage& rhs) const;

        class Marshaler
        {
        public:

            void Marshal(GridMate::WriteBuffer& wb, const EntitySpawnParamsStorage& s);
            void Unmarshal(EntitySpawnParamsStorage& s, GridMate::ReadBuffer& rb);
        };
    };

    /*!
     * When entities are created in CryEngine, they can optionally specify an arbitrary
     * amount block of spawn data. We maintain support for this and marshal the data
     * along with the replica.
     */
    class EntityExtraSpawnInfo
        : public _i_multithread_reference_target_t
    {
    public:

        typedef _smart_ptr<EntityExtraSpawnInfo> Ptr;

        EntityExtraSpawnInfo() {}
        EntityExtraSpawnInfo(const char* sourceBuffer, uint32 sourceBufferSize)
            : m_buffer(sourceBuffer, sourceBufferSize)
        {
        }

        typedef FlexibleBuffer<1024> DataBuffer;
        DataBuffer m_buffer;

        struct Marshaler
        {
            void Marshal(GridMate::WriteBuffer& wb, const EntityExtraSpawnInfo::Ptr& v);
            void Unmarshal(EntityExtraSpawnInfo::Ptr& v, GridMate::ReadBuffer& rb);
        };
    };

    typedef GridMate::DataSet < EntityExtraSpawnInfo::Ptr, EntityExtraSpawnInfo::Marshaler >
        SerializedEntityExtraSpawnInfo;

    inline bool EntitySpawnParamsStorage::operator == (const EntitySpawnParamsStorage& rhs) const
    {
        return    (m_id == rhs.m_id &&
                   m_flags == rhs.m_flags &&
                   m_flagsExtended == rhs.m_flagsExtended &&
                   m_channelId == rhs.m_channelId &&
                   m_entityName == rhs.m_entityName &&
                   m_className == rhs.m_className &&
                   m_archetypeName == rhs.m_archetypeName &&
                   m_paramsFlags == rhs.m_paramsFlags &&
                   m_position.IsEquivalent(rhs.m_position) &&
                   Quat::IsEquivalent(m_orientation, rhs.m_orientation));
    }
} // namespace GridMate

#endif // INCLUDE_GRIDMATEENTITYREPLICASPAWNPARAMS_HEADER
