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
