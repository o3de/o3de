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
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>


namespace PhysX
{
    class PhysXColliderPrefabTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
           
        }

        void TearDown() override
        {

        }


    };

    TEST_F(PhysXColliderPrefabTest, JsonStoreAndLoadPhysicsObjectsWithPrefabTest)
    {
        AzToolsFramework::Prefab::PrefabDom prefabDom;

        //material selection
        Physics::MaterialSelection materialSelection;
        AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), materialSelection);

        AzToolsFramework::Prefab::PrefabDomUtils::PrintPrefabDomValue("Material Selection", prefabDom);

        Physics::MaterialSelection newSelection;
        AZ::JsonSerialization::Load(newSelection, prefabDom);

        EXPECT_EQ(materialSelection.GetMaterialId(), newSelection.GetMaterialId());

        //collider configuration
        Physics::ColliderConfiguration colliderConfig;
        AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), colliderConfig);

        AzToolsFramework::Prefab::PrefabDomUtils::PrintPrefabDomValue("Collider Configuration", prefabDom);

        Physics::ColliderConfiguration newConfig;
        AZ::JsonSerialization::Load(newConfig, prefabDom);

        EXPECT_EQ(colliderConfig.m_collisionLayer, newConfig.m_collisionLayer);

        //shared pointer - collider configuration - defaults only
        auto colliderConfigPtr = AZStd::make_shared<Physics::ColliderConfiguration>();
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), colliderConfigPtr);

        AzToolsFramework::Prefab::PrefabDomUtils::PrintPrefabDomValue("Collider Configuration", prefabDom);

        colliderConfigPtr = nullptr;
        AZ::JsonSerialization::Load(colliderConfigPtr, prefabDom);

        EXPECT_NE(nullptr, colliderConfigPtr);

        //shared pointer - collider configuration - non default
        auto updatedColliderConfigPtr = AZStd::make_shared<Physics::ColliderConfiguration>();
        updatedColliderConfigPtr->m_isTrigger = true;
        AZ::JsonSerializationResult::ResultCode result2 = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), updatedColliderConfigPtr);

        AzToolsFramework::Prefab::PrefabDomUtils::PrintPrefabDomValue("Collider Configuration", prefabDom);

        updatedColliderConfigPtr = nullptr;
        AZ::JsonSerialization::Load(updatedColliderConfigPtr, prefabDom);

        EXPECT_NE(nullptr, updatedColliderConfigPtr);

        //shared pointer - shape configuration - defaults only
        auto shapeConfigPtr = AZStd::make_shared<Physics::SphereShapeConfiguration>();
        AZ::JsonSerializationResult::ResultCode result3 = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), shapeConfigPtr);

        AzToolsFramework::Prefab::PrefabDomUtils::PrintPrefabDomValue("Shape Configuration", prefabDom);

        shapeConfigPtr = nullptr;
        AZ::JsonSerialization::Load(shapeConfigPtr, prefabDom);

        EXPECT_NE(nullptr, shapeConfigPtr);

        //shared pointer - shape configuration - non default
        auto updatedShapeConfigPtr = AZStd::make_shared<Physics::SphereShapeConfiguration>();
        updatedShapeConfigPtr->m_radius = 2.0f;
        AZ::JsonSerializationResult::ResultCode result4 = AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), updatedShapeConfigPtr);

        AzToolsFramework::Prefab::PrefabDomUtils::PrintPrefabDomValue("Shape Configuration", prefabDom);

        updatedShapeConfigPtr = nullptr;
        AZ::JsonSerialization::Load(updatedShapeConfigPtr, prefabDom);

        EXPECT_NE(nullptr, updatedColliderConfigPtr);
    }
}
