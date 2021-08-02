/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
