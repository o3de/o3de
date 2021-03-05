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

#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace ScriptCanvasEditor
{
    class MainWindow;

    class BatchOperatorTool
    {
    protected:
        enum class OperationStatus
        {
            Incomplete,
            Complete
        };
        
    public:
        AZ_CLASS_ALLOCATOR(BatchOperatorTool, AZ::SystemAllocator, 0);

        BatchOperatorTool(MainWindow* mainWindow, QStringList directories, QString progressDialogTitle = QString("Batch Processing Files..."));
        virtual ~BatchOperatorTool();
        
    protected:
    
        // Operates on the specified files. Returns whether or not the file has completed being operated upon.
        virtual OperationStatus OperateOnFile(const QString& fileName) = 0;
        void SignalOperationComplete();
        
        MainWindow* GetMainWindow() const;
        QProgressDialog* GetProgressDialog() const;

        virtual void OnOperationCancelled();

    private:

        void CancelOperation();

        void IterateOverDirectory(QDir directory);
        void TickIterator();

        void OnBatchComplete();

        AZStd::vector< QDirIterator* > m_directoryIterators;

        MainWindow* m_mainWindow;

        bool m_cancelled = false;

        // This is for the CVar editor setting
        int m_originalActive;        

        // This is for tracking which tab is currently selected
        int m_originalActiveTab;
        
        QProgressDialog* m_progressDialog;

        QMetaObject::Connection m_connection;
    };
}
