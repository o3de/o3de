
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
