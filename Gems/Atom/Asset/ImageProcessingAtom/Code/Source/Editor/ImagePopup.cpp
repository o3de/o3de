/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
