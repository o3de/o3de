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
#include <TerrainTestFixtures.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

namespace UnitTest
{
    class TerrainSurfaceMaterialsListTest
        : public TerrainTestFixture
    {
    protected:
        constexpr static inline float TestBoxHalfBounds = 128.0f;
    };

    TEST_F(TerrainSurfaceMaterialsListTest, SurfaceMaterialsListRequiresShapeToActivate)
    {
        // Check that the component requires a shape service to activate: trying to Activate the entity will cause the test to fail, so
        // use the EvaluateDependenciesGetDetails function to check the dependencies are met.
        auto entity = CreateEntity();

        entity->CreateComponent<Terrain::TerrainSurfaceMaterialsListComponent>();

        const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());

        entity.reset();
    }

    TEST_F(TerrainSurfaceMaterialsListTest, SurfaceMaterialsListActivatesSuccessfully)
    {
        auto entity = CreateTestBoxEntity(TestBoxHalfBounds);

        entity->CreateComponent<Terrain::TerrainSurfaceMaterialsListComponent>();

        ActivateEntity(entity.get());

        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
                
        entity.reset();
    }
} // namespace UnitTest
