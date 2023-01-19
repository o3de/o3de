/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    /// The specific ComponentMode responsible for handling box editing.
    class BoxComponentMode
        : public ComponentModeFramework::EditorBaseComponentMode
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(BoxComponentMode, "{8E09B2C1-ED99-4945-A0B1-C4AFE6FE2FA9}", EditorBaseComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        BoxComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        BoxComponentMode(const BoxComponentMode&) = delete;
        BoxComponentMode& operator=(const BoxComponentMode&) = delete;
        BoxComponentMode(BoxComponentMode&&) = delete;
        BoxComponentMode& operator=(BoxComponentMode&&) = delete;
        ~BoxComponentMode();

        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    private:
        BoxViewportEdit m_boxEdit;
    };
} // namespace AzToolsFramework
