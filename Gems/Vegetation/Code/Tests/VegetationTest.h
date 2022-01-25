/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>

/*
* testing fixtures for vegetation testing
*/
namespace UnitTest
{
    class VegetationComponentTests
        : public ScopedAllocatorSetupFixture
    {
    protected:
        VegetationComponentTests()
            : ScopedAllocatorSetupFixture(
                []() {
                    AZ::SystemAllocator::Descriptor desc;
                    desc.m_heap.m_fixedMemoryBlocksByteSize[0] = 20 * 1024 * 1024;
                    desc.m_stackRecordLevels = 20;
                    return desc;
                }()
            )
        {
        }

        AZ::ComponentApplication m_app;

        virtual void RegisterComponentDescriptors() {}

        void SetUp() override
        {
            if (AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::GetAllocator().GetRecords();
                records != nullptr)
            {
                records->SetMode(AZ::Debug::AllocationRecords::RECORD_NO_RECORDS);
            }

            m_app.Create({});
            RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        int m_createdCallbackCount = 0;
        int m_existedCallbackCount = 0;
        bool m_existedCallbackOutput = false;

        template <const AZStd::size_t CountX = 1, const AZStd::size_t CountY = 1>
        Vegetation::ClaimContext CreateContext(AZ::Vector3 startValue, float stepValue = 1.0f)
        {
            Vegetation::ClaimContext claimContext;

            for (auto x = 0; x < CountX; ++x)
            {
                for (auto y = 0; y < CountY; ++y)
                {
                    AZ::Vector3 value = startValue + AZ::Vector3(x * stepValue, y * stepValue, 0.0f);

                    size_t hash = 0;
                    AZStd::hash_combine<float>(hash, value.GetX());
                    AZStd::hash_combine<float>(hash, value.GetY());

                    Vegetation::ClaimPoint pt;
                    pt.m_position = value;
                    pt.m_handle = hash;
                    claimContext.m_availablePoints.push_back(pt);
                }
            }

            claimContext.m_createdCallback = [this](const Vegetation::ClaimPoint&, const Vegetation::InstanceData&) 
            {
                ++m_createdCallbackCount;
            };

            claimContext.m_existedCallback = [this](const Vegetation::ClaimPoint&, const Vegetation::InstanceData&) 
            { 
                return m_existedCallbackOutput;
            };

            return claimContext;
        }

        template <typename Component>
        AZStd::unique_ptr<AZ::Entity> CreateEntity(Component** ppComponent)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            auto entity = AZStd::make_unique<AZ::Entity>();

            if (ppComponent)
            {
                *ppComponent = entity->CreateComponent<Component>();
            }
            else
            {
                entity->CreateComponent<Component>();
            }

            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

            return entity;
        }

        template <typename Component, typename Configuration>
        AZStd::unique_ptr<AZ::Entity> CreateEntity(const Configuration& config, Component** ppComponent)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            auto entity = AZStd::make_unique<AZ::Entity>();

            if (ppComponent)
            {
                *ppComponent = entity->CreateComponent<Component>(config);
            }
            else
            {
                entity->CreateComponent<Component>(config);
            }

            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

            return entity;
        }

        using CreateAdditionalComponents = AZStd::function<void(AZ::Entity*)>;

        template <typename Component, typename Configuration>
        AZStd::unique_ptr<AZ::Entity> CreateEntity(const Configuration& config, Component** ppComponent, CreateAdditionalComponents fnCreateAdditionalComponents)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            auto entity = AZStd::make_unique<AZ::Entity>();

            if (ppComponent)
            {
                *ppComponent = entity->CreateComponent<Component>(config);
            }
            else
            {
                entity->CreateComponent<Component>(config);
            }

            fnCreateAdditionalComponents(entity.get());

            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

            return entity;
        }

        void DestroyEntity(AZ::Entity* entity)
        {
            entity->Deactivate();
            delete entity;
        }
    };
}
