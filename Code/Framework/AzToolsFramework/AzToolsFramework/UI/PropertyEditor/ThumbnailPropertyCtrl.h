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
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    class AspectRatioAwarePixmapWidget;

    namespace Thumbnailer
    {
        class ThumbnailWidget;
    }

    //! Used by PropertyAssetCtrl to display thumbnail preview of the asset as well as additional drop-down actions
    class ThumbnailPropertyCtrl : public QWidget
    {
        Q_OBJECT
    public:
        explicit ThumbnailPropertyCtrl(QWidget* parent = nullptr);

        //! Call this to set what thumbnail widget will display
        void SetThumbnailKey(Thumbnailer::SharedThumbnailKey key, const char* contextName = "Default");

        //! Remove current thumbnail
        void ClearThumbnail();

        //! Display a clickable dropdown arrow next to the thumbnail
        void ShowDropDownArrow(bool visible);

        //! Override the thumbnail widget with a custom image
        void SetCustomThumbnailEnabled(bool enabled);

        //! Assign a custom image to display in place of thumbnail
        void SetCustomThumbnailPixmap(const QPixmap& pixmap);

    Q_SIGNALS:
        void clicked();

    private:
        void UpdateVisibility();

        bool event(QEvent* e) override;
        void paintEvent(QPaintEvent* e) override;
        void enterEvent(QEvent* e) override;
        void leaveEvent(QEvent* e) override;

        Thumbnailer::SharedThumbnailKey m_key;
        Thumbnailer::ThumbnailWidget* m_thumbnail = nullptr;
        Thumbnailer::ThumbnailWidget* m_thumbnailEnlarged = nullptr;

        QLabel* m_customThumbnail = nullptr;
        QLabel* m_customThumbnailEnlarged = nullptr;
        bool m_customThumbnailEnabled = false;

        QLabel* m_emptyThumbnail = nullptr;
        AspectRatioAwarePixmapWidget* m_dropDownArrow = nullptr;
    };
}
