/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <Editor/View/Widgets/LoggingPanel/AssetWindowSession/LoggingAssetDataAggregator.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////
    // LoggingAssetDataAggregator
    ///////////////////////////////
    
    LoggingAssetDataAggregator::LoggingAssetDataAggregator(const AZ::Data::AssetId& assetId)
        : m_assetId(assetId)
    {
    }
    
    LoggingAssetDataAggregator::~LoggingAssetDataAggregator()
    {
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::AnnotateNodeSignal& /**/)
    {
        // call parent process function in the aggregator class
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::ExecutionThreadEnd& /*loggableEvent*/)
    {
        // call parent process function in the aggregator class
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::ExecutionThreadBeginning& /*loggableEvent*/)
    {
        // call parent process function in the aggregator class
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::GraphActivation& /*loggableEvent*/)
    {
        // call parent process function in the aggregator class
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::GraphDeactivation& /*loggableEvent*/)
    {
        // call parent process function in the aggregator class
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::NodeStateChange& loggableEvent)
    {
        ProcessNodeStateChanged(loggableEvent);
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::InputSignal& loggableEvent)
    {
        ProcessInputSignal(loggableEvent);
    }
    
    void LoggingAssetDataAggregator::Visit(ScriptCanvas::OutputSignal& loggableEvent)
    {
        ProcessOutputSignal(loggableEvent);
    }

    void LoggingAssetDataAggregator::Visit(ScriptCanvas::VariableChange& loggableEvent)
    {
        ProcessVariableChangedSignal(loggableEvent);
    }
}
