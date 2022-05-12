/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/SurfaceDataTestFixtures.h>
#include <SurfaceData/Tests/SurfaceDataTestMocks.h>

#include <AzFramework/Components/TransformComponent.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <SurfaceData/Components/SurfaceDataColliderComponent.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>


namespace UnitTest
{
    void SurfaceDataTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "LmbrCentral" });

        AddComponentDescriptors({
            AzFramework::TransformComponent::CreateDescriptor(),

            SurfaceData::SurfaceDataSystemComponent::CreateDescriptor(),
            SurfaceData::SurfaceDataColliderComponent::CreateDescriptor(),
            SurfaceData::SurfaceDataShapeComponent::CreateDescriptor(),

            MockPhysicsColliderComponent::CreateDescriptor()
        });
    }
}

