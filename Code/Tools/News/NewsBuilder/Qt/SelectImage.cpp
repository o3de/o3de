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

#include "SelectImage.h"
#include "ImageItem.h"
#include "NewsShared/ResourceManagement/Resource.h"
#include "ResourceManagement/BuilderResourceManifest.h"

#include "Qt/ui_SelectImage.h"

namespace News
{

    const int SelectImage::MAX_COLS = 2;

    SelectImage::SelectImage(const ResourceManifest& manifest)
        : QDialog()
        , m_ui(new Ui::SelectImageDialog)
        , m_manifest(manifest)
    {
        m_ui->setupUi(this);

        auto layout = static_cast<QGridLayout*>(m_ui->scrollAreaContents->layout());
        layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        int row = 0;
        int col = 0;
        //! read all image resources and populate the container
        for (auto pResource : m_manifest)
        {
            if (pResource->GetType().compare("image") == 0)
            {
                auto imageItem = new ImageItem(*pResource);
                connect(imageItem, &ImageItem::selectSignal, this, &SelectImage::ImageSelected);
                layout->addWidget(imageItem, row, col, Qt::AlignLeft);
                m_images.append(imageItem);
                col++;
                if (col >= MAX_COLS)
                {
                    col = 0;
                    row++;
                }
            }
        }

        connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    }

    SelectImage::~SelectImage() {}

    Resource* SelectImage::GetSelected() const
    {
        return m_pSelected;
    }

    void SelectImage::ImageSelected(ImageItem* imageItem)
    {
        for (auto image : m_images)
        {
            image->SetSelect(image == imageItem);
        }
        if (imageItem)
        {
            m_pSelected = &imageItem->GetResource();
        }
    }

#include "Qt/moc_SelectImage.cpp"

}
