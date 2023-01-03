/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>
#include <Components/TerrainWorldRendererComponent.h>

#include <AzTest/AzTest.h>

#include <TerrainTestFixtures.h>

class TerrainWorldRendererComponentTest
    : public UnitTest::TerrainSystemTestFixture
{
};

TEST_F(TerrainWorldRendererComponentTest, ComponentActivatesSuccessfully)
{
    auto entity = CreateEntity();

    entity->CreateComponent<Terrain::TerrainWorldComponent>();
    entity->CreateComponent<Terrain::TerrainWorldRendererComponent>();

    ActivateEntity(entity.get());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}
