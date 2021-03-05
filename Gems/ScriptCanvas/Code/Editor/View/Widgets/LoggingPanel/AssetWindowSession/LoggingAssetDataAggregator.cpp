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
    
    void LoggingAssetDataAggregator::Visit(ScriptCanvas::OutputDataSignal& loggableEvent)
    {
        ProcessOutputDataSignal(loggableEvent);
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