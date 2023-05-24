
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QLineEdit>

namespace EMStudio
{

    //! A QLineEdit whose size can be dynamically scaled. Helpful for the zoomable NodeGraph
    class ZoomableLineEdit : public QLineEdit
    {
        Q_OBJECT

    public:
        explicit ZoomableLineEdit(QWidget* parent);
        void setScale(float scale);

        // Use these functions just before showing the widget
        void setBaseSize(QSize sz);
        void setBaseFontPixelSize(qreal pointSizeF);

    private:
        float m_scale = 1.0f;
        QSize m_baseSize;
        qreal m_fontPixelSize;

        void updateScaledSize();
    };

} // namespace EMStudio
