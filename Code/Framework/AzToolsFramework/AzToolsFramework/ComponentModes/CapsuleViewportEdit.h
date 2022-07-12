/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    //! Wraps 2 linear manipulators, providing a viewport experience for 
    //! modifying the radius and height of a capsule.
    //! It is designed to be usable either by a component mode or by other contexts which are not associated with a
    //! particular component, so it does not contain any reference to an EntityComponentIdPair or other component-based
    //! identifier.
    class CapsuleViewportEdit
    {
    public:
        CapsuleViewportEdit() = default;
        virtual ~CapsuleViewportEdit() = default;

        void SetupCapsuleManipulators(const ManipulatorManagerId manipulatorManagerId);
        void TeardownCapsuleManipulators();
        void UpdateCapsuleManipulators();
        void ResetCapsuleManipulators();

        virtual AZ::Transform GetCapsuleWorldTransform() const = 0;
        virtual AZ::Transform GetCapsuleLocalTransform() const = 0;
        virtual AZ::Vector3 GetCapsuleNonUniformScale() const;
        virtual float GetCapsuleRadius() const = 0;
        virtual float GetCapsuleHeight() const = 0;
        virtual void SetCapsuleRadius(float radius) = 0;
        virtual void SetCapsuleHeight(float height) = 0;
        virtual void BeginEditing();
        virtual void FinishEditing();

    protected:
        void OnCameraStateChanged(const AzFramework::CameraState& cameraState);
        void SetupRadiusManipulator(
            const AZ::Transform& worldTransform,
            const AZ::Transform& localTransform,
            const AZ::Vector3& nonUniformScale);
        void SetupHeightManipulator(
            const AZ::Transform& worldTransform,
            const AZ::Transform& localTransform,
            const AZ::Vector3& nonUniformScale);
        void OnRadiusManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action);
        void OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action);
        void AdjustRadiusManipulator(const float capsuleHeight);
        void AdjustHeightManipulator(const float capsuleRadius);

        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_radiusManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_heightManipulator;

        ManipulatorManagerId m_manipulatorManagerId = InvalidManipulatorManagerId;
    };
} // namespace AzToolsFramework
