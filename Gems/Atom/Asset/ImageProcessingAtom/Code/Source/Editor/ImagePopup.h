/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/StyledDialog.h>
#endif

namespace Ui
{
    class ImagePopup;
}

namespace ImageProcessingAtomEditor
{
    class ImagePopup
        : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ImagePopup, AZ::SystemAllocator);
        explicit ImagePopup(QImage previewImage, QWidget* parent = nullptr);
        ~ImagePopup();

    private:
        QScopedPointer<Ui::ImagePopup> m_ui;
        QImage m_previewImage;
    };
} //namespace ImageProcessingAtomEditor

