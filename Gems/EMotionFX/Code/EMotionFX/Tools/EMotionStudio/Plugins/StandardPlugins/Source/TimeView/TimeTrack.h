/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Motion.h>
#include "../StandardPluginsConfig.h"
#include <QPen>
#include <QFont>
#include <QKeyEvent>
#include <QBrush>
#include "TimeTrackElement.h"
#endif

QT_FORWARD_DECLARE_CLASS(QBrush)
QT_FORWARD_DECLARE_CLASS(QPaintEvent)


namespace EMStudio
{
    // forward declarations
    class TimeViewPlugin;

    class TimeTrack
    {
        MCORE_MEMORYOBJECTCATEGORY(TimeTrack, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);
    public:
        TimeTrack(TimeViewPlugin* plugin);
        ~TimeTrack();

        void SetHeight(uint32 height)                       { mHeight = height; }
        MCORE_INLINE uint32 GetHeight() const               { return mHeight; }

        void RenderHeader(QPainter& painter, uint32 width, int32 startY);

        // @param startTime The time in seconds of the left border of the visible area in the widget.
        void RenderData(QPainter& painter, uint32 width, int32 startY, double startTime, double endTime, double animationLength, double clipStartTime, double clipEndTime);

        MCORE_INLINE uint32 GetNumElements() const                      { return static_cast<uint32>(mElements.size()); }
        MCORE_INLINE TimeTrackElement* GetElement(uint32 index) const   { return mElements[static_cast<uint32>(index)]; }
        void AddElement(TimeTrackElement* elem)                         { elem->SetTrack(this); mElements.push_back(elem); }
        void RemoveElement(TimeTrackElement* elem, bool delFromMem = true)
        {
            mElements.erase(AZStd::remove(mElements.begin(), mElements.end(), elem), mElements.end());
            if (delFromMem)
            {
                delete elem;
            }
        }
        void RemoveElement(uint32 index, bool delFromMem = true)
        {
            if (delFromMem)
            {
                delete mElements[index];
            }
            mElements.erase(mElements.begin() + index);
        }
        void RemoveAllElements(bool delFromMem = true);
        void SetElementCount(size_t count)
        {
            mElements.resize(count);
        }

        uint32 CalcNumSelectedElements() const;
        TimeTrackElement* GetFirstSelectedElement() const;
        void RangeSelectElements(uint32 elementStartNr, uint32 elementEndNr);
        void SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode);

        MCORE_INLINE TimeViewPlugin* GetPlugin()            { return mPlugin; }
        MCORE_INLINE void SetStartY(uint32 y)               { mStartY = y; }
        MCORE_INLINE uint32 GetStartY() const               { return mStartY; }
        bool GetIsInside(uint32 y)                          { return (y >= mStartY) && (y <= (mStartY + mHeight)); }

        void SetName(const char* name)                      { mName = name; }
        const char* GetName() const                         { return mName.c_str(); }

        bool GetIsEnabled() const                           { return mEnabled; }
        void SetIsEnabled(bool enabled)                     { mEnabled = enabled; }

        bool GetIsDeletable() const                         { return mDeletable; }
        void SetIsDeletable(bool isDeletable)               { mDeletable = isDeletable; }

        bool GetIsVisible() const                           { return mVisible; }
        void SetIsVisible(bool visible)                     { mVisible = visible; }

        MCORE_INLINE bool GetIsHighlighted() const          { return mIsHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool enabled)    { mIsHighlighted = enabled; }

        TimeTrackElement* GetElementAt(int32 x, int32 y);

    protected:
        AZStd::string                       mName;
        uint32                              mHeight;
        uint32                              mStartY;
        QFont                               mFont;
        QBrush                              mBrushHeaderBG;
        QColor                              mBrushDataBG;
        QColor                              mBrushDataDisabledBG;
        QPen                                mPenText;
        TimeViewPlugin*                     mPlugin;
        AZStd::vector<TimeTrackElement*>    mElements;
        bool                                mEnabled;
        bool                                mVisible;
        bool                                mDeletable;
        bool                                mIsHighlighted;
    };
}   // namespace EMStudio
