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
    //! Wraps linear manipulators, providing a viewport experience for 
    //! modifying the radius and height of a capsule.
    //! It is designed to be usable either by a component mode or by other contexts which are not associated with a
    //! particular component, so editing does not rely on an EntityComponentIdPair or other component-based identifier.
    class CapsuleViewportEdit : public BaseShapeViewportEdit
    {
    public:
        CapsuleViewportEdit(bool allowAsymmetricalEditing = false);

        //! Set whether to force height to exceed twice the radius when editing (default is true).
        void SetEnsureHeightExceedsTwiceRadius(bool ensureHeightExceedsTwiceRadius);

        void InstallGetCapsuleRadius(AZStd::function<float()> getCapsuleRadius);
        void InstallGetCapsuleHeight(AZStd::function<float()> getCapsuleHeight);
        void InstallSetCapsuleRadius(AZStd::function<void(float)> setCapsuleRadius);
        void InstallSetCapsuleHeight(AZStd::function<void(float)> setCapsuleHeight);

        // BaseShapeViewportEdit overrides ...
        void Setup(const ManipulatorManagerId manipulatorManagerId) override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValuesImpl() override;
        void AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void OnCameraStateChanged(const AzFramework::CameraState& cameraState);
    private:
        float GetCapsuleRadius() const;
        float GetCapsuleHeight() const;
        void SetCapsuleRadius(float radius);
        void SetCapsuleHeight(float height);

        void SetupRadiusManipulator(
            const ManipulatorManagerId manipulatorManagerId,
            const AZ::Transform& worldTransform,
            const AZ::Transform& localTransform,
            const AZ::Vector3& nonUniformScale);
        AZStd::shared_ptr<LinearManipulator> SetupHeightManipulator(
            const ManipulatorManagerId manipulatorManagerId,
            const AZ::Transform& worldTransform,
            const AZ::Transform& localTransform,
            const AZ::Vector3& nonUniformScale,
            float axisDirection);
        void OnRadiusManipulatorMoved(const LinearManipulator::Action& action);
        void OnHeightManipulatorMoved(const LinearManipulator::Action& action);
        void AdjustRadiusManipulator(const float capsuleHeight);
        void AdjustHeightManipulators(const float capsuleRadius);

        AZStd::shared_ptr<LinearManipulator> m_radiusManipulator;
        AZStd::shared_ptr<LinearManipulator> m_topManipulator;
        AZStd::shared_ptr<LinearManipulator> m_bottomManipulator;
        bool m_allowAsymmetricalEditing = false; //!< Whether moving the ends of the capsule independently is allowed.
        bool m_ensureHeightExceedsTwiceRadius = true; //!< Whether to force height to exceed twice the radius when editing.

        AZStd::function<float()> m_getCapsuleRadius;
        AZStd::function<float()> m_getCapsuleHeight;
        AZStd::function<void(float)> m_setCapsuleRadius;
        AZStd::function<void(float)> m_setCapsuleHeight;
    };
} // namespace AzToolsFramework
