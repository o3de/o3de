/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DHQComboBox.hxx"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: '...' needs to have dll-interface to be used by clients of class '...'
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QStylePainter>
#include <QToolTip>
#include <QWheelEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    DHQComboBox::DHQComboBox(QWidget* parent) : QComboBox(parent) 
    {
        view()->setTextElideMode(Qt::ElideNone);
    }

    void DHQComboBox::showPopup()
    {
        // make sure the combobox pop-up is wide enough to accommodate its text
        QFontMetrics comboMetrics(view()->fontMetrics());
        QMargins theMargins = view()->contentsMargins();
        const int marginsWidth = theMargins.left() + theMargins.right();

        int widestStringWidth = 0;
        for (int index = 0, numIndices = count(); index < numIndices; ++index)
        {
            const int newMax = comboMetrics.boundingRect(itemText(index)).width() + marginsWidth;
            
            if (newMax > widestStringWidth)
                widestStringWidth = newMax;
        }

        view()->setMinimumWidth(widestStringWidth);
        QComboBox::showPopup();
    }

    void DHQComboBox::SetHeaderOverride(const QString& overrideString)
    {
        m_headerOverride = overrideString;
    }

    void DHQComboBox::wheelEvent(QWheelEvent* e)
    {
        if (hasFocus())
        {
            QComboBox::wheelEvent(e);
        }
        else
        {
            e->ignore();
        }
    }

    bool DHQComboBox::event(QEvent* event)
    {
        if (event->type() == QEvent::ToolTip && !m_headerOverride.isEmpty())
        {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
            QToolTip::showText(helpEvent->globalPos(), "<i>- Multiple selected -</i>");
            return true;
        }
        return QWidget::event(event);
    }

    void DHQComboBox::paintEvent(QPaintEvent* event)
    {
        if (!m_headerOverride.isEmpty())
        {
            // We need to override the printing of the ComboBox header when we want the selection box
            // to be in italics as there is a problem with Qt stylesheets that ignore {font-style: *}
            // when used in the QAbstractListView.
            QStylePainter painter(this);

            QStyleOptionComboBox opt;

            QFont font = painter.font();

            painter.save();

            this->initStyleOption(&opt);

            opt.currentText = m_headerOverride;
            opt.iconSize = QSize(0, 0);

            painter.drawComplexControl(QStyle::CC_ComboBox, opt);

            font.setItalic(true);
            painter.setFont(font);
            painter.drawControl(QStyle::CE_ComboBoxLabel, opt);

            painter.restore();
        }
        else
        {
            QComboBox::paintEvent(event);
        }
    }

}
