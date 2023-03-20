/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Capsule.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Hemisphere.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/vector.h>

namespace AzFramework
{
    //! This is a base spatial hash node type.
    //! This is so that we can potentially have different spatial hash implementations that satisfy the IVisibilitySystem interface.
    struct VisibilityNode {};

    //! Data for an object that is added to the visibility system
    struct VisibilityEntry
    {
        //! Can be used for filtering and to determine the underlying type for VisibilityEntry::m_userData
        enum TypeFlags
        {
            TYPE_None = 0,
            TYPE_Entity = 1 << 0,      // All entities
            TYPE_RPI_Cullable = 1 << 2 // Cullable by the render system
        };

        AZ::Aabb m_boundingVolume = AZ::Aabb::CreateNull();
        VisibilityNode* m_internalNode = nullptr;
        void* m_userData = nullptr;
        uint32_t m_internalNodeIndex = 0;
        TypeFlags m_typeFlags = TYPE_None;
    };

    //! @class IVisibilityScene
    //! @brief This is the interface for managing objects and visibility queries for a given scene.
    class IVisibilityScene
    {
    public:
        AZ_RTTI(IVisibilityScene, "{822BC414-3CE3-40B4-A9A2-A42EA5B9499F}");

        IVisibilityScene() = default;
        virtual ~IVisibilityScene() = default;

        struct NodeData
        {
            const AZ::Aabb m_bounds;
            const AZStd::vector<VisibilityEntry*>& m_entries;
        };
        using EnumerateCallback = AZStd::function<void(const NodeData&)>;

        //! Get the unique scene name, used to look up the scene in the IVisibilitySystem. Duplicate names will assert on creation.
        virtual const AZ::Name& GetName() const = 0;

        //! Insert or update an entry within the visibility system.
        //! This encompasses the following three scenarios:
        //   1. A new entry is added to the underlying spatial hash.
        //   2. A previously added entry moves to a new position within its current node in the spatial hash.
        //   3. A previously added entry moves to a new node in the spatial hash.
        //       (causing it to be removed from its original node and added to its new node)
        //! @param visibilityEntry data for the object being added/updated
        virtual void InsertOrUpdateEntry(VisibilityEntry& visibilityEntry) = 0;

        //! Removes an entry from the visibility system.
        //! @param visibilityEntry data for the object being removed
        virtual void RemoveEntry(VisibilityEntry& visibilityEntry) = 0;

        //! Intersects an axis aligned bounding box against the visibility system.
        //! @param aabb the axis aligned bounding box to test against
        //! @param callback the callback to invoke when a node is visible
        virtual void Enumerate(const AZ::Aabb& aabb, const EnumerateCallback& callback) const = 0;

        //! Intersects a sphere against the visibility system.
        //! @param sphere the sphere to test against
        //! @param callback the callback to invoke when a node is visible
        virtual void Enumerate(const AZ::Sphere& sphere, const EnumerateCallback& callback) const = 0;

        //! Intersects a hemisphere against the visibility system.
        //! @param hemisphere the hemisphere to test against
        //! @param callback the callback to invoke when a node is visible
        virtual void Enumerate(const AZ::Hemisphere& hemisphere, const EnumerateCallback& callback) const = 0;

        //! Intersects a capsule against the visibility system.
        //! @param capsule the capsule to test against
        //! @param callback the callback to invoke when a node is visible
        virtual void Enumerate(const AZ::Capsule& capsule, const EnumerateCallback& callback) const = 0;

        //! Intersects a frustum against the visibility system.
        //! @param frustum the frustum to test against
        //! @param callback the callback to invoke when a node is visible
        virtual void Enumerate(const AZ::Frustum& frustum, const EnumerateCallback& callback) const = 0;

        //! Enumerate *all* OctreeNodes that have any entries in them (without any culling).
        //! @param callback the callback to invoke when a node is visible
        virtual void EnumerateNoCull(const EnumerateCallback& callback) const = 0;

        //! Return the number of VisibilityEntries that have been added to the system
        virtual uint32_t GetEntryCount() const = 0;
    };

    //! @class IVisibilitySystem
    //! @brief This is an AZ::Interface<> useful for extremely fast, CPU only, proximity and visibility queries.
    class IVisibilitySystem
    {
    public:
        AZ_RTTI(IVisibilitySystem, "{7C6C710F-ACDB-44CD-867D-A2C4B912ECF5}");

        IVisibilitySystem() = default;
        virtual ~IVisibilitySystem() = default;

        //! Return the default IVisibilityScene for entities.
        virtual IVisibilityScene* GetDefaultVisibilityScene() = 0;

        //! Create a new IVisibilityScene that is uniquely identified by the scene name.
        virtual IVisibilityScene* CreateVisibilityScene(const AZ::Name& sceneName) = 0;

        //! Destroy the visibility scene.
        //! This does not destroy the entities that are a part of the scene, only the visibility scene.
        //! This will set the visScene to nullptr
        virtual void DestroyVisibilityScene(IVisibilityScene* visScene) = 0;

        //! Find the IVisibilityScene that is identified by sceneName.
        virtual IVisibilityScene* FindVisibilityScene(const AZ::Name& sceneName) = 0;

        //! Logs stats about the visibility system to the console.
        virtual void DumpStats(const AZ::ConsoleCommandContainer& arguments) = 0;

        AZ_DISABLE_COPY_MOVE(IVisibilitySystem);
    };

    // EBus wrapper for ScriptCanvas
    class IVisibilitySystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using IVisibilitySystemRequestBus = AZ::EBus<IVisibilitySystem, IVisibilitySystemRequests>;
}
