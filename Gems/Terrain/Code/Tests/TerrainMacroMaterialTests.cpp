/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzTest/AzTest.h>

#include <Terrain/MockTerrain.h>
#include <MockAxisAlignedBoxShapeComponent.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;


class TerrainMacroMaterialComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    UnitTest::MockAxisAlignedBoxShapeComponent* m_shapeComponent;

    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
        appDesc.m_stackRecordLevels = 20;

        m_app.Create(appDesc);
    }

    void TearDown() override
    {
        m_app.Destroy();
    }

    AZStd::unique_ptr<AZ::Entity> CreateEntity()
    {
        auto entity = AZStd::make_unique<AZ::Entity>();
        entity->Init();

        return entity;
    }
};

TEST_F(TerrainMacroMaterialComponentTest, MissingRequiredComponentsActivateFailure)
{
    auto entity = CreateEntity();

    auto macroMaterialComponent = entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>();
    m_app.RegisterComponentDescriptor(macroMaterialComponent->CreateDescriptor());

    const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
    EXPECT_FALSE(sortOutcome.IsSuccess());

    entity.reset();
}

TEST_F(TerrainMacroMaterialComponentTest, RequiredComponentsPresentEntityActivateSuccess)
{
    auto entity = CreateEntity();

    auto macroMaterialComponent = entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>();
    m_app.RegisterComponentDescriptor(macroMaterialComponent->CreateDescriptor());

    auto shapeComponent = entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
    m_app.RegisterComponentDescriptor(shapeComponent->CreateDescriptor());

    entity->Activate();
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
     
    entity.reset();
}
