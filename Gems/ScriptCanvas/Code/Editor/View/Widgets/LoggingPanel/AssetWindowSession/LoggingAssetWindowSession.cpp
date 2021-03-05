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

#include <Editor/View/Widgets/LoggingPanel/AssetWindowSession/LoggingAssetWindowSession.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////
    // LoggingAssetWindowSession
    //////////////////////////////

    LoggingAssetWindowSession::LoggingAssetWindowSession(const AZ::Data::AssetId& assetId, QWidget* parent)
        : LoggingWindowSession(parent) 
        , m_dataAggregator(assetId)
        , m_assetId(assetId)
    {
        SetDataId(m_dataAggregator.GetDataId());

        m_ui->captureButton->setEnabled(false);

        RegisterTreeRoot(m_dataAggregator.GetTreeRoot());
    }

    LoggingAssetWindowSession::~LoggingAssetWindowSession()
    {
    }
    
    void LoggingAssetWindowSession::OnCaptureButtonPressed()
    {
    }
    
    void LoggingAssetWindowSession::OnPlaybackButtonPressed()
    {
        // TODO
    }

    void LoggingAssetWindowSession::OnOptionsButtonPressed()
    {
        // TODO
    }
    
#include <Editor/View/Widgets/LoggingPanel/AssetWindowSession/moc_LoggingAssetWindowSession.cpp>
}