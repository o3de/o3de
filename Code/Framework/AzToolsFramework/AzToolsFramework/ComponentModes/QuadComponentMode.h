/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>
#include <AzToolsFramework/ComponentModes/QuadViewportEdit.h>

namespace AzToolsFramework
{
    void InstallQuadViewportEditFunctions(
        QuadViewportEdit* quadViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! The specific ComponentMode responsible for handling capsule editing.
    class QuadComponentMode
        : public BaseShapeComponentMode
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(QuadComponentMode, "{B6C509E6-7661-4F04-AECE-F547B0167056}", BaseShapeComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        QuadComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        QuadComponentMode(const QuadComponentMode&) = delete;
        QuadComponentMode& operator=(const QuadComponentMode&) = delete;
        QuadComponentMode(QuadComponentMode&&) = delete;
        QuadComponentMode& operator=(QuadComponentMode&&) = delete;
        ~QuadComponentMode();

        // EditorBaseComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    private:
        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
    };
} // namespace AzToolsFramework

