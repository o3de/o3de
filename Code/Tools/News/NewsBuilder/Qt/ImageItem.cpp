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

#include "ImageItem.h"
#include "NewsShared/ResourceManagement/Resource.h"

#include "Qt/ui_ImageItem.h"

namespace News
{

    const char* ImageItem::SELECTED_CSS =
        "border: 4px solid; border-color: white; background-color: rgb(45, 45, 45);";
    const char* ImageItem::UN_SELECTED_CSS =
        "background-color: rgb(45, 45, 45);";

    ImageItem::ImageItem(Resource& resource)
        : QWidget()
        , m_ui(new Ui::ImageItemWidget)
        , m_resource(resource)
    {
        m_ui->setupUi(this);

        QPixmap pixmap;
        pixmap.loadFromData(m_resource.GetData());
        m_ui->ImageLabel->setPixmap(pixmap);

        connect(m_ui->ImageLabel, &AzQtComponents::ExtendedLabel::clicked, this, &ImageItem::imageClickedSlot);
    }

    void ImageItem::imageClickedSlot()
    {
        emit selectSignal(this);
    }

    ImageItem::~ImageItem() {}

    void ImageItem::SetSelect(bool selected) const
    {
        m_ui->ImageLabel->setStyleSheet(selected ? SELECTED_CSS : UN_SELECTED_CSS);
    }

    Resource& ImageItem::GetResource() const
    {
        return m_resource;
    }

#include "Qt/moc_ImageItem.cpp"

}
