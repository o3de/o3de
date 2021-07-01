/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! This class represents the layers a collider should collide with.
    //! For two colliders to collide, each layer must be present
    //! in the other colliders group.
    class CollisionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(CollisionGroup, "{E6DA080B-7ED1-4135-A78C-A6A5E495A43E}");
        static void Reflect(AZ::ReflectContext* context);

        static const CollisionGroup None; //!< Collide with nothing
        static const CollisionGroup All; //!< Collide with everything
        static const CollisionGroup All_NoTouchBend; //!< Collide with everything, except Touch Bendable Vegetation.

        //! Construct a Group with the given bitmask.
        //! The each bit in the bitmask corresponds to a CollisionLayer.
        //! @param mask The bitmask to assign to the group.
        CollisionGroup(AZ::u64 mask = All.GetMask());

        //! Construct a Group with the given name.
        //! This will lookup the group name to retrieve the group mask. If not found, CollisionGroup::All is set.
        //! @param groupName The name of the group to look up the group mask.
        CollisionGroup(const AZStd::string& groupName);

        //! Enable/Disable a CollisionLayer on this group.
        //! @param layer The layer to modify.
        //! @param set If true, toggle the layer ON for this group, otherwise toggle OFF.
        void SetLayer(CollisionLayer layer, bool set);

        //! Check is the given CollisionLayer is ON in this group.
        //! @param layer The layer to check.
        //! @return Returns true if the given layer is ON in this group, otherwise returns false.
        bool IsSet(CollisionLayer layer) const;

        //! Get the groups bitmask.
        AZ::u64 GetMask() const;

        bool operator==(const CollisionGroup& collisionGroup) const;
        bool operator!=(const CollisionGroup& collisionGroup) const;
    private:
        AZ::u64 m_mask;
    };

    //! Overloads for collision layers and groups.
    //! Example usage:
    //! @code{.cpp}
    //! CollisionGroup group1 = CollisionLayer(0) | CollisionLayer(1) | CollisionLayer(2).
    //! @endcode
    CollisionGroup operator|(CollisionLayer layer1, CollisionLayer layer2);
    CollisionGroup operator|(CollisionGroup group, CollisionLayer layer);

    //! Collision groups can be defined and edited in the PhysXConfiguration window.
    //! The idea is that collision groups are authored there, and then assigned to components via the
    //! edit context by reflecting Physics::CollisionGroups::Id, or alternatively can be retrieved by
    //! name from the CollisionConfiguration.
    class CollisionGroups
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(CollisionGroups, "{309B0B28-F51F-48E2-972E-DA7618ED7249}");
        static void Reflect(AZ::ReflectContext* context);

        //! Id of a collision group. Mainly used by the editor to assign a collision
        //! group to an editor component.
        class Id
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL;
            AZ_TYPE_INFO(Id, "{DFED4FE5-2292-4F07-A318-41C68DAEFE9C}");
            static void Reflect(AZ::ReflectContext* context);

            bool operator==(const Id& other) const { return m_id == other.m_id; }
            bool operator!=(const Id& other) const { return m_id != other.m_id; }
            bool operator<(const Id& other) const { return m_id < other.m_id; }
            static Id Create() { Id id; id.m_id = AZ::Uuid::Create(); return id; }
            AZ::Uuid m_id = AZ::Uuid::CreateNull();
        };

        //! A collision group defined with a name and an id which
        //! can be edited in the editor.
        struct Preset
        {
            AZ_CLASS_ALLOCATOR_DECL;
            AZ_TYPE_INFO(Preset, "{032D1485-60A4-45B6-8D74-5B38C929B066}");
            static void Reflect(AZ::ReflectContext* context);

            Id m_id;
            AZStd::string m_name;
            CollisionGroup m_group;
            bool m_readOnly;

            bool operator==(const Preset& other) const;
            bool operator!=(const Preset& other) const;
        };

        CollisionGroups() = default;

        //! Create a CollisionGroup.
        //! @param name The name to give the group.
        //! @param group The CollisionGroup data to set to this group.
        //! @param id A CollisionGroup::Id to assign to the group. By default will create a new Id.
        //! @param readOnly Mark the group as read only. Default false.
        //! @return Returns the CollisionGroup::Id associated with the new Group. 
        Id CreateGroup(const AZStd::string& name, CollisionGroup group, Id id = Id::Create(), bool readOnly = false);

        //! Delete a group with the given CollisionGroup::Id.
        void DeleteGroup(Id id);

        //! Set the name of a group with the given CollisionGroup::Id.
        void SetGroupName(Id id, const AZStd::string& groupName);

        //! Set a CollisionLayer ON or OFF on in the given CollisionGroup::Id.
        //! This will verify id is valid.
        //! @param id The group id to affect.
        //! @param layer The CollisionLayer to turn ON or OFF.
        //! @param enabled If true toggle the given CollisionLayer to ON, otherwise OFF
        void SetLayer(Id id, CollisionLayer layer, bool enabled);

        // Get a CollisionGroup by its id.
        //! @param id The CollisionGroup::Id to find.
        //! @return Returns the requested CollisionGroup, otherwise return CollisionGroup::All.
        CollisionGroup FindGroupById(Id id) const;

        // Get a CollisionGroup by its name.
        //! @param groupName The CollisionGroup name to find.
        //! @return Returns the requested CollisionGroup, otherwise return CollisionGroup::All.
        CollisionGroup FindGroupByName(const AZStd::string& groupName) const;

        // Get a CollisionGroup by its name.
        //! @param groupName The CollisionGroup name to find.
        //! @param group [Out] The requested CollisionGroup if successful. Otherwise group is unchanged.
        //! @return Returns true if located the requested CollisionGroup, otherwise false.
        bool TryFindGroupByName(const AZStd::string& groupName, CollisionGroup& group) const;

        //! Retrieve the CollisionGroup::Id of the request name.
        //! @param groupName The name of the CollisionGroup to lookup.
        //! @return Returns the request CollisionGroup::Id otherwise returns a 'null id'.
        Id FindGroupIdByName(const AZStd::string& groupName) const;

        //! Retrieve the name of the requested CollisionGroup::Id.
        //! @param id The CollisionGroup::Id to preform a name lookup for.
        //! @return The name of the CollisionGroup, otherwise returns an empty string.
        AZStd::string FindGroupNameById(Id id) const;

        //! Retrieve a list of all current Presets (see CollisionGroup::Preset).
        const AZStd::vector<Preset>& GetPresets()const;

        bool operator==(const CollisionGroups& other) const;
        bool operator!=(const CollisionGroups& other) const;
    private:
        AZStd::vector<Preset> m_groups;
    };
}
