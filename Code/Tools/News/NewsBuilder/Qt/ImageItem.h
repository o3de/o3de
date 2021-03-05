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