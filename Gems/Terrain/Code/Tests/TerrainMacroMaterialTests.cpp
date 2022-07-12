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
#include <TerrainTestFixtures.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;


class TerrainMacroMaterialComponentTest
    : public UnitTest::TerrainTestFixture
{
};

TEST_F(TerrainMacroMaterialComponentTest, MissingRequiredComponentsActivateFailure)
{
    auto entity = CreateEntity();

    entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>();

    const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
    EXPECT_FALSE(sortOutcome.IsSuccess());

    entity.reset();
}

TEST_F(TerrainMacroMaterialComponentTest, RequiredComponentsPresentEntityActivateSuccess)
{
    constexpr float BoxHalfBounds = 128.0f;
    auto entity = CreateTestBoxEntity(BoxHalfBounds);

    entity->CreateComponent<Terrain::TerrainMacroMaterialComponent>();

    ActivateEntity(entity.get());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
     
    entity.reset();
}
