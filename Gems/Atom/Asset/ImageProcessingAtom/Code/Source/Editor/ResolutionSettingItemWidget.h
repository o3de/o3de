/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/EditorCommon.h>
#endif

namespace ImageProcessingAtom
{
    class PresetSettings;
}
namespace Ui
{
    class ResolutionSettingItemWidget;
}

namespace ImageProcessingAtomEditor
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

        AZ_CLASS_ALLOCATOR(ResolutionSettingItemWidget, AZ::SystemAllocator);
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
        QString GetFinalFormat(const ImageProcessingAtom::PresetName& preset);

        QScopedPointer<Ui::ResolutionSettingItemWidget> m_ui;
        ResoultionWidgetType m_type;
        AZStd::string m_platform;
        ImageProcessingAtom::TextureSettings* m_textureSetting;
        EditorTextureSetting* m_editorTextureSetting;
        const ImageProcessingAtom::PresetSettings* m_preset;
        //Cached list of calculated final resolution info based on different reduce levels
        AZStd::list<ResolutionInfo> m_resolutionInfos;
        //Final reduce level range
        unsigned int m_maxReduce;
        unsigned int m_minReduce;
    };
} //namespace ImageProcessingAtomEditor

