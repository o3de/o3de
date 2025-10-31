/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/hash.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>

namespace UnitTest
{
    struct SurfaceDataTest
        : public ::testing::Test
    {
    protected:
        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            return AZStd::make_unique<AZ::Entity>();
        }

        void ActivateEntity(AZ::Entity* entity)
        {
            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());
        }

        template <typename Component, typename Configuration>
        AZ::Component* CreateComponent(AZ::Entity* entity, const Configuration& config)
        {
            return entity->CreateComponent<Component>(config);
        }

        template <typename Component>
        AZ::Component* CreateComponent(AZ::Entity* entity)
        {
            return entity->CreateComponent<Component>();
        }
    };

    struct MockShapeComponentHandler
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
        MockShapeComponentHandler(const AZ::EntityId& id)
        {
            BusConnect(id);
        }

        ~MockShapeComponentHandler() override
        {
            BusDisconnect();
        }

        AZ::Aabb m_GetLocalBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        AZ::Transform m_GetTransform = AZ::Transform::CreateIdentity();
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override
        {
            transform = m_GetTransform;
            bounds = m_GetLocalBounds;
        }

        AZ::Crc32 m_GetShapeType = AZ_CRC_CE("MockShapeComponentHandler");
        AZ::Crc32 GetShapeType() const override
        {
            return m_GetShapeType;
        }

        AZ::Aabb m_GetEncompassingAabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        AZ::Aabb GetEncompassingAabb() const override
        {
            return m_GetEncompassingAabb;
        }

        bool IsPointInside(const AZ::Vector3& point) const override
        {
            return m_GetEncompassingAabb.Contains(point);
        }

        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override
        {
            return m_GetEncompassingAabb.GetDistanceSq(point);
        }
    };

    struct MockShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockShapeComponent, "{DD9590BC-916B-4EFA-98B8-AC5023941672}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ShapeService"));
        }
    };

    // Mock out a generic Physics Collider Component, which is a required dependency for adding a SurfaceDataColliderComponent.
    struct MockPhysicsColliderComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockPhysicsColliderComponent, "{4F7C36DE-6475-4E0A-96A7-BFAF21C07C95}", AZ::Component);

        void Activate() override
        {
        }
        void Deactivate() override
        {
        }

        static void Reflect(AZ::ReflectContext* reflect)
        {
            AZ_UNUSED(reflect);
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        }
    };

    struct MockTransformHandler
        : public AZ::TransformBus::Handler
    {
        ~MockTransformHandler()
        {
            AZ::TransformBus::Handler::BusDisconnect();
        }

        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override
        {
            ;
        }

        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override
        {
            ;
        }

        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override
        {
            ;
        }

        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override
        {
            ;
        }

        AZ::Transform m_GetLocalTMOutput = AZ::Transform::CreateIdentity();
        const AZ::Transform & GetLocalTM() override
        {
            return m_GetLocalTMOutput;
        }

        AZ::Transform m_GetWorldTMOutput = AZ::Transform::CreateIdentity();
        const AZ::Transform & GetWorldTM() override
        {
            return m_GetWorldTMOutput;
        }

        bool IsStaticTransform() override
        {
            return false;
        }
    };

    struct MockSurfaceDataSystem
        : public SurfaceData::SurfaceDataSystemRequestBus::Handler
    {
        MockSurfaceDataSystem()
        {
            AZ::Interface<SurfaceDataSystem>::Register(this);
            BusConnect();
        }

        ~MockSurfaceDataSystem()
        {
            BusDisconnect();
            AZ::Interface<SurfaceDataSystem>::Unregister(this);
        }

        AZStd::unordered_map<AZStd::pair<float, float>, SurfaceData::SurfacePointList> m_surfacePoints;
        void GetSurfacePoints(const AZ::Vector3& inPosition, [[maybe_unused]] const SurfaceData::SurfaceTagVector& masks, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            auto surfacePoints = m_surfacePoints.find(AZStd::make_pair(inPosition.GetX(), inPosition.GetY()));

            if (surfacePoints != m_surfacePoints.end())
            {
                surfacePointList = surfacePoints->second;
            }
        }

        void GetSurfacePointsFromRegion([[maybe_unused]] const AZ::Aabb& inRegion, [[maybe_unused]] const AZ::Vector2 stepSize, [[maybe_unused]] const SurfaceData::SurfaceTagVector& desiredTags,
            [[maybe_unused]] SurfaceData::SurfacePointList& surfacePointListPerPosition) const override
        {
        }

        void GetSurfacePointsFromList(
            [[maybe_unused]] AZStd::span<const AZ::Vector3> inPositions,
            [[maybe_unused]] const SurfaceData::SurfaceTagVector& desiredTags,
            [[maybe_unused]] SurfaceData::SurfacePointList& surfacePointLists) const override
        {
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            return RegisterEntry(entry, m_providers);
        }

        void UnregisterSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            UnregisterEntry(handle, m_providers);
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            return RegisterEntry(entry, m_modifiers);
        }

        void UnregisterSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            UnregisterEntry(handle, m_modifiers);
        }

        void UpdateSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            UpdateEntry(handle, entry, m_providers);
        }

        void UpdateSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            UpdateEntry(handle, entry, m_modifiers);
        }

        void RefreshSurfaceData([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& providerHandle,
            [[maybe_unused]] const AZ::Aabb& dirtyBounds) override
        {
        }

        SurfaceData::SurfaceDataRegistryHandle GetSurfaceDataProviderHandle(const AZ::EntityId& providerEntityId) override
        {
            return GetSurfaceProviderHandle(providerEntityId);
        }

        SurfaceData::SurfaceDataRegistryHandle GetSurfaceDataModifierHandle(const AZ::EntityId& modifierEntityId) override
        {
            return GetSurfaceModifierHandle(modifierEntityId);
        }

        SurfaceData::SurfaceDataRegistryHandle GetSurfaceProviderHandle(AZ::EntityId id)
        {
            return GetEntryHandle(id, m_providers);
        }

        SurfaceData::SurfaceDataRegistryHandle GetSurfaceModifierHandle(AZ::EntityId id)
        {
            return GetEntryHandle(id, m_modifiers);
        }

        SurfaceData::SurfaceDataRegistryEntry GetSurfaceProviderEntry(AZ::EntityId id)
        {
            return GetEntry(id, m_providers);
        }

        SurfaceData::SurfaceDataRegistryEntry GetSurfaceModifierEntry(AZ::EntityId id)
        {
            return GetEntry(id, m_modifiers);
        }
    protected:
        AZStd::vector<SurfaceData::SurfaceDataRegistryEntry> m_providers;
        AZStd::vector<SurfaceData::SurfaceDataRegistryEntry> m_modifiers;

        SurfaceData::SurfaceDataRegistryHandle RegisterEntry(const SurfaceData::SurfaceDataRegistryEntry& entry, AZStd::vector<SurfaceData::SurfaceDataRegistryEntry>& entryList)
        {
            // Keep a list of registered entries.  Use the "list index + 1" as the handle.  (We add +1 because 0 is used to mean "invalid handle")
            entryList.emplace_back(entry);
            return static_cast<SurfaceData::SurfaceDataRegistryHandle>(entryList.size());
        }

        void UnregisterEntry(const SurfaceData::SurfaceDataRegistryHandle& handle, AZStd::vector<SurfaceData::SurfaceDataRegistryEntry>& entryList)
        {
            // We don't actually remove the entry from our list because we use handles as indices, so the indices can't change.
            // Clearing out the entity Id should be good enough.
            uint32_t index = static_cast<uint32_t>(handle) - 1;
            if (index < entryList.size())
            {
                entryList[index].m_entityId = AZ::EntityId();
            }
        }

        void UpdateEntry(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry,
                         AZStd::vector<SurfaceData::SurfaceDataRegistryEntry>& entryList)
        {
            uint32_t index = static_cast<uint32_t>(handle) - 1;
            if (index < entryList.size())
            {
                entryList[index] = entry;
            }

        }

        SurfaceData::SurfaceDataRegistryHandle GetEntryHandle(AZ::EntityId id, const AZStd::vector<SurfaceData::SurfaceDataRegistryEntry>& entryList)
        {
            // Look up the requested entity Id and see if we have a registered surface entry with that handle.  If so, return the handle.
            auto result = AZStd::find_if(entryList.begin(), entryList.end(), [id](const SurfaceData::SurfaceDataRegistryEntry& entry) { return entry.m_entityId == id; });
            if (result == entryList.end())
            {
                return SurfaceData::InvalidSurfaceDataRegistryHandle;
            }
            else
            {
                // We use "index + 1" as our handle
                return static_cast<SurfaceData::SurfaceDataRegistryHandle>(result - entryList.begin() + 1);
            }

        }

        SurfaceData::SurfaceDataRegistryEntry GetEntry(AZ::EntityId id, const AZStd::vector<SurfaceData::SurfaceDataRegistryEntry>& entryList)
        {
            SurfaceData::SurfaceDataRegistryHandle handle = GetEntryHandle(id, entryList);
            if (handle != SurfaceData::InvalidSurfaceDataRegistryHandle)
            {
                return entryList[handle - 1];
            }
            else
            {
                SurfaceData::SurfaceDataRegistryEntry emptyEntry;
                return emptyEntry;
            }
        }
    };
}
