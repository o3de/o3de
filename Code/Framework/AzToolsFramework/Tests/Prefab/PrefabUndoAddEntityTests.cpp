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

        InstanceList carInstances{focusedCarInstance,  secondCarInstance};

        // Create a car entity and add it under our car instance.
        const AZStd::string carEntityName = "Car";
        const EntityAlias carEntityAlias = CreateEntity(carEntityName, focusedCarInstance);
        ASSERT_FALSE(carEntityAlias.empty());

        // Create undo/redo node for adding the car entity under the car instance.
        PrefabUndoAddEntity undoAddCarEntityNode = CreatePrefabUndoAddEntityNode(
            carEntityAlias, focusedCarInstance, "Undo Adding Car Entity");

        size_t expectedEntityCount = 0;

        // Adding the car entity under the car instance by redoing with our undo node and doing template propagation.
        undoAddCarEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityUnderInstance(instance, carEntityAlias, carEntityName, expectedEntityCount);
        }

        // Create an axle entity and add it under the car entity.
        const AZStd::string axleEntityName = "Axle";
        const EntityAlias axleEntityAlias = CreateEntity(axleEntityName, focusedCarInstance, carEntityAlias);
        ASSERT_FALSE(axleEntityAlias.empty());

        // Create undo/redo node for adding the axle entity under the car entity.
        PrefabUndoAddEntity undoAddAxleEntityNode = CreatePrefabUndoAddEntityNode(
            axleEntityAlias, focusedCarInstance, "Undo Adding Axle Entity", carEntityAlias);

        // Adding the axle entity under the car entity by redoing with our undo node and doing template propagation.
        undoAddAxleEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityUnderParentEntity(instance, carEntityAlias, carEntityName,
                axleEntityAlias, axleEntityName, expectedEntityCount);
        }

        // Create a wheel entity and add it under the axle entity.
        const AZStd::string wheelEntityName = "Wheel";
        const EntityAlias wheelEntityAlias = CreateEntity(wheelEntityName, focusedCarInstance, axleEntityAlias);
        ASSERT_FALSE(wheelEntityAlias.empty());

        // Create undo/redo node for adding the wheel entity under the axle entity.
        PrefabUndoAddEntity undoAddWheelEntityNode = CreatePrefabUndoAddEntityNode(
            wheelEntityAlias, focusedCarInstance, "Undo Adding Wheel Entity", axleEntityAlias);

        // Adding the wheel entity under the axle entity by redoing with our undo node and doing template propagation.
        undoAddWheelEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityUnderParentEntity(instance, axleEntityAlias, axleEntityName,
                wheelEntityAlias, wheelEntityName, expectedEntityCount);
        }

        // Undo adding the wheel entity under the axle entity.
        undoAddWheelEntityNode.Undo();
        PropagateAllTemplateChanges();
        --expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityNotUnderParentEntity(instance, axleEntityAlias, axleEntityName,
                wheelEntityAlias, expectedEntityCount);
        }

        // Undo adding the axle entity under the car entity.
        undoAddAxleEntityNode.Undo();
        PropagateAllTemplateChanges();
        --expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityNotUnderParentEntity(instance, carEntityAlias, carEntityName,
                axleEntityAlias, expectedEntityCount);
        }

        // Undo adding the car entity under the car instance.
        undoAddCarEntityNode.Undo();
        PropagateAllTemplateChanges();
        --expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityNotUnderInstance(instance, carEntityAlias, expectedEntityCount);
        }

        // Redo all adding entity operations.
        undoAddCarEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        undoAddAxleEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        undoAddWheelEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedEntityCount;

        for (auto& instance : carInstances)
        {
            ValidateNewEntityUnderParentEntity(instance, axleEntityAlias, axleEntityName,
                wheelEntityAlias, wheelEntityName, expectedEntityCount);
        }
    }

    TEST_F(PrefabUndoAddEntityTests, PrefabUndoAddEntity_AddEntityUnderUnfocusedInstance)
    {
        // Create a wheel instance.
        AZStd::unique_ptr<Instance> wheelInstancePtr =
            m_prefabSystemComponent->CreatePrefab({}, {}, "test/path/wheel");
        ASSERT_TRUE(wheelInstancePtr);
        Instance& leftWheelInstanceBeforePropagation = *wheelInstancePtr;
        const InstanceAlias leftWheelInstanceAlias = wheelInstancePtr->GetInstanceAlias();

        // Create another wheel instance to be added under an axle instance later.
        AZStd::unique_ptr<Instance> secondWheelInstancePtr =
            m_prefabSystemComponent->InstantiatePrefab(wheelInstancePtr->GetTemplateId());
        ASSERT_TRUE(secondWheelInstancePtr);
        const InstanceAlias rightWheelInstanceAlias = secondWheelInstancePtr->GetInstanceAlias();

        // Create an axle instance which includes two wheel instances under it.
        AZStd::unique_ptr<Instance> axleInstancePtr =
            m_prefabSystemComponent->CreatePrefab({},
                MakeInstanceList(AZStd::move(wheelInstancePtr), AZStd::move(secondWheelInstancePtr)),
                "test/path/axle");
        ASSERT_TRUE(axleInstancePtr);
        const InstanceAlias axleInstanceAlias = axleInstancePtr->GetInstanceAlias();

        // Create a car instance which includes one axle instance under it.
        AZStd::unique_ptr<Instance> carInstancePtr =
            m_prefabSystemComponent->CreatePrefab({},
                MakeInstanceList(AZStd::move(axleInstancePtr)),
                "test/path/car");
        ASSERT_TRUE(carInstancePtr);
        Instance& focusedCarInstance = *carInstancePtr;

        // Create a left wheel entity and add it under our left wheel instance.
        const AZStd::string leftWheelEntityName = "LeftWheel";
        const EntityAlias leftWheelEntityAlias = CreateEntity(leftWheelEntityName, leftWheelInstanceBeforePropagation);
        ASSERT_FALSE(leftWheelEntityAlias.empty());

        // Create undo/redo node for adding the left wheel entity under the left wheel instance.
        PrefabUndoAddEntityAsOverride undoAddLeftWheelEntityNode = CreatePrefabUndoAddEntityAsOverrideNode(
            leftWheelEntityAlias, leftWheelInstanceBeforePropagation, focusedCarInstance, "Undo Adding Left Wheel Entity");

        size_t expectedLeftWheelInstanceEntityCount = 0;
        size_t expectedRightWheelInstanceEntityCount = 0;

        // Adding the left wheel entity under the left wheel instance by redoing with our undo node and doing template propagation.
        undoAddLeftWheelEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        auto findAxleInstanceResult = focusedCarInstance.FindNestedInstance(axleInstanceAlias);
        ASSERT_TRUE(findAxleInstanceResult.has_value());
        Instance& axleInstance = findAxleInstanceResult->get();

        auto findLeftWheelInstanceResult = axleInstance.FindNestedInstance(leftWheelInstanceAlias);
        ASSERT_TRUE(findLeftWheelInstanceResult.has_value());
        Instance& leftWheelInstance = findLeftWheelInstanceResult->get();

        auto findRightWheelInstanceResult = axleInstance.FindNestedInstance(rightWheelInstanceAlias);
        ASSERT_TRUE(findRightWheelInstanceResult.has_value());
        Instance& rightWheelInstance = findRightWheelInstanceResult->get();

        // We should see the left wheel entity be added under left wheel instance.
        ValidateNewEntityUnderInstance(
            leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName, expectedLeftWheelInstanceEntityCount);

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
        PrefabUndoAddEntityAsOverride undoAddTireEntityNode = CreatePrefabUndoAddEntityAsOverrideNode(
            tireEntityAlias, leftWheelInstance, focusedCarInstance, "Undo Adding Tire Entity", leftWheelEntityAlias);

        // Adding the tire entity under the left wheel entity by redoing with our undo node and doing template propagation.
        undoAddTireEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        ValidateNewEntityUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            tireEntityAlias, tireEntityName, expectedLeftWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(rightWheelInstance, tireEntityAlias, expectedRightWheelInstanceEntityCount);

        // Undo adding the tire entity under the left wheel entity.
        undoAddTireEntityNode.Undo();
        PropagateAllTemplateChanges();
        --expectedLeftWheelInstanceEntityCount;

        ValidateNewEntityNotUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            tireEntityAlias, expectedLeftWheelInstanceEntityCount);

        // Undo adding the left wheel entity under the left wheel instance.
        undoAddLeftWheelEntityNode.Undo();
        PropagateAllTemplateChanges();
        --expectedLeftWheelInstanceEntityCount;

        InstanceList wheelInstances = { leftWheelInstance, rightWheelInstance };
        for (auto& instance : wheelInstances)
        {
            ValidateNewEntityNotUnderInstance(instance, leftWheelEntityAlias, expectedLeftWheelInstanceEntityCount);
        }

        // Redo all adding entity operations.
        undoAddLeftWheelEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        undoAddTireEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedLeftWheelInstanceEntityCount;

        ValidateNewEntityUnderParentEntity(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            tireEntityAlias, tireEntityName, expectedLeftWheelInstanceEntityCount);
        ValidateNewEntityUnderInstance(leftWheelInstance, leftWheelEntityAlias, leftWheelEntityName,
            expectedLeftWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(rightWheelInstance, leftWheelEntityAlias, expectedRightWheelInstanceEntityCount);
        ValidateNewEntityNotUnderInstance(rightWheelInstance, tireEntityAlias, expectedRightWheelInstanceEntityCount);

        // Create a right wheel entity and add it under our right wheel instance.
        const AZStd::string rightWheelEntityName = "RightWheel";
        const EntityAlias rightWheelEntityAlias = CreateEntity(rightWheelEntityName, rightWheelInstance);
        ASSERT_FALSE(rightWheelEntityAlias.empty());

        // Create undo/redo node for adding the right wheel entity under the right wheel instance.
        PrefabUndoAddEntityAsOverride undoAddRightWheelEntityNode = CreatePrefabUndoAddEntityAsOverrideNode(
            rightWheelEntityAlias, rightWheelInstance, focusedCarInstance, "Undo Adding Right Wheel Entity");

        // Adding the right wheel entity under the right wheel instance by redoing with our undo node and doing template propagation.
        undoAddRightWheelEntityNode.Redo();
        PropagateAllTemplateChanges();
        ++expectedRightWheelInstanceEntityCount;

        ValidateNewEntityUnderInstance(rightWheelInstance, rightWheelEntityAlias, rightWheelEntityName,
            expectedRightWheelInstanceEntityCount);

        ValidateNewEntityNotUnderInstance(leftWheelInstance, rightWheelEntityAlias, expectedLeftWheelInstanceEntityCount);

        // Undo adding the right wheel entity under the right wheel instance.
        undoAddRightWheelEntityNode.Undo();
        PropagateAllTemplateChanges();
        --expectedRightWheelInstanceEntityCount;

        ValidateNewEntityNotUnderInstance(rightWheelInstance, rightWheelEntityAlias, expectedRightWheelInstanceEntityCount);
        ValidateNewEntityNotUnderInstance(leftWheelInstance, rightWheelEntityAlias, expectedLeftWheelInstanceEntityCount);
    }
}
