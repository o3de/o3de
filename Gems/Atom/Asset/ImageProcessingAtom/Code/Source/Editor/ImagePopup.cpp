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

#include <ImageProcessing_precompiled.h>
#include "ImagePopup.h"

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <Source/Editor/ui_ImagePopup.h>
#include <QLabel>
#include <QPixmap>
AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtomEditor
{
    ImagePopup::ImagePopup(QImage previewImage, QWidget* parent /*= nullptr*/)
        : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::Popup)
        , m_ui(new Ui::ImagePopup)
    {
        m_ui->setupUi(this);
        m_previewImage = previewImage;

        if (!m_previewImage.isNull())
        {
            int height = previewImage.height();
            int width = previewImage.width();

            this->resize(width, height);
            m_ui->imageLabel->resize(width, height);
            QPixmap pixmap = QPixmap::fromImage(previewImage);
            m_ui->imageLabel->setPixmap(pixmap);

            this->setFocusPolicy(Qt::FocusPolicy::NoFocus);
            this->setModal(false);
        }
    }

    ImagePopup::~ImagePopup()
    {
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_ImagePopup.cpp>
