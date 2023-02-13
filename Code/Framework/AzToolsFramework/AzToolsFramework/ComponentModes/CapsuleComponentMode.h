/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>
#include <AzToolsFramework/ComponentModes/CapsuleViewportEdit.h>

namespace AzToolsFramework
{
    void InstallCapsuleViewportEditFunctions(
        CapsuleViewportEdit* capsuleViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! The specific ComponentMode responsible for handling capsule editing.
    class CapsuleComponentMode
        : public BaseShapeComponentMode
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(CapsuleComponentMode, "{17036B78-EB62-4F2B-8F74-C9FB037D8973}", BaseShapeComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        CapsuleComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        CapsuleComponentMode(const CapsuleComponentMode&) = delete;
        CapsuleComponentMode& operator=(const CapsuleComponentMode&) = delete;
        CapsuleComponentMode(CapsuleComponentMode&&) = delete;
        CapsuleComponentMode& operator=(CapsuleComponentMode&&) = delete;
        ~CapsuleComponentMode();

        // EditorBaseComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    private:
        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
    };
} // namespace AzToolsFramework
