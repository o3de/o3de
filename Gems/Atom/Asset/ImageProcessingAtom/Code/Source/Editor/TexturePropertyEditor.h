/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
// warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
// warning C4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4800 4251, "-Wunknown-warning-option")
#include <QDialog>
AZ_POP_DISABLE_WARNING
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Source/Editor/EditorCommon.h>
#include <Source/Editor/TexturePresetSelectionWidget.h>
#include <Source/Editor/TexturePreviewWidget.h>
#include <Source/Editor/ResolutionSettingWidget.h>
#include <Source/Editor/MipmapSettingWidget.h>
#endif

namespace Ui
{
    class TexturePropertyEditor;
}

namespace ImageProcessingAtomEditor
{
    class TexturePropertyEditor
        : public AzQtComponents::StyledDialog
        , protected EditorInternalNotificationBus::Handler
    {
        Q_OBJECT
    public:

        AZ_CLASS_ALLOCATOR(TexturePropertyEditor, AZ::SystemAllocator, 0);
        explicit TexturePropertyEditor(const AZ::Uuid& sourceTextureId, QWidget* parent = nullptr);
        ~TexturePropertyEditor();

        bool HasValidImage();

    protected:
        void OnSave();
        void OnHelp();

        ////////////////////////////////////////////////////////////////////////
        //EditorInternalNotificationBus
        void OnEditorSettingsChanged(bool needRefresh, const AZStd::string& platform) override;
        ////////////////////////////////////////////////////////////////////////

        bool event(QEvent* event) override;

    private:
        QScopedPointer<Ui::TexturePropertyEditor> m_ui;
        QScopedPointer<TexturePreviewWidget> m_previewWidget;
        QScopedPointer<TexturePresetSelectionWidget> m_presetSelectionWidget;
        QScopedPointer<ResolutionSettingWidget> m_resolutionSettingWidget;
        QScopedPointer<MipmapSettingWidget> m_mipmapSettingWidget;

        EditorTextureSetting m_textureSetting;
        bool m_validImage = true;

        void SaveTextureSetting(AZStd::string outputPath);
        void DeleteLegacySetting();
    };
} //namespace ImageProcessingAtomEditor

