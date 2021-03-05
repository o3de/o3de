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

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/ImGuiEditorWindowBus.h>
#include "ImGuiManager.h"
#include "ImGuiBus.h"

namespace ImGui
{
    class ImGuiEditorWindowSystemComponent
        : public AZ::Component
        , protected ImGuiEditorWindowRequestBus::Handler
        , public ImGuiUpdateListenerBus::Handler
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(ImGuiEditorWindowSystemComponent, "{91021F3E-B5F0-4E26-A7C9-6ED0F6CB6C5A}");

        virtual ~ImGuiEditorWindowSystemComponent();

        static void Reflect(AZ::ReflectContext* context);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ImGuiEditorWindowRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ImGuiUpdateListenerBus interface implementation
        void OnOpenEditorWindow() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorEvents
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
