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
#include <PhysX_precompiled.h>

#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Editor/EditorJointTypeDrawer.h>

namespace PhysX
{
    EditorJointTypeDrawer::EditorJointTypeDrawer(EditorJointType jointType,
        AzFramework::EntityContextId entityContextId,
        const AZStd::string& subComponentModeName):
        m_subComponentModeName(subComponentModeName)
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(entityContextId);
        EditorJointTypeDrawerBus::Handler::BusConnect(EditorJointTypeDrawerId(jointType,
            EditorSubComponentModeNameCrc(subComponentModeName)));
    }

    EditorJointTypeDrawer::~EditorJointTypeDrawer()
    {
        EditorJointTypeDrawerBus::Handler::BusDisconnect();
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorJointTypeDrawer::DisplayViewport2d(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AZ::u32 stateBefore = debugDisplay.GetState();

        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);

        const float xOffsetCurrentMode = 125.0f;
        const float yOffsetCurrentMode = 55.0f;

        const float xOffsetHotKeys = 125.0f;
        const float yOffsetHotKeys = 30.0f;

        const float viewportWidthHalf = cameraState.m_viewportSize.GetX() / 2.0f;
        const float viewportHeight = cameraState.m_viewportSize.GetY();

        debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));

        AZStd::string screenTextCurrentMode = "Edit mode: " + m_subComponentModeName;
        float xPos = viewportWidthHalf - xOffsetCurrentMode;
        float yPos = viewportHeight - yOffsetCurrentMode;
        float textSize = 2.0f;
        debugDisplay.Draw2dTextLabel(xPos, yPos, textSize, screenTextCurrentMode.c_str());

        AZStd::string screenTextHotKeys = "<Tab> or <Shift+Tab> to change modes";
        xPos = viewportWidthHalf - xOffsetHotKeys;
        yPos = viewportHeight - yOffsetHotKeys;
        textSize = 1.2f;
        debugDisplay.Draw2dTextLabel(xPos, yPos, textSize, screenTextHotKeys.c_str());

        debugDisplay.SetState(stateBefore);
    }

    AZStd::shared_ptr<EditorJointTypeDrawer> EditorJointTypeDrawer::GetEditorJointTypeDrawer()
    {
        return shared_from_this();
    }

} // namespace PhysX
