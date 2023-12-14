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
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QWidget>
#endif
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! A widget used to display thumbnail
        class ThumbnailWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            explicit ThumbnailWidget(QWidget* parent = nullptr);
            ~ThumbnailWidget() override = default;

            //! Call this to set what thumbnail widget will display
            void SetThumbnailKey(SharedThumbnailKey key);
            //! Remove current thumbnail
            void ClearThumbnail();

            int heightForWidth(int w) const override;
            QSize sizeHint() const override;

        protected:
            void paintEvent(QPaintEvent* event) override;

        private:
            SharedThumbnailKey m_key;

        private Q_SLOTS:
            void RepaintThumbnail();
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
