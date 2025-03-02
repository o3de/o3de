/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <View/Windows/MainWindow.h>

namespace ScriptCanvas
{
    class ScriptCanvasApplication
        : public AtomToolsFramework::AtomToolsDocumentApplication
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , private AzToolsFramework::EditorWindowRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasApplication, AZ::SystemAllocator)
        AZ_TYPE_INFO(ScriptCanvasApplication, "{484D42F9-30C5-4221-BF23-EDCA71726C05}");
        using Base = AtomToolsFramework::AtomToolsDocumentApplication;

        ScriptCanvasApplication(int* argc, char*** argv);
        virtual ~ScriptCanvasApplication() override;

        // AzFramework::Application overrides
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;
        // ~AzToolsFramework::ToolsApplication

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

    private:
        void InitMainWindow();

    private:
        AZStd::unique_ptr<ScriptCanvasEditor::MainWindow> m_window;
    };
} // namespace ScriptCanvas
