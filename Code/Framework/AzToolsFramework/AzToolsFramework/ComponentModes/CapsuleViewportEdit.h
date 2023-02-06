/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/ComponentModes/BaseShapeViewportEdit.h>

namespace AzToolsFramework
{
    //! Wraps 2 linear manipulators, providing a viewport experience for 
    //! modifying the radius and height of a capsule.
    //! It is designed to be usable either by a component mode or by other contexts which are not associated with a
    //! particular component, so it does not contain any reference to an EntityComponentIdPair or other component-based
    //! identifier.
    class CapsuleViewportEdit : public BaseShapeViewportEdit
    {
    public:
        CapsuleViewportEdit() = default;
        virtual ~CapsuleViewportEdit() = default;

        void InstallGetRotationOffset(AZStd::function<AZ::Quaternion()> getRotationOffset);
        void InstallGetCapsuleRadius(AZStd::function<float()> getCapsuleRadius);
        void InstallGetCapsuleHeight(AZStd::function<float()> getCapsuleHeight);
        void InstallSetCapsuleRadius(AZStd::function<void(float)> setCapsuleRadius);
        void InstallSetCapsuleHeight(AZStd::function<void(float)> setCapsuleHeight);

        // BaseShapeViewportEdit overrides ...
        void Setup(const ManipulatorManagerId manipulatorManagerId = g_mainManipulatorManagerId) override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValues() override;
        void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void OnCameraStateChanged(const AzFramework::CameraState& cameraState);
    private:
        AZ::Quaternion GetRotationOffset() const;
        float GetCapsuleRadius() const;
        float GetCapsuleHeight() const;
        void SetCapsuleRadius(float radius);
        void SetCapsuleHeight(float height);

        AZ::Transform GetLocalTransform() const;

        void SetupRadiusManipulator(
            const ManipulatorManagerId manipulatorManagerId,
            const AZ::Transform& worldTransform,
            const AZ::Transform& localTransform,
            const AZ::Vector3& nonUniformScale);
        void SetupHeightManipulator(
            const ManipulatorManagerId manipulatorManagerId,
            const AZ::Transform& worldTransform,
            const AZ::Transform& localTransform,
            const AZ::Vector3& nonUniformScale);
        void OnRadiusManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action);
        void OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action);
        void AdjustRadiusManipulator(const float capsuleHeight);
        void AdjustHeightManipulator(const float capsuleRadius);

        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_radiusManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_heightManipulator;

        AZStd::function<AZ::Quaternion()> m_getRotationOffset;
        AZStd::function<float()> m_getCapsuleRadius;
        AZStd::function<float()> m_getCapsuleHeight;
        AZStd::function<void(float)> m_setCapsuleRadius;
        AZStd::function<void(float)> m_setCapsuleHeight;
    };
} // namespace AzToolsFramework
