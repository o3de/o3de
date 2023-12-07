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
    class CylinderViewportEdit : public BaseShapeViewportEdit
    {
    public:
        CylinderViewportEdit(bool allowAsymmetricalEditing = false);

        void InstallGetCylinderRadius(AZStd::function<float()> getCapsuleRadius);
        void InstallGetCylinderHeight(AZStd::function<float()> getCapsuleHeight);
        void InstallSetCylinderRadius(AZStd::function<void(float)> setCapsuleRadius);
        void InstallSetCylinderHeight(AZStd::function<void(float)> setCapsuleHeight);

        // BaseShapeViewportEdit overrides ...
        void Setup(const ManipulatorManagerId manipulatorManagerId) override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValuesImpl() override;
        void AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void OnCameraStateChanged(const AzFramework::CameraState& cameraState);
    private:
        float GetCylinderRadius() const;
        float GetCylinderHeight() const;
        void SetCylinderRadius(float radius);
        void SetCylinderHeight(float height);

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

        AZStd::shared_ptr<LinearManipulator> m_radiusManipulator;
        AZStd::shared_ptr<LinearManipulator> m_topManipulator;
        AZStd::shared_ptr<LinearManipulator> m_bottomManipulator;
        bool m_allowAsymmetricalEditing = false; //!< Whether moving the ends of the cylinder independently is allowed.

        AZStd::function<float()> m_getCylinderRadius;
        AZStd::function<float()> m_getCylinderHeight;
        AZStd::function<void(float)> m_setCylinderRadius;
        AZStd::function<void(float)> m_setCylinderHeight;
    };
} // namespace AzToolsFramework

