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
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

namespace AzToolsFramework
{
    //! Wraps translation manipulators for editing shape translation offsets.
    class ShapeTranslationOffsetViewportEdit : public BaseShapeViewportEdit
    {
    public:
        ShapeTranslationOffsetViewportEdit() = default;

        // BaseShapeViewportEdit overrides ...
        void Setup(const ManipulatorManagerId manipulatorManagerId) override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValuesImpl() override;
        void AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

    private:
        AZStd::shared_ptr<TranslationManipulators> m_translationManipulators; //!< Manipulators for editing shape offset.
    };
} // namespace AzToolsFramework
