/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>

#include <AzTest/AzTest.h>

#include <TerrainTestFixtures.h>

class TerrainWorldComponentTest
    : public UnitTest::TerrainTestFixture
{
};

TEST_F(TerrainWorldComponentTest, ComponentActivatesSuccessfully)
{
    auto entity = CreateEntity();

    entity->CreateComponent<Terrain::TerrainWorldComponent>();

    ActivateEntity(entity.get());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}
