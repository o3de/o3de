/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#endif

class QPaintEvent;

namespace AzQtComponents::Internal
{
    //! A rectangle of a single color.
    //! To be used when filling some space with a solid color
    //! is not feasible through styling.
    class AZ_QT_COMPONENTS_API RectangleWidget : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(QColor color READ color WRITE setColor)
    public:
        explicit RectangleWidget(QWidget* parent);

        //! Set the color with which the rectangle will be painted.
        void setColor(const QColor& color);
        //! Get the current color of the rectangle.
        [[nodiscard]] QColor color() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QColor m_color = Qt::white;
    };

} // namespace AzQtComponents::Internal
