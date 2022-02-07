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
#include <Editor/ImagePopup.h>
#include <QEvent>
#include <QTimer>

#include <Processing/ImagePreview.h>
#endif

namespace Ui
{
    class TexturePreviewWidget;
}

namespace ImageProcessingAtomEditor
{
    enum PreviewMode
    {
        RGB = 0,
        RRR,
        GGG,
        BBB,
        Alpha,
        RGBA,
        Count
    };

    enum class RefreshMode
    {
        Convert,    // Convert the whole image from beginning, takes longest time
        Mip,        // Generate a new mip from from converted image
        Channel,    // Generate a new channel image from converted image
        Repaint,
    };

    class TexturePreviewWidget
        : public QWidget
        , protected EditorInternalNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TexturePreviewWidget, AZ::SystemAllocator, 0);
        explicit TexturePreviewWidget(EditorTextureSetting& texureSetting, QWidget* parent = 0);
        ~TexturePreviewWidget();
        bool OnQtEvent(QEvent* event);

    public slots:
        void OnTiledChanged(bool checked);
        void OnPrevMip();
        void OnNextMip();
        void OnChangePreviewMode(int index);
        void UpdatePreview();
        void OnAlwaysRefresh();
        void OnRefreshPerClick();
        void OnRefreshClicked();

    protected:
        ////////////////////////////////////////////////////////////////////////
        //EditorInternalNotificationBus
        void OnEditorSettingsChanged(bool needRefresh, const AZStd::string& platform) override;
        ////////////////////////////////////////////////////////////////////////
        void resizeEvent(QResizeEvent* event) override;
        bool eventFilter(QObject* obj, QEvent* event) override;

    private:
        void SetUpResolutionInfo();
        void RefreshUI(bool fullRefresh = false);
        void RefreshPreviewImage(RefreshMode mode);
        void GenerateMipmap(int mip);
        void GenerateChannelImage(PreviewMode channel);
        void PaintPreviewImage();
        void SetImageLabelText(const QString& text, bool busyStatus = true);
        void RefreshWarning();

        AZStd::list<ResolutionInfo> m_resolutionInfos;
        QScopedPointer<Ui::TexturePreviewWidget> m_ui;
        EditorTextureSetting* m_textureSetting;
        int m_currentMipIndex = 0;
        bool m_previewTiled = false;
        float m_imageLabelSize = 0;
        AZStd::string m_platform;
        unsigned int m_mipCount = 1;

        ///////////////////////////////////////////
        // Preview window
        PreviewMode m_previewMode = PreviewMode::RGB;
        QScopedPointer<ImagePopup> m_previewPopup;
        AZStd::unique_ptr<ImageProcessingAtom::ImagePreview> m_previewConverter;
        ImageProcessingAtom::IImageObjectPtr m_previewImageRaw;
        QImage m_previewImages[PreviewMode::Count];
        QTimer* m_updateTimer;
        static const int s_updateInterval = 200;
        ////////////////////////////////////////////

        ////////////////////////////////////////////
        // Refresh button
        bool m_alwaysRefreshPreview = true;
        QAction* m_alwaysRefreshAction = nullptr;
        QAction* m_refreshPerClickAction = nullptr;
        QIcon m_refreshPerClickIcon;
        QIcon m_alwaysRefreshIcon;
        ////////////////////////////////////////////
    };
} //namespace ImageProcessingAtomEditor

