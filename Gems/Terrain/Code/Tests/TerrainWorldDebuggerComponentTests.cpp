/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>
#include <Components/TerrainWorldDebuggerComponent.h>

#include <AzTest/AzTest.h>

#include <TerrainTestFixtures.h>

class TerrainWorldDebuggerComponentTest
    : public UnitTest::TerrainTestFixture
{
};

TEST_F(TerrainWorldDebuggerComponentTest, MissingRequiredComponentsActivateFailure)
{
    auto entity = CreateEntity();

    entity->CreateComponent<Terrain::TerrainWorldDebuggerComponent>();

    // This should report failure because it depends on a missing TerrainWorldCmponent.
    const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
    EXPECT_FALSE(sortOutcome.IsSuccess());

    entity.reset();
}

TEST_F(TerrainWorldDebuggerComponentTest, ComponentActivatesSuccessfully)
{
    auto entity = CreateEntity();

    entity->CreateComponent<Terrain::TerrainWorldComponent>();
    entity->CreateComponent<Terrain::TerrainWorldDebuggerComponent>();

    ActivateEntity(entity.get());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}
