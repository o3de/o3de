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
#include <Source/BuilderSettings/BuilderSettingManager.h>
#include <Source/Editor/EditorCommon.h>
#include <Editor/PresetInfoPopup.h>
#endif

class QCheckBox;
namespace Ui
{
    class TexturePresetSelectionWidget;
}

namespace ImageProcessingAtomEditor
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
        AZStd::unordered_set<ImageProcessingAtom::PresetName> m_presetList;
        EditorTextureSetting* m_textureSetting;
        QScopedPointer<PresetInfoPopup> m_presetPopup;
        bool IsMatchingWithFileMask(const AZStd::string& filename, const AZStd::string& fileMask);
        void SetPresetConvention(const ImageProcessingAtom::PresetSettings* presetSettings);
        void SetCheckBoxReadOnly(QCheckBox* checkBox, bool readOnly);

        bool m_listAllPresets = true;
    };
} //namespace ImageProcessingAtomEditor

