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

#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>
#include <ScriptCanvas/Debugger/Logger.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

namespace ScriptCanvasEditor
{
    class LoggingAssetDataAggregator
        : public LoggingDataAggregator
        , public ScriptCanvas::LoggableEventVisitor
    {
    public:
        AZ_CLASS_ALLOCATOR(LoggingAssetDataAggregator, AZ::SystemAllocator, 0);
        
        LoggingAssetDataAggregator(const AZ::Data::AssetId& assetId);
        ~LoggingAssetDataAggregator() override;

        bool CanCaptureData() const override { return false;  }
        bool IsCapturingData() const override { return false; }
        
    protected:
        void Visit(ScriptCanvas::AnnotateNodeSignal&);
        void Visit(ScriptCanvas::ExecutionThreadEnd&);
        void Visit(ScriptCanvas::ExecutionThreadBeginning&);
        void Visit(ScriptCanvas::GraphActivation&);
        void Visit(ScriptCanvas::GraphDeactivation&);
        void Visit(ScriptCanvas::NodeStateChange&);
        void Visit(ScriptCanvas::InputSignal&);
        void Visit(ScriptCanvas::OutputDataSignal&);
        void Visit(ScriptCanvas::OutputSignal&);
        void Visit(ScriptCanvas::VariableChange&);

    private:
    
        AZ::Data::AssetId m_assetId;
    };
}