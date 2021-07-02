/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <QPen>
#include <QFont>
#include <QBrush>
#endif

QT_FORWARD_DECLARE_CLASS(QBrush)
QT_FORWARD_DECLARE_CLASS(QPaintEvent)


namespace EMStudio
{
    // forward declarations
    class TimeTrack;

    class TimeTrackElement
    {
        MCORE_MEMORYOBJECTCATEGORY(TimeTrackElement, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);
    public:
        enum
        {
            RESIZEPOINT_START   = 0,
            RESIZEPOINT_END     = 1
        };

        TimeTrackElement(const char* name, TimeTrack* timeTrack, uint32 elementNumber = MCORE_INVALIDINDEX32, QColor color = QColor(0, 0, 0));
        virtual ~TimeTrackElement();

        MCORE_INLINE double GetStartTime() const                            { return mStartTime; }
        MCORE_INLINE double GetEndTime() const                              { return mEndTime; }
        MCORE_INLINE bool GetIsSelected() const                             { return mIsSelected; }
        MCORE_INLINE TimeTrack* GetTrack()                                  { return mTrack; }
        MCORE_INLINE uint32 GetElementNumber() const                        { return mElementNumber; }
        QColor GetColor() const                                             { return mColor; }

        void SetIsSelected(bool selected)                                   { mIsSelected = selected; }
        void SetStartTime(double startTime)                                 { mStartTime = startTime; }
        void SetEndTime(double endTime)                                     { mEndTime = endTime; }
        void SetName(const char* name)                                      { mName = name; }
        void SetToolTip(const char* toolTip)                                { mToolTip = toolTip; }
        void SetTrack(TimeTrack* track)                                     { mTrack = track; }
        void SetElementNumber(uint32 elementNumber)                         { mElementNumber = elementNumber; }
        void SetColor(QColor color)                                         { mColor = color; }

        const QString& GetName() const                                      { return mName; }
        const QString& GetToolTip() const                                   { return mToolTip; }
        const QFont& GetFont() const                                        { return mFont; }

        virtual void Render(QPainter& painter, bool isTrackEnabled);
        virtual bool SnapTime(double* inOutTime, double snapTreshold) const;
        virtual bool FindResizePoint(int32 x, int32 y, uint32* outID);
        virtual uint32 HandleResize(uint32 id, double timeDelta, double snapThreshold);

        bool CheckIfIsInside(int32 x, int32 y) const;
        void MoveRelative(double timeDelta);
        void CalcDimensions(int32* outStartX, int32* outStartY, int32* outWidth, int32* outHeight) const;
        QRect CalcRect();

        bool GetShowTimeHandles() const                                     { return mShowTimeHandles; }
        void SetShowTimeHandles(bool show)                                  { mShowTimeHandles = show; }
        void SetShowToolTip(bool show)                                      { mShowToolTip = show; }
        bool GetShowToolTip() const                                         { return mShowToolTip; }

        bool GetIsVisible() const                                           { return mVisible; }
        void SetIsVisible(bool visible)                                     { mVisible = visible; }

        bool GetIsCut() const                                               { return mIsCut; }
        void SetIsCut(bool cut)                                             { mIsCut = cut; }

        MCORE_INLINE bool GetIsHighlighted() const                          { return mIsHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool enabled)                    { mIsHighlighted = enabled; }

    protected:
        QFont               mFont;
        QBrush              mBrush;
        TimeTrack*          mTrack;
        double              mStartTime;
        double              mEndTime;
        QString             mName;
        QString             mToolTip;
        QColor              mColor;
        uint32              mElementNumber;
        QPoint              mTickPoints[6];

        bool                mVisible;
        bool                mIsCut;
        bool                mIsSelected;
        bool                mShowTimeHandles;
        bool                mShowToolTip;
        bool                mIsHighlighted;

        static QColor       mHighlightedColor;
        static QColor       mTextColor;
        static QColor       mHighlightedTextColor;
        static int32        mTickHalfWidth;
    };
}   // namespace EMStudio
