/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BaseShapeViewportEdit.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    //! Wraps linear manipulators, providing a viewport experience for
    //! modifying the radius and height of a capsule.
    //! It is designed to be usable either by a component mode or by other contexts which are not associated with a
    //! particular component, so editing does not rely on an EntityComponentIdPair or other component-based identifier.
    class QuadViewportEdit : public BaseShapeViewportEdit
    {
    public:
        QuadViewportEdit();

        void InstallSetQuadWidth(AZStd::function<void(float)> setQuadWidth);
        void InstallSetQuadHeight(AZStd::function<void(float)> setQuadHeight);
        void InstallGetQuadWidth(AZStd::function<float()> getQuadWidth);
        void InstallGetQuadHeight(AZStd::function<float()> getQuadHeight);

        // BaseShapeViewportEdit overrides ...
        void Setup(const ManipulatorManagerId manipulatorManagerId) override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValuesImpl() override;
        void AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void OnCameraStateChanged(const AzFramework::CameraState& cameraState);

    private:
        float GetQuadHeight() const;
        float GetQuadWidth() const;
        void SetQuadWidth(float width);
        void SetQuadHeight(float height);

        AZStd::shared_ptr<LinearManipulator> m_widthManipulator;
        AZStd::shared_ptr<LinearManipulator> m_heightManipulator;

        AZStd::function<float()> m_getQuadHeight;
        AZStd::function<float()> m_getQuadWidth;
        AZStd::function<void(float)> m_setQuadWidth;
        AZStd::function<void(float)> m_setQuadHeight;
    };
} // namespace AzToolsFramework
