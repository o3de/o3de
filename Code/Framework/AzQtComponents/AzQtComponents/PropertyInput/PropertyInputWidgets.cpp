/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/PropertyInput/PropertyInputWidgets.h>

#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>

constexpr int FieldMargins = 17;
constexpr int SpinBoxWidth = 64;

namespace AzQtComponents
{
    PropertyInputDoubleWidget::PropertyInputDoubleWidget()
    {
        // Create Label.
        m_label = new QLabel(this);
        m_label->setContentsMargins(QMargins(0, 0, FieldMargins / 2, 0));

        // Create SpinBox.
        m_spinBox = new AzQtComponents::DoubleSpinBox(this);
        m_spinBox->setFixedWidth(SpinBoxWidth);

        // Trigger OnSpinBoxValueChanged when the user changes the value.
        connect(
            m_spinBox,
            QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged),
            this,
            [&](double value)
            {
                OnSpinBoxValueChanged(value);
            }
        );

        // Clear focus when the user is done editing (especially if this is added to a menu).
        QObject::connect(m_spinBox, &AzQtComponents::DoubleSpinBox::editingFinished, &QWidget::clearFocus);

        // Add to Layout.
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(QMargins(FieldMargins, 0, FieldMargins, 0));
        layout->addWidget(m_label);
        layout->addWidget(m_spinBox);
    }

    PropertyInputDoubleWidget::~PropertyInputDoubleWidget()
    {
    }

    bool PropertyInputDoubleWidget::event(QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            {
                // Prevent Enter events from being propagated up.
                event->accept();
                return true;
            }
        }

        return false;
    }

} // namespace AzQtComponents
