/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PhysX_precompiled.h>

#include <AzTest/AzTest.h>

#include <PhysX/SystemComponentBus.h>
#include <SphereColliderComponent.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <ShapeColliderComponent.h>

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>


namespace PhysX
{
    class PhysXColliderPrefabTests
        : public ::testing::Test
    {
    protected:
    };

    TEST_F(PhysXColliderPrefabTests, StoreAndLoad_DefaultPhysicsTypes_ValuesNotNull)
    {
        //create a prefab for storing data
        AzToolsFramework::Prefab::PrefabDom prefabDom;

        //material selection
        Physics::MaterialSelection materialSelection;
        AZ::JsonSerializationResult::ResultCode result
            = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), materialSelection);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        Physics::MaterialSelection newSelection;
        result = AZ::JsonSerialization::Load(newSelection, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_EQ(materialSelection.GetMaterialId(), newSelection.GetMaterialId());

        //collider configuration
        Physics::ColliderConfiguration colliderConfig;
        result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), colliderConfig);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());


        Physics::ColliderConfiguration newConfig;
        result = AZ::JsonSerialization::Load(newConfig, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_EQ(colliderConfig.m_collisionLayer, newConfig.m_collisionLayer);
    }

    TEST_F(PhysXColliderPrefabTests, StoreAndLoad_DefaultPhysicsTypes_PointersNotNull)
    {
        //create a prefab for storing data
        AzToolsFramework::Prefab::PrefabDom prefabDom;

        //shared pointer - collider configuration - defaults only
        auto colliderConfigPtr = AZStd::make_shared<Physics::ColliderConfiguration>();
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), colliderConfigPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        colliderConfigPtr = nullptr;
        result = AZ::JsonSerialization::Load(colliderConfigPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, colliderConfigPtr);

        //shared pointer - shape configuration - defaults only
        auto shapeConfigPtr = AZStd::make_shared<Physics::SphereShapeConfiguration>();
        result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), shapeConfigPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        shapeConfigPtr = nullptr;
        result = AZ::JsonSerialization::Load(shapeConfigPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, shapeConfigPtr);
    }

    TEST_F(PhysXColliderPrefabTests, StoreAndLoad_NonDefaultPhysicsTypes_PointersNotNull)
    {
        //create a prefab for storing data
        AzToolsFramework::Prefab::PrefabDom prefabDom;

        //shared pointer - collider configuration - non default
        auto updatedColliderConfigPtr = AZStd::make_shared<Physics::ColliderConfiguration>();
        updatedColliderConfigPtr->m_isTrigger = true;
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), updatedColliderConfigPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        updatedColliderConfigPtr = nullptr;
        result = AZ::JsonSerialization::Load(updatedColliderConfigPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, updatedColliderConfigPtr);

        //shared pointer - shape configuration - non default
        auto updatedShapeConfigPtr = AZStd::make_shared<Physics::SphereShapeConfiguration>();
        updatedShapeConfigPtr->m_radius = 2.0f;
        result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), updatedShapeConfigPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        updatedShapeConfigPtr = nullptr;
        result = AZ::JsonSerialization::Load(updatedShapeConfigPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, updatedColliderConfigPtr);
    }

    TEST_F(PhysXColliderPrefabTests, StoreAndLoad_DefaultPhysicsColliderComponents_PointersNotNull)
    {
        //create a prefab for storing data
        AzToolsFramework::Prefab::PrefabDom prefabDom;

        //shared pointer - box collider - defaults only
        auto boxColliderPtr = AZStd::make_shared<BoxColliderComponent>();
        AZ::JsonSerializationResult::ResultCode result
            = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), boxColliderPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        boxColliderPtr = nullptr;
        result = AZ::JsonSerialization::Load(boxColliderPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, boxColliderPtr);

        //shared pointer - sphere collider - defaults only
        auto sphereColliderPtr = AZStd::make_shared<SphereColliderComponent>();
        result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), sphereColliderPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        sphereColliderPtr = nullptr;
        result = AZ::JsonSerialization::Load(sphereColliderPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, sphereColliderPtr);

        //shared pointer - capsule collider - defaults only
        auto capsuleColliderPtr = AZStd::make_shared<CapsuleColliderComponent>();
        result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), capsuleColliderPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        capsuleColliderPtr = nullptr;
        result = AZ::JsonSerialization::Load(capsuleColliderPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, capsuleColliderPtr);

        //shared pointer - shape collider - defaults only
        auto shapeColliderPtr = AZStd::make_shared<ShapeColliderComponent>();
        result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), shapeColliderPtr);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());

        shapeColliderPtr = nullptr;
        result = AZ::JsonSerialization::Load(shapeColliderPtr, prefabDom);

        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, shapeColliderPtr);
    }
}
