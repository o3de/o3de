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
#include <Source/BuilderSettings/BuilderSettingManager.h>
#include <Source/Editor/EditorCommon.h>
#include <Editor/PresetInfoPopup.h>
#endif

class QCheckBox;
namespace Ui
{
    class TexturePresetSelectionWidget;
}

namespace ImageProcessingEditor
{
    class TexturePresetSelectionWidget
        : public QWidget
        , protected EditorInternalNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TexturePresetSelectionWidget, AZ::SystemAllocator, 0);
        explicit TexturePresetSelectionWidget(EditorTextureSetting& texureSetting, QWidget* parent = nullptr);
        ~TexturePresetSelectionWidget();

    public slots:
        void OnCheckBoxStateChanged(bool checked);
        void OnRestButton();
        void OnChangePreset(int index);
        void OnPresetInfoButton();

    protected:
        ////////////////////////////////////////////////////////////////////////
        //EditorInternalNotificationBus
        void OnEditorSettingsChanged(bool needRefresh, const AZStd::string& platform);
        ////////////////////////////////////////////////////////////////////////

    private:
        QScopedPointer<Ui::TexturePresetSelectionWidget> m_ui;
        AZStd::set<AZStd::string> m_presetList;
        EditorTextureSetting* m_textureSetting;
        QScopedPointer<PresetInfoPopup> m_presetPopup;
        bool IsMatchingWithFileMask(const AZStd::string& filename, const AZStd::string& fileMask);
        void SetPresetConvention(const ImageProcessing::PresetSettings* presetSettings);
        void SetCheckBoxReadOnly(QCheckBox* checkBox, bool readOnly);

        bool m_listAllPresets = true;
    };
} //namespace ImageProcessingEditor

