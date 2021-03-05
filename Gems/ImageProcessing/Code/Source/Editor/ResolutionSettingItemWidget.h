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
#include <QWidget>
#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/EditorCommon.h>
#endif

namespace ImageProcessing
{
    class PresetSettings;
}
namespace Ui
{
    class ResolutionSettingItemWidget;
}

namespace ImageProcessingEditor
{    
    enum class ResoultionWidgetType
    {
        TexturePipeline,       //Fully editable
        TexturePropety,        //Only DownRes is editable
    };

    class ResolutionSettingItemWidget
        : public QWidget
        , EditorInternalNotificationBus::Handler
    {
        Q_OBJECT
    public:

        AZ_CLASS_ALLOCATOR(ResolutionSettingItemWidget, AZ::SystemAllocator, 0);
        explicit ResolutionSettingItemWidget(ResoultionWidgetType type, QWidget* parent = nullptr);
        ~ResolutionSettingItemWidget();
        void Init(AZStd::string platform, EditorTextureSetting* editorTextureSetting);

    public slots:

        void OnChangeDownRes(int downRes);
        void OnChangeFormat(int index);

    protected:
        ////////////////////////////////////////////////////////////////////////
        //EditorInternalNotificationBus
        void OnEditorSettingsChanged(bool needRefresh, const AZStd::string& platform);
        ////////////////////////////////////////////////////////////////////////

    private:

        void SetupFormatComboBox();
        void SetupResolutionInfo();
        void RefreshUI();
        QString GetFinalFormat(const AZ::Uuid& presetId);

        QScopedPointer<Ui::ResolutionSettingItemWidget> m_ui;
        ResoultionWidgetType m_type;
        AZStd::string m_platform;
        ImageProcessing::TextureSettings* m_textureSetting;
        EditorTextureSetting* m_editorTextureSetting;
        const ImageProcessing::PresetSettings* m_preset;
        //Cached list of calculated final resolution info based on different reduce levels
        AZStd::list<ResolutionInfo> m_resolutionInfos;
        //Final reduce level range
        unsigned int m_maxReduce;
        unsigned int m_minReduce;
    };
} //namespace ImageProcessingEditor

