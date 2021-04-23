#pragma once

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

        bool event(QEvent* e) override;

    Q_SIGNALS:
        void clicked();

    protected:
        void paintEvent(QPaintEvent* e) override;
        void enterEvent(QEvent* e) override;
        void leaveEvent(QEvent* e) override;

    private:
        Thumbnailer::SharedThumbnailKey m_key;
        Thumbnailer::ThumbnailWidget* m_thumbnail = nullptr;
        Thumbnailer::ThumbnailWidget* m_thumbnailEnlarged = nullptr;
        QLabel* m_emptyThumbnail = nullptr;
        AspectRatioAwarePixmapWidget* m_dropDownArrow = nullptr;
    };
}
