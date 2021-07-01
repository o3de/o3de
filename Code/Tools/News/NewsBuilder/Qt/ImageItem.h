/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class ImageItemWidget;
}

namespace News {
    class Resource;

    //! A clickable image in selectImage control
    class ImageItem
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit ImageItem(Resource& resource);
        ~ImageItem();

        void SetSelect(bool selected) const;
        Resource& GetResource() const;

    Q_SIGNALS:
        void selectSignal(ImageItem* imageItem);

    private:
        QScopedPointer<Ui::ImageItemWidget> m_ui;
        const static char* SELECTED_CSS;
        const static char* UN_SELECTED_CSS;

        Resource& m_resource;
    private Q_SLOTS:
        void imageClickedSlot();
    };
}
