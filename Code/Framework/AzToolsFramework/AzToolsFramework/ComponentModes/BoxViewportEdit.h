/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ComponentModes/BaseShapeViewportEdit.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    //! Wraps 6 linear manipulators, providing a viewport experience for modifying the extents of a box.
    //! It is designed to be usable either by a component mode or by other contexts which are not associated with a
    //! particular component, so editing does not rely on an EntityComponentIdPair or other component-based identifier.
    class BoxViewportEdit : public BaseShapeViewportEdit
    {
    public:
        BoxViewportEdit(bool allowAsymmetricalEditing = false);

        // BaseShapeViewportEdit overrides ...
        void Setup(const ManipulatorManagerId manipulatorManagerId) override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValuesImpl() override;
        void AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void InstallGetBoxDimensions(AZStd::function<AZ::Vector3()> getBoxDimensions);
        void InstallSetBoxDimensions(AZStd::function<void(const AZ::Vector3&)> setBoxDimensions);

    private:
        AZ::Vector3 GetBoxDimensions() const;
        void SetBoxDimensions(const AZ::Vector3& boxDimensions);

        using BoxManipulators = AZStd::array<AZStd::shared_ptr<LinearManipulator>, 6>;
        BoxManipulators m_linearManipulators; ///< Manipulators for editing box size.
        bool m_allowAsymmetricalEditing = false; ///< Whether moving individual faces independently is allowed.

        AZStd::function<AZ::Vector3()> m_getBoxDimensions;
        AZStd::function<void(const AZ::Vector3&)> m_setBoxDimensions;
    };
} // namespace AzToolsFramework
