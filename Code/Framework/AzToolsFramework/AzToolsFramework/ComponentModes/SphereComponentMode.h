/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>
#include <AzToolsFramework/ComponentModes/SphereViewportEdit.h>

namespace AzToolsFramework
{
    void InstallSphereViewportEditFunctions(
        SphereViewportEdit* sphereViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! The specific ComponentMode responsible for handling sphere editing.
    class SphereComponentMode
        : public BaseShapeComponentMode
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(SphereComponentMode, "{540CD123-86E7-4C4F-894D-56F06FC3F9D1}", BaseShapeComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        SphereComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        SphereComponentMode(const SphereComponentMode&) = delete;
        SphereComponentMode& operator=(const SphereComponentMode&) = delete;
        SphereComponentMode(SphereComponentMode&&) = delete;
        SphereComponentMode& operator=(SphereComponentMode&&) = delete;
        ~SphereComponentMode();

        // EditorBaseComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    private:
        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
    };
} // namespace AzToolsFramework
