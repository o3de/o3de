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

#ifndef CRYINCLUDE_CRYCOMMON_IPROXIMITYTRIGGERSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_IPROXIMITYTRIGGERSYSTEM_H
#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/function/function_fwd.h>
#include <Cry_Geo.h> // AABB


/**
 * Represents a registered proximity trigger.
 *
 * Contains the id of the trigger, its bounds, and whether or not it's active.
 */
struct SProximityElement
{
    AZ::EntityId id;
    AABB aabb;
    uint32 bActivated : 1;
    std::vector<SProximityElement*> inside;

    using NarrowPassCheckFunction = AZStd::function<bool(const AZ::Vector3&)>;

    // Can be used to do an optional narrow pass check on this proximity element
    NarrowPassCheckFunction m_narrowPassChecker;

    SProximityElement()
    {
        id = AZ::EntityId(0);
        bActivated = 0;
    }
    ~SProximityElement()
    {
    }
    bool AddInside(SProximityElement* elem)
    {
        // Sorted add.
        return stl::binary_insert_unique(inside, elem);
    }
    bool RemoveInside(SProximityElement* elem)
    {
        // sorted remove.
        return stl::binary_erase(inside, elem);
    }
    bool IsInside(SProximityElement* elem)
    {
        return std::binary_search(inside.begin(), inside.end(), elem);
    }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};

/**
 * Bus for events dispatched by the proximity trigger system as triggered are
 * entered and exited by entities in the world.
 */
class ProximityTriggerEvents
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Ebus Traits
    // ID'd on trigger entity Id
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::EntityId;
    //////////////////////////////////////////////////////////////////////////

    virtual ~ProximityTriggerEvents() {}

    /// Dispatched when an entity enters a trigger. The bus message is ID'd on the triggers entity Id.
    virtual void OnTriggerEnter(AZ::EntityId /*entityEntering*/) {};

    /// Dispatched when an entity exits a trigger. The bus message is ID'd on the triggers entity Id.
    virtual void OnTriggerExit(AZ::EntityId /*entityExiting*/) {};
};

using ProximityTriggerEventBus = AZ::EBus<ProximityTriggerEvents>;

/**
 * Bus for requests sent by components or game code to the proximity trigger system.
 */
class ProximityTriggerSystemRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides - proximity trigger system is a singleton
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    virtual ~ProximityTriggerSystemRequests() {}

    /// Creates a new trigger instance.
    virtual SProximityElement* CreateTrigger(SProximityElement::NarrowPassCheckFunction narrowPassChecker = nullptr) = 0;

    /// Removes a trigger and queues it for deletion.
    virtual void RemoveTrigger(SProximityElement* pTrigger) = 0;

    /// Moves a trigger in the world or redefines its dimensions.
    virtual void MoveTrigger(SProximityElement* pTrigger, const AABB& aabb, bool invalidateCachedAABB = false) = 0;

    /// Creates a proxy in the world associated with an entity (Component or Legacy) for interacting with proximity trigger instances.
    virtual SProximityElement* CreateEntity(AZ::EntityId id) = 0;

    /** 
     * Set the entity's AABB to a unit AABB at the entity's world position if \aabb is empty, otherwise set the entity's AABB to \aabb
     * @param pEntity The pointer to a SProximityElment whose AABB needs to be updated
     * @param pos World position of the entity
     * @param aabb The new AABB in world space to set
     */
    virtual void MoveEntity(SProximityElement* pEntity, const Vec3& pos, const AABB& aabb) = 0;

    /// Removes an entity's proximity trigger proxy.
    virtual void RemoveEntity(SProximityElement* pEntity, bool instantEvent = false) = 0;
};

using ProximityTriggerSystemRequestBus = AZ::EBus<ProximityTriggerSystemRequests>;

#endif // CRYINCLUDE_CRYCOMMON_IPROXIMITYTRIGGERSYSTEM_H
