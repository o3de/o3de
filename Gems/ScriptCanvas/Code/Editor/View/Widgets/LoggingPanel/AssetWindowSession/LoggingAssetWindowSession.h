/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/View/Widgets/LoggingPanel/LoggingWindowSession.h>
#include <Editor/View/Widgets/LoggingPanel/AssetWindowSession/LoggingAssetDataAggregator.h>
#endif

namespace ScriptCanvasEditor
{
    class LoggingAssetWindowSession
        : public LoggingWindowSession
    {
        Q_OBJECT
        
    public:
        AZ_CLASS_ALLOCATOR(LoggingAssetWindowSession, AZ::SystemAllocator, 0);
        
        LoggingAssetWindowSession(const AZ::Data::AssetId& assetId, QWidget* parent = nullptr);
        ~LoggingAssetWindowSession() override;
        
    protected:
    
        void OnCaptureButtonPressed() override;
        void OnPlaybackButtonPressed() override;
        void OnOptionsButtonPressed() override;
        
    private:
    
        AZ::Data::AssetId m_assetId;
    
        LoggingAssetDataAggregator m_dataAggregator;
    };
}
