/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/numeric.h>
#include "TimeTrack.h"
#include "TimeViewPlugin.h"
#include <QPainter>
#include <QPaintEvent>
#include <MCore/Source/LogManager.h>


namespace EMStudio
{
    // the constructor
    TimeTrack::TimeTrack(TimeViewPlugin* plugin)
    {
        mPlugin         = plugin;
        mHeight         = 20;
        mStartY         = 0;
        mEnabled        = false;
        mIsHighlighted  = false;
        mVisible        = false;
        mDeletable      = true;

        // init font
        mFont.setPixelSize(14);
        //mFont.setBold( true );

        // init brushes and pens
        mBrushDataBG            = QColor(60, 65, 70);
        //mBrushDataBG          = QColor(50, 50, 50);
        mBrushDataDisabledBG    = QColor(50, 50, 50);// use the same color //QBrush( QColor(33, 33, 33) );
        mBrushHeaderBG          = QBrush(QColor(30, 30, 30));
        mPenText                = QPen(QColor(255, 255, 255));
    }


    // destructor
    TimeTrack::~TimeTrack()
    {
        RemoveAllElements(true);
    }


    // render the timetrack
    void TimeTrack::RenderData(QPainter& painter, uint32 width, int32 startY, double startTime, double endTime, double animationLength, double clipStartTime, double clipEndTime)
    {
        MCORE_UNUSED(width);

        if (mVisible == false)
        {
            return;
        }

        int32 animEndPixel  = aznumeric_cast<int32>(mPlugin->TimeToPixel(animationLength));
        int32 clipStartPixel = aznumeric_cast<int32>(mPlugin->TimeToPixel(clipStartTime));
        int32 clipEndPixel  = aznumeric_cast<int32>(mPlugin->TimeToPixel(clipEndTime));

        // fill the background
        uint32 height = GetHeight();

        QRect enabledRect(clipStartPixel,   startY, clipEndPixel - clipStartPixel,    height);
        QRect clipStartRect(0,              startY, clipStartPixel,                 height);
        QRect clipEndRect(clipEndPixel,     startY, animEndPixel - clipEndPixel,      height);

        QColor disabledBGColor  = mBrushDataDisabledBG;
        QColor bgColor          = mBrushDataBG;

        // make the colors a bit lighter so that we see some highlighting effect
        if (mIsHighlighted)
        {
            disabledBGColor = disabledBGColor.lighter(120);
            bgColor         = bgColor.lighter(120);
        }

        if (mEnabled)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(disabledBGColor);
            painter.drawRect(clipStartRect);

            painter.setBrush(bgColor);
            painter.drawRect(enabledRect);

            painter.setBrush(disabledBGColor);
            painter.drawRect(clipEndRect);

            //painter.fillRect( clipStartRect, disabledBGColor );
            //painter.fillRect( enabledRect, bgColor );
            //painter.fillRect( clipEndRect, disabledBGColor );
        }
        else
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(disabledBGColor);
            QRect rect(0, startY, animEndPixel, height);
            painter.drawRect(rect);
            //painter.fillRect( rect, disabledBGColor );
        }

        // render all elements
        //uint32 numRenderedElements = 0;
        const size_t numElems = mElements.size();
        for (size_t i = 0; i < numElems; ++i)
        {
            TimeTrackElement* element = mElements[i];

            // skip rendering the element in case it is not inside the visible area in the widget
            if (element->GetEndTime() < startTime ||
                element->GetStartTime() > endTime)
            {
                continue;
            }

            bool enabled = mEnabled;

            // make sure we render the motion event as disabled as soon as it is in the clipped area
            if (element->GetEndTime() < clipStartTime ||
                element->GetStartTime() > clipEndTime)
            {
                enabled = false;
            }

            element->Render(painter, enabled);
            //numRenderedElements++;
        }

        //MCore::LOG("Elements=%i", numRenderedElements);
    }


    // render the track header
    void TimeTrack::RenderHeader(QPainter& painter, uint32 width, int32 startY)
    {
        if (mVisible == false)
        {
            return;
        }

        // fill the background
        uint32 height = GetHeight();
        QRect rect(0, startY, width, height);

        painter.setPen(Qt::NoPen);
        painter.setBrush(mBrushHeaderBG);
        painter.drawRect(rect);

        // render the name
        QTextOption options;
        options.setAlignment(Qt::AlignCenter);
        painter.setPen(mPenText);
        painter.drawText(rect, mName.c_str(), options);
    }


    // delete all elements
    void TimeTrack::RemoveAllElements(bool delFromMem)
    {
        if (delFromMem)
        {
            for (TimeTrackElement* mElement : mElements)
            {
                delete mElement;
            }
        }

        mElements.clear();
    }


    // get the track element at a given pixel
    TimeTrackElement* TimeTrack::GetElementAt(int32 x, int32 y) const
    {
        if (mVisible == false)
        {
            return nullptr;
        }

        // for all elements
        for (TimeTrackElement* element : mElements)
        {
            // check if its inside
            if (element->GetIsVisible() == false)
            {
                continue;
            }

            if (element->CheckIfIsInside(x, y))
            {
                return element;
            }
        }

        return nullptr;
    }


    // calculate the number of selected elements
    size_t TimeTrack::CalcNumSelectedElements() const
    {
        if (mVisible == false)
        {
            return 0;
        }

        return AZStd::accumulate(begin(mElements), end(mElements), size_t{0}, [](size_t total, const TimeTrackElement* element)
        {
            return total + element->GetIsSelected();
        });
    }


    // find and return the first of the selected elements
    TimeTrackElement* TimeTrack::GetFirstSelectedElement() const
    {
        if (mVisible == false)
        {
            return nullptr;
        }

        const auto foundElement = AZStd::find_if(begin(mElements), end(mElements), [](const TimeTrackElement* element)
        {
            return element->GetIsSelected();
        });
        return foundElement != end(mElements) ? *foundElement : nullptr;
    }


    // select elements in a given range, unselect all other
    void TimeTrack::RangeSelectElements(size_t elementStartNr, size_t elementEndNr)
    {
        // make sure the start number is actually the smaller one of the two values
        const size_t startNr    = AZStd::min(elementStartNr, elementEndNr);
        const size_t endNr      = AZStd::max(elementStartNr, elementEndNr);

        // get the number of elements and iterate through them
        const size_t numElems = mElements.size();
        for (size_t i = 0; i < numElems; ++i)
        {
            const size_t        elementNr   = i;
            TimeTrackElement*   element     = mElements[i];

            // check if the current element is in range
            if (elementNr >= startNr && elementNr <= endNr)
            {
                element->SetIsSelected(true);
            }
            else
            {
                element->SetIsSelected(false);
            }
        }
    }


    // select all elements within a given rect
    void TimeTrack::SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode)
    {
        // get the number of elements and iterate through them
        for (TimeTrackElement* element : mElements)
        {
            // get the current element and the corresponding rect
            QRect               elementRect = element->CalcRect();

            if (elementRect.intersects(rect))
            {
                if (toggleMode)
                {
                    element->SetIsSelected(!element->GetIsSelected());
                }
                else
                {
                    element->SetIsSelected(select);
                }
            }
            else
            {
                if (overwriteCurSelection)
                {
                    element->SetIsSelected(false);
                }
            }
        }
    }
} // namespace EMStudio
