/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>
#include <AzToolsFramework/ComponentModes/CylinderViewportEdit.h>

namespace AzToolsFramework
{
    void InstallCylinderViewportEditFunctions(
        CylinderViewportEdit* sphereViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! The specific ComponentMode responsible for handling sphere editing.
    class CylinderComponentMode
        : public BaseShapeComponentMode
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(CylinderComponentMode, "{BC1F676F-2BAF-4C87-9981-DC27CA34469D}", BaseShapeComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        CylinderComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        CylinderComponentMode(const CylinderComponentMode&) = delete;
        CylinderComponentMode& operator=(const CylinderComponentMode&) = delete;
        CylinderComponentMode(CylinderComponentMode&&) = delete;
        CylinderComponentMode& operator=(CylinderComponentMode&&) = delete;
        ~CylinderComponentMode();

        // EditorBaseComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    private:
        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
    };
} // namespace AzToolsFramework

