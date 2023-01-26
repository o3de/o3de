/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ComponentModes/BaseViewportEdit.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    /// Wraps 6 linear manipulators, providing a viewport experience for 
    /// modifying the extents of a box
    class BoxViewportEdit : public BaseViewportEdit
    {
    public:
        BoxViewportEdit(bool allowAsymmetricalEditing = false);

        // BaseViewportEdit overrides ...
        void Setup() override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValues() override;
        void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void InstallGetManipulatorSpaceFunction(const AZStd::function<AZ::Transform()>& getManipulatorSpaceFunction);
        void InstallGetNonUniformScaleFunction(const AZStd::function<AZ::Vector3()>& getNonUniformScaleFunction);
        void InstallGetBoxDimensionsFunction(const AZStd::function<AZ::Vector3()>& getBoxDimensionsFunction);
        void InstallGetTranslationOffsetFunction(const AZStd::function<AZ::Vector3()>& getTranslationOffsetFunction);
        void InstallGetLocalTransformFunction(const AZStd::function<AZ::Transform()>& getLocalTransformFunction);
        void InstallSetBoxDimensionsFunction(const AZStd::function<void(const AZ::Vector3)>& setBoxDimensionsFunction);
        void InstallSetTranslationOffsetFunction(const AZStd::function<void(const AZ::Vector3)>& setTranslationOffsetFunction);

    private:
        AZ::Transform GetManipulatorSpace() const;
        AZ::Vector3 GetNonUniformScale() const;
        AZ::Vector3 GetBoxDimensions() const;
        AZ::Vector3 GetTranslationOffset() const;
        AZ::Transform GetLocalTransform() const;
        void SetBoxDimensions(const AZ::Vector3& boxDimensions);
        void SetTranslationOffset(const AZ::Vector3& translationOffset);

        using BoxManipulators = AZStd::array<AZStd::shared_ptr<LinearManipulator>, 6>;
        BoxManipulators m_linearManipulators; ///< Manipulators for editing box size.
        bool m_allowAsymmetricalEditing = false; ///< Whether moving individual faces independently is allowed.

        AZStd::function<AZ::Transform()> m_getManipulatorSpaceFunction;
        AZStd::function<AZ::Vector3()> m_getNonUniformScaleFunction;
        AZStd::function<AZ::Vector3()> m_getBoxDimensionsFunction;
        AZStd::function<AZ::Vector3()> m_getTranslationOffsetFunction;
        AZStd::function<AZ::Transform()> m_getLocalTransformFunction;
        AZStd::function<void(const AZ::Vector3)> m_setBoxDimensionsFunction;
        AZStd::function<void(const AZ::Vector3)> m_setTranslationOffsetFunction;
    };
} // namespace AzToolsFramework
