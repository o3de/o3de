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

#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Editor/EditorJointTypeDrawerBus.h>

namespace PhysX
{
    /// This class enables drawing in the viewport once for the component modes of multiple components in one entity.
    /// Until the component mode framework allows a way to do this, a work-around like this class is necessary.
    /// An instance of this class is created for each pair of component type and sub-component mode.
    class EditorJointTypeDrawer
        : public EditorJointTypeDrawerBus::Handler
        , public AZStd::enable_shared_from_this<EditorJointTypeDrawer>
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        EditorJointTypeDrawer(EditorJointType id,
            AzFramework::EntityContextId entityContextId,
            const AZStd::string& subComponentModeName);
        ~EditorJointTypeDrawer();

    private:
        // AzFramework::ViewportDebugDisplayEventBus
        void DisplayViewport2d(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // PhysX::EditorJointTypeDrawerBus
        AZStd::shared_ptr<EditorJointTypeDrawer> GetEditorJointTypeDrawer() override;

        AZStd::string m_subComponentModeName;///< Name of the sub component mode. E.g. Position, Rotation, Snap Position, etc.
    };
} // namespace PhysX
