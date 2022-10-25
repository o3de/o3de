/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabUndoAddEntityTestFixture.h>

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
        Instance& secondCarInstance = *secondCarInstancePtr;

        InstanceList carinstances{focusedCarInstance,  secondCarInstance};

        // Create a car entity and add it under our car instance.
        const AZStd::string carEntityName = "Car";
        const EntityAlias carEntityAlias = CreateEntity(carEntityName, focusedCarInstance);
        ASSERT_FALSE(carEntityAlias.empty());

        // Create undo/redo node for adding the car entity under the car instance.
        auto createUndoAddCarEntityNodeResult = CreateFocusedInstanceAddEntityUndoNode(
            carEntityAlias, focusedCarInstance, "Undo Adding Car Entity");
        ASSERT_TRUE(createUndoAddCarEntityNodeResult.IsSuccess());
        PrefabUndoFocusedInstanceAddEntity undoAddCarEntityNode = createUndoAddCarEntityNodeResult.TakeValue();

        size_t expectedEntityCount = 0;

        // Adding the car entity under the car instance by redoing with our undo node and doing template propagation.
        undoAddCarEntityNode.Redo();
        PropagateAllTemplateChanges();
        
        ValidateNewEntityUnderInstance(carinstances, carEntityAlias, carEntityName, ++expectedEntityCount);

        // Create an axis entity and add it under the car entity.
        const AZStd::string axisEntityName = "Axis";
        const EntityAlias axisEntityAlias = CreateEntity(axisEntityName, focusedCarInstance, carEntityAlias);
        ASSERT_FALSE(axisEntityAlias.empty());

        // Create undo/redo node for adding the axis entity under the car entity.
        auto createUndoAddAxisEntityNodeResult = CreateFocusedInstanceAddEntityUndoNode(
            carEntityAlias, axisEntityAlias, focusedCarInstance, "Undo Adding Axis Entity");
        ASSERT_TRUE(createUndoAddAxisEntityNodeResult.IsSuccess());
        PrefabUndoFocusedInstanceAddEntity undoAddAxisEntityNode = createUndoAddAxisEntityNodeResult.TakeValue();

        // Adding the axis entity under the car entity by redoing with our undo node and doing template propagation.
        undoAddAxisEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(carinstances, carEntityAlias, carEntityName,
            axisEntityAlias, axisEntityName, ++expectedEntityCount);

        // Create a wheel entity and add it under the axis entity.
        const AZStd::string wheelEntityName = "Wheel";
        const EntityAlias wheelEntityAlias = CreateEntity(wheelEntityName, focusedCarInstance, axisEntityAlias);
        ASSERT_FALSE(wheelEntityAlias.empty());

        // Create undo/redo node for adding the wheel entity under the axis entity.
        auto createUndoAddWheelEntityNodeResult = CreateFocusedInstanceAddEntityUndoNode(
            axisEntityAlias, wheelEntityAlias, focusedCarInstance, "Undo Adding Wheel Entity");
        ASSERT_TRUE(createUndoAddWheelEntityNodeResult.IsSuccess());
        PrefabUndoFocusedInstanceAddEntity undoAddWheelEntityNode = createUndoAddWheelEntityNodeResult.TakeValue();

        // Adding the wheel entity under the axis entity by redoing with our undo node and doing template propagation.
        undoAddWheelEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(carinstances, axisEntityAlias, axisEntityName,
            wheelEntityAlias, wheelEntityName, ++expectedEntityCount);

        // Undo adding the wheel entity under the axis entity.
        undoAddWheelEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(carinstances, axisEntityAlias, axisEntityName,
            wheelEntityAlias, --expectedEntityCount);

        // Undo adding the axis entity under the car entity.
        undoAddAxisEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(carinstances, carEntityAlias, carEntityName,
            axisEntityAlias, --expectedEntityCount);

        // Undo adding the car entity under the car instance.
        undoAddCarEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderInstance(carinstances, carEntityAlias, --expectedEntityCount);

        // Redo all adding entity operations.
        undoAddCarEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        undoAddAxisEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        undoAddWheelEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(carinstances, axisEntityAlias, axisEntityName,
            wheelEntityAlias, wheelEntityName, ++expectedEntityCount);
    }

    TEST_F(PrefabUndoAddEntityTests, PrefabUndoAddEntity_AddEntityUnderUnfocusedInstance)
    {
        // Create a wheel instance.
        AZStd::unique_ptr<Instance> wheelInstancePtr =
            m_prefabSystemComponent->CreatePrefab({}, {}, "test/path/wheel");
        ASSERT_TRUE(wheelInstancePtr);
        Instance& leftWheelInstance = *wheelInstancePtr;
        const InstanceAlias leftWheelInstanceAlias = wheelInstancePtr->GetInstanceAlias();

        // Create another wheel instance to be added under an axis instance later.
        AZStd::unique_ptr<Instance> secondWheelInstancePtr =
            m_prefabSystemComponent->InstantiatePrefab(wheelInstancePtr->GetTemplateId());
        ASSERT_TRUE(secondWheelInstancePtr);
        const InstanceAlias rightWheelInstanceAlias = secondWheelInstancePtr->GetInstanceAlias();

        // Create an axis instance which includes two wheel instances under it.
        AZStd::unique_ptr<Instance> axisInstancePtr =
            m_prefabSystemComponent->CreatePrefab({},
                MakeInstanceList(AZStd::move(wheelInstancePtr), AZStd::move(secondWheelInstancePtr)),
                "test/path/axis");
        ASSERT_TRUE(axisInstancePtr);
        const InstanceAlias axisInstanceAlias = axisInstancePtr->GetInstanceAlias();

        // Create a car instance which includes one axis instance under it.
        AZStd::unique_ptr<Instance> carInstancePtr =
            m_prefabSystemComponent->CreatePrefab({},
                MakeInstanceList(AZStd::move(axisInstancePtr)),
                "test/path/car");
        ASSERT_TRUE(carInstancePtr);
        Instance& focusedCarInstance = *carInstancePtr;

        // Create a left wheel entity and add it under our left wheel instance.
        const AZStd::string leftWheelEntityName = "LeftWheel";
        const EntityAlias leftWheelEntityAlias = CreateEntity(leftWheelEntityName, leftWheelInstance);
        ASSERT_FALSE(leftWheelEntityAlias.empty());

        // Create undo/redo node for adding the left wheel entity under the left wheel instance.
        auto createUndoAddLeftWheelEntityNodeResult = CreateUnfocusedInstanceAddEntityUndoNode(
            leftWheelEntityAlias, leftWheelInstance, focusedCarInstance, "Undo Adding Left Wheel Entity");
        ASSERT_TRUE(createUndoAddLeftWheelEntityNodeResult.IsSuccess());
        PrefabUndoUnfocusedInstanceAddEntity undoAddLeftWheelEntityNode = createUndoAddLeftWheelEntityNodeResult.TakeValue();

        size_t expectedLeftWheelInstanceEntityCount = 0;
        size_t expectedRightWheelInstanceEntityCount = 0;

        // Adding the left wheel entity under the left wheel instance by redoing with our undo node and doing template propagation.
        undoAddLeftWheelEntityNode.Redo();
        PropagateAllTemplateChanges();

        auto findAxisInstanceResult = focusedCarInstance.FindNestedInstance(axisInstanceAlias);
        ASSERT_TRUE(findAxisInstanceResult.has_value());
        Instance& axisInstance = findAxisInstanceResult->get();

        auto findRightWheelInstanceResult = axisInstance.FindNestedInstance(rightWheelInstanceAlias);
        ASSERT_TRUE(findRightWheelInstanceResult.has_value());
        Instance& rightWheelInstance = findRightWheelInstanceResult->get();

        // We should see the left wheel entity be added under left wheel instance.
        ValidateNewEntityUnderInstance(
            leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName, ++expectedLeftWheelInstanceEntityCount);

        // The focused instance is the car instance, which is the ancestor instance of
        // the owning instance of our new wheel entity (left wheel instance).
        // After propagation, we should see the new wheel entity has been added under 
        // left wheel instance, not right wheel instance.
        ValidateNewEntityNotUnderInstance(
            rightWheelInstance, leftWheelEntityAlias, expectedRightWheelInstanceEntityCount);

        // Create a tire entity and add it under our left wheel entity.
        const AZStd::string tireEntityName = "Tire";
        const EntityAlias tireEntityAlias = CreateEntity(tireEntityName, leftWheelInstance, leftWheelEntityAlias);
        ASSERT_FALSE(tireEntityAlias.empty());

        // Create undo/redo node for adding the tire entity under the left wheel entity.
        auto createUndoAddTireEntityNodeResult = CreateUnfocusedInstanceAddEntityUndoNode(
            leftWheelEntityAlias, tireEntityAlias, leftWheelInstance, focusedCarInstance, "Undo Adding Tire Entity");
        ASSERT_TRUE(createUndoAddTireEntityNodeResult.IsSuccess());
        PrefabUndoUnfocusedInstanceAddEntity undoAddTireEntityNode = createUndoAddTireEntityNodeResult.TakeValue();

        // Adding the tire entity under the left wheel entity by redoing with our undo node and doing template propagation.
        undoAddTireEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            tireEntityAlias, tireEntityName, ++expectedLeftWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(rightWheelInstance, tireEntityAlias, expectedRightWheelInstanceEntityCount);

        // Create a frame entity and add it under our left wheel entity.
        const AZStd::string frameEntityName = "Frame";
        const EntityAlias frameEntityAlias = CreateEntity(frameEntityName, leftWheelInstance, leftWheelEntityAlias);
        ASSERT_FALSE(frameEntityAlias.empty());

        // Create undo/redo node for adding the frame entity under the left wheel entity.
        auto createUndoAddFrameEntityNodeResult = CreateUnfocusedInstanceAddEntityUndoNode(
            leftWheelEntityAlias, frameEntityAlias, leftWheelInstance, focusedCarInstance, "Undo Adding Frame Entity");
        ASSERT_TRUE(createUndoAddFrameEntityNodeResult.IsSuccess());
        PrefabUndoUnfocusedInstanceAddEntity undoAddFrameEntityNode = createUndoAddFrameEntityNodeResult.TakeValue();

        // Adding the tire entity under the left wheel entity by redoing with our undo node and doing template propagation.
        undoAddFrameEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            frameEntityAlias, frameEntityName, ++expectedLeftWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(rightWheelInstance, frameEntityAlias, expectedRightWheelInstanceEntityCount);

        // Create a nail entity and add it under our tire entity.
        const AZStd::string nailEntityName = "Nail";
        const EntityAlias nailEntityAlias = CreateEntity(nailEntityName, leftWheelInstance, tireEntityAlias);
        ASSERT_FALSE(nailEntityAlias.empty());

        // Create undo/redo node for adding the nail entity under the tire entity.
        auto createUndoAddNailEntityNodeResult = CreateUnfocusedInstanceAddEntityUndoNode(
            tireEntityAlias, nailEntityAlias, leftWheelInstance, focusedCarInstance, "Undo Adding Nail Entity");
        ASSERT_TRUE(createUndoAddNailEntityNodeResult.IsSuccess());
        PrefabUndoUnfocusedInstanceAddEntity undoAddNailEntityNode = createUndoAddNailEntityNodeResult.TakeValue();

        // Adding the tire entity under the left wheel entity by redoing with our undo node and doing template propagation.
        undoAddNailEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(leftWheelInstance, tireEntityAlias, tireEntityName,
            nailEntityAlias, nailEntityName, ++expectedLeftWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(rightWheelInstance, nailEntityAlias, expectedRightWheelInstanceEntityCount);

        // Undo adding the nail entity under the tire entity.
        undoAddNailEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(leftWheelInstance, tireEntityAlias, tireEntityName,
            nailEntityAlias, --expectedLeftWheelInstanceEntityCount);

        // Undo adding the frame entity under the left wheel entity.
        undoAddFrameEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            frameEntityAlias, --expectedLeftWheelInstanceEntityCount);

        // Undo adding the tire entity under the left wheel entity.
        undoAddTireEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            tireEntityAlias, --expectedLeftWheelInstanceEntityCount);

        // Undo adding the left wheel entity under the left wheel instance.
        undoAddLeftWheelEntityNode.Undo();
        PropagateAllTemplateChanges();

        InstanceList wheelInstances = { leftWheelInstance, rightWheelInstance };
        ValidateNewEntityNotUnderInstance(wheelInstances, leftWheelEntityAlias, --expectedLeftWheelInstanceEntityCount);

        // Redo all adding entity operations.
        undoAddLeftWheelEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        undoAddTireEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        undoAddFrameEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        undoAddNailEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderParentEntity(leftWheelInstance, tireEntityAlias, tireEntityName,
            nailEntityAlias, nailEntityName, ++expectedLeftWheelInstanceEntityCount);
        ValidateNewEntityUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            frameEntityAlias, frameEntityName, expectedLeftWheelInstanceEntityCount);
        ValidateNewEntityUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            tireEntityAlias, tireEntityName, expectedLeftWheelInstanceEntityCount);
        ValidateNewEntityUnderInstance(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            expectedLeftWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(rightWheelInstance, leftWheelEntityAlias, expectedRightWheelInstanceEntityCount);
        ValidateNewEntityNotUnderInstance(rightWheelInstance, tireEntityAlias, expectedRightWheelInstanceEntityCount);
        ValidateNewEntityNotUnderInstance(rightWheelInstance, frameEntityAlias, expectedRightWheelInstanceEntityCount);
        ValidateNewEntityNotUnderInstance(rightWheelInstance, nailEntityAlias, expectedRightWheelInstanceEntityCount);

        // Create a right wheel entity and add it under our right wheel instance.
        const AZStd::string rightWheelEntityName = "RightWheel";
        const EntityAlias rightWheelEntityAlias = CreateEntity(rightWheelEntityName, rightWheelInstance);
        ASSERT_FALSE(rightWheelEntityAlias.empty());

        // Create undo/redo node for adding the right wheel entity under the right wheel instance.
        auto createUndoAddRightWheelEntityNodeResult = CreateUnfocusedInstanceAddEntityUndoNode(
            rightWheelEntityAlias, rightWheelInstance, focusedCarInstance, "Undo Adding Right Wheel Entity");
        ASSERT_TRUE(createUndoAddRightWheelEntityNodeResult.IsSuccess());
        PrefabUndoUnfocusedInstanceAddEntity undoAddRightWheelEntityNode = createUndoAddRightWheelEntityNodeResult.TakeValue();

        // Adding the right wheel entity under the right wheel instance by redoing with our undo node and doing template propagation.
        undoAddRightWheelEntityNode.Redo();
        PropagateAllTemplateChanges();

        ValidateNewEntityUnderInstance(rightWheelInstance, rightWheelEntityAlias, rightWheelEntityName,
            ++expectedRightWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(leftWheelInstance, rightWheelEntityAlias, expectedLeftWheelInstanceEntityCount);

        // Undo adding the right wheel entity under the right wheel instance.
        undoAddRightWheelEntityNode.Undo();
        PropagateAllTemplateChanges();

        ValidateNewEntityNotUnderInstance(rightWheelInstance, rightWheelEntityAlias, --expectedRightWheelInstanceEntityCount);
        ValidateNewEntityNotUnderInstance(leftWheelInstance, rightWheelEntityAlias, expectedLeftWheelInstanceEntityCount);
    }
}
