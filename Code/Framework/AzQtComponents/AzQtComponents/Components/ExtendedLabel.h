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
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4244 4800, "-Wunknown-warning-option") // 4251: class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QLabel>
AZ_POP_DISABLE_WARNING

#include <AzQtComponents/AzQtComponentsAPI.h>
#endif

//! An improved QLabel control with better QPixmap resize functionality and click signal
/*!
    ExtendedLabel allows to automatically downscale its QPixmap
    while preserving image's aspect ratio. The maximum QPixmap size is set
    to the minimum of [QLabel size] OR [maxPixMapWidth and maxPixMapHeight values].
*/
namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ExtendedLabel
        : public QLabel
    {
        Q_OBJECT

    public:
        explicit ExtendedLabel(QWidget* parent = nullptr);

        int heightForWidth(int width) const override;

    public Q_SLOTS:
        void setPixmap(const QPixmap& p);
        void resizeEvent(QResizeEvent* event) override;

    Q_SIGNALS:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;

    private:
        QPixmap m_pix;
        QSize m_pixmapSize;

        void rescale();

        static double aspectRatio(int w, int h);
    };
}
