/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>
#include <MockAxisAlignedBoxShapeComponent.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

namespace UnitTest
{
    class TerrainSurfaceMaterialsListTest : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            auto entity = AZStd::make_unique<AZ::Entity>();
            entity->Init();
            return entity;
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntityWithShapeComponents()
        {
            auto entity = CreateEntity();

            auto shapeComponent = entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
            m_app.RegisterComponentDescriptor(shapeComponent->CreateDescriptor());

            return entity;
        }

        Terrain::TerrainSurfaceMaterialsListComponent* AddSurfaceMaterialListComponent(AZ::Entity* entity)
        {
            auto surfaceMaterialsListComponent = entity->CreateComponent<Terrain::TerrainSurfaceMaterialsListComponent>();
            m_app.RegisterComponentDescriptor(surfaceMaterialsListComponent->CreateDescriptor());

            return surfaceMaterialsListComponent;
        }

        void TearDown() override
        {
            m_app.Destroy();
        }
    };

    TEST_F(TerrainSurfaceMaterialsListTest, SurfaceGradientListRequiresShapeToActivate)
    {
        auto entity = CreateEntity();

        AddSurfaceMaterialListComponent(entity.get());

        const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());

        entity.reset();
    }

    TEST_F(TerrainSurfaceMaterialsListTest, SurfaceGradientListActivatesSuccessfully)
    {
        auto entity = CreateEntityWithShapeComponents();

        AddSurfaceMaterialListComponent(entity.get());

        entity->Activate();

        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                
        entity.reset();
    }
} // namespace UnitTest
