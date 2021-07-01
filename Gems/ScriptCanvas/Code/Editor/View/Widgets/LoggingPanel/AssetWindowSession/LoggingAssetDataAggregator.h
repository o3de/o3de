/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        void Visit(ScriptCanvas::OutputSignal&);
        void Visit(ScriptCanvas::VariableChange&);

    private:
    
        AZ::Data::AssetId m_assetId;
    };
}
