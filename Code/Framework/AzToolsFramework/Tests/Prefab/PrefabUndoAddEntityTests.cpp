/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabUndoAddEntityTestFixture.h>
#include <Prefab/Undo/PrefabUndoAddEntity.h>

#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Components/TransformComponent.h>


namespace UnitTest
{
    using PrefabUndoAddEntityTests = PrefabUndoAddEntityTestFixture;

    TEST_F(PrefabUndoAddEntityTests, PrefabUndoAddEntity_AddEntityUnderFocusedInstance)
    {
        // Create a car instance as our current focused instance.
        AZStd::unique_ptr<Instance> focusedCarInstancePtr =
            m_prefabSystemComponent->CreatePrefab({}, {}, "test/path");
        ASSERT_TRUE(focusedCarInstancePtr);
        Instance& focusedCarInstance = *focusedCarInstancePtr;

        // Create another car instance to later help verify if propagation works.
        AZStd::unique_ptr<Instance> secondCarInstancePtr =
            m_prefabSystemComponent->InstantiatePrefab(focusedCarInstance.GetTemplateId());
        ASSERT_TRUE(secondCarInstancePtr);

        auto carinstances = MakeInstanceList(
            AZStd::move(focusedCarInstancePtr),
            AZStd::move(secondCarInstancePtr));

        // Create a car entity and add it under our car instance.
        const AZStd::string carEntityName = "Car";
        const EntityAlias carEntityAlias = CreateEntity(carEntityName, focusedCarInstance);
        ASSERT_FALSE(carEntityAlias.empty());

        // Create undo/redo node for adding the car entity under the car instance.
        auto createUndoAddCarEntityNodeResult = CreatePrefabUndoAddEntityNode(
            focusedCarInstance, carEntityAlias, focusedCarInstance, "Undo Adding Car Entity");
        ASSERT_TRUE(createUndoAddCarEntityNodeResult.IsSuccess());
        PrefabUndoAddEntity undoAddCarEntityNode = createUndoAddCarEntityNodeResult.TakeValue();

        size_t expectedEntityCount = 0;

        // Adding the car entity under the car instance by redoing with our undo node and doing template propagation.
        undoAddCarEntityNode.Redo();
        PropagateAllTemplateChanges();
        
        ValidateNewEntityUnderInstance(carinstances,
            carEntityAlias, carEntityName,
            ++expectedEntityCount);

        // Create an axis entity and add it under the car entity.
        const AZStd::string axisEntityName = "Axis";
        const EntityAlias axisEntityAlias = CreateEntity(axisEntityName, focusedCarInstance, carEntityAlias);
        ASSERT_FALSE(axisEntityAlias.empty());

        // Create undo/redo node for adding the axis entity under the car entity.
        auto createUndoAddAxisEntityNodeResult = CreatePrefabUndoAddEntityNode(
            focusedCarInstance, carEntityAlias, axisEntityAlias, focusedCarInstance, "Undo Adding Axis Entity");
        ASSERT_TRUE(createUndoAddAxisEntityNodeResult.IsSuccess());
        PrefabUndoAddEntity undoAddAxisEntityNode = createUndoAddAxisEntityNodeResult.TakeValue();

        // Adding the axis entity under the car entity by redoing with our undo node and doing template propagation.
        undoAddAxisEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(carinstances,
            carEntityAlias, carEntityName,
            axisEntityAlias, axisEntityName,
            ++expectedEntityCount);

        // Create a wheel entity and add it under the axis entity.
        const AZStd::string wheelEntityName = "Wheel";
        const EntityAlias wheelEntityAlias = CreateEntity(wheelEntityName, focusedCarInstance, axisEntityAlias);
        ASSERT_FALSE(wheelEntityAlias.empty());

        // Create undo/redo node for adding the wheel entity under the axis entity.
        auto createUndoAddWheelEntityNodeResult = CreatePrefabUndoAddEntityNode(
            focusedCarInstance, axisEntityAlias, wheelEntityAlias, focusedCarInstance, "Undo Adding Wheel Entity");
        ASSERT_TRUE(createUndoAddWheelEntityNodeResult.IsSuccess());
        PrefabUndoAddEntity undoAddWheelEntityNode = createUndoAddWheelEntityNodeResult.TakeValue();

        // Adding the wheel entity under the axis entity by redoing with our undo node and doing template propagation.
        undoAddWheelEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(carinstances,
            axisEntityAlias, axisEntityName,
            wheelEntityAlias, wheelEntityName,
            ++expectedEntityCount);

        // Undo adding the wheel entity under the axis entity.
        undoAddWheelEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(carinstances,
            axisEntityAlias, axisEntityName,
            wheelEntityAlias, 
            --expectedEntityCount);

        // Undo adding the axis entity under the car entity.
        undoAddAxisEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(carinstances,
            carEntityAlias, carEntityName,
            axisEntityAlias,
            --expectedEntityCount);

        // Undo adding the car entity under the car instance.
        undoAddCarEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderInstance(carinstances,
            carEntityAlias,
            --expectedEntityCount);

        // Redo all adding entity operations.
        undoAddCarEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        undoAddAxisEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        undoAddWheelEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(carinstances,
            axisEntityAlias, axisEntityName,
            wheelEntityAlias, wheelEntityName,
            ++expectedEntityCount);
    }
}
