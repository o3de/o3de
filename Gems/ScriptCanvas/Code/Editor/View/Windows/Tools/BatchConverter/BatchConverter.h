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

#include <QDir>
#include <QDirIterator>
#include <QProgressDialog>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <Editor/View/Windows/Tools/BatchOperatorTool.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasBatchConverter
        : public BatchOperatorTool
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasBatchConverter, AZ::SystemAllocator, 0);

        ScriptCanvasBatchConverter(MainWindow* mainWindow, QStringList directories);
        ~ScriptCanvasBatchConverter();

        // GraphCanvas::AssetEditorNotifications
        void PostOnActiveGraphChanged() override;
        ////
        
    protected:
    
        OperationStatus OperateOnFile(const QString& fileName) override;

    private:

        // AZ::SystemTickBus
        void OnSystemTick();
        ////

        bool m_processing;

        AZ::Data::AssetId m_assetId;
    };
}
