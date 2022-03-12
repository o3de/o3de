/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AZAutoSizingScrollArea.hxx"

#include <qscrollbar.h>

namespace AzToolsFramework
{

    AZAutoSizingScrollArea::AZAutoSizingScrollArea(QWidget* parent)
        : QScrollArea(parent)        
    {    
    }

    // this code was copied from the regular implementation of the same function in QScrollArea, but converted
    // the private calls to public calls and removed the cache.
    QSize AZAutoSizingScrollArea::sizeHint() const
    {
        int initialSize = 2 * frameWidth();
        QSize sizeHint(initialSize, initialSize);
        
        if (widget())
        {
            sizeHint += this->widgetResizable() ? widget()->sizeHint() : widget()->size();
        }
        else
        {
            // If we don't have a widget, we want to reserve some space visually for ourselves.            
            int fontHeight = fontMetrics().height();
            sizeHint += QSize(2 * fontHeight, 2 * fontHeight);
        }

        if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
        {
            sizeHint.setWidth(sizeHint.width() + verticalScrollBar()->sizeHint().width());
        }

        if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
        {
            sizeHint.setHeight(sizeHint.height() + horizontalScrollBar()->sizeHint().height());
        }

        return sizeHint;
    }

}

#include "UI/UICore/moc_AZAutoSizingScrollArea.cpp"
