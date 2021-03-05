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
#include <QDialog>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/StyledDialog.h>
#endif

namespace Ui
{
    class ImagePopup;
}

namespace ImageProcessingEditor
{
    class ImagePopup
        : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ImagePopup, AZ::SystemAllocator, 0);
        explicit ImagePopup(QImage previewImage, QWidget* parent = nullptr);
        ~ImagePopup();

    private:
        QScopedPointer<Ui::ImagePopup> m_ui;
        QImage m_previewImage;
        
    };
} //namespace ImageProcessingEditor

