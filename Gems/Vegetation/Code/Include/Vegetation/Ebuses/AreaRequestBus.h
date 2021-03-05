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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <Vegetation/InstanceData.h>

namespace AZ
{
    class Vector3;
}

namespace Vegetation
{
    struct Descriptor;

    using ClaimHandle = uint64_t;
    using EntityIdStack = AZStd::vector<AZ::EntityId>;


    /**
    * allows adding and automatic removal (on destruction) from a stack of entity ids
    */
    class EntityIdStackPusher
    {
    public:
        EntityIdStackPusher(EntityIdStack& stack, const AZ::EntityId id)
            : m_stack(stack)
        {
            m_stack.push_back(id);
        }

        ~EntityIdStackPusher()
        {
            m_stack.pop_back();
        }
    private:
        EntityIdStackPusher() = delete;
        EntityIdStackPusher(EntityIdStack& stack) = delete;
        EntityIdStackPusher(EntityIdStack&& stack) = delete;
        EntityIdStackPusher& operator=(EntityIdStack& stack) = delete;
        EntityIdStack& m_stack;
    };

    struct ClaimPoint
    {
        ClaimHandle m_handle;
        AZ::Vector3 m_position;
        AZ::Vector3 m_normal;
        SurfaceData::SurfaceTagWeightMap m_masks;
    };

    struct ClaimContext
    {
        SurfaceData::SurfaceTagWeightMap m_masks;
        AZStd::vector<ClaimPoint> m_availablePoints;
        AZStd::function<bool(const ClaimPoint&, const InstanceData&)> m_existedCallback;
        AZStd::function<void(const ClaimPoint&, const InstanceData&)> m_createdCallback;
    };

    /**
    * This bus declares the minimum interface needed for a component to serve as a vegetation area
    */      
    class AreaRequests : public AZ::ComponentBus
    {
    public: 
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        /**
        * execute any pre-claim checks or logic that's not needed per point
        */
        virtual bool PrepareToClaim(EntityIdStack& stackIds) = 0;

        /**
        * Claims a world position by executing a 'vegetation location operation' such as 'place' or 'clear'
        */
        virtual void ClaimPositions(EntityIdStack& stackIds, ClaimContext& context) = 0;

        /**
        * Reverses a previous 'vegetation location operation' such as 'place' or 'clear'
        */
        virtual void UnclaimPosition(const ClaimHandle handle) = 0;

    };

    typedef AZ::EBus<AreaRequests> AreaRequestBus;
}