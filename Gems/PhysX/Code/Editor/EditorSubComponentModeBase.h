
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }
}

namespace PhysX
{
    class EditorJointTypeDrawer;

    /// Base class for (joints) sub-component modes.
    class EditorSubComponentModeBase
    {
    public:
        EditorSubComponentModeBase(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZ::Uuid& componentType,
            const AZStd::string& name);
        virtual ~EditorSubComponentModeBase() = default;

        /// Additional mouse handling by sub-component mode. Does not absorb mouse event.
        virtual void HandleMouseInteraction(
            [[maybe_unused]] const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) {};

        virtual void Refresh() = 0;

        AZStd::string m_name;///< Name of sub-component mode.

    protected:
        AZ::EntityComponentIdPair m_entityComponentId;///< Entity Id and component pair.

    private:
        AZStd::shared_ptr<EditorJointTypeDrawer> m_jointTypeDrawer;///< Drawer that draws component type specific objects in the viewport.
    };
} // namespace PhysX
