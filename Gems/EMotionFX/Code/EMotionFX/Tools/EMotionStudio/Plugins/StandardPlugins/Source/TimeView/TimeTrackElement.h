/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        TimeTrackElement(const char* name, TimeTrack* timeTrack, size_t elementNumber = InvalidIndex, QColor color = QColor(0, 0, 0));
        virtual ~TimeTrackElement();

        MCORE_INLINE double GetStartTime() const                            { return m_startTime; }
        MCORE_INLINE double GetEndTime() const                              { return m_endTime; }
        MCORE_INLINE bool GetIsSelected() const                             { return m_isSelected; }
        MCORE_INLINE TimeTrack* GetTrack()                                  { return m_track; }
        MCORE_INLINE size_t GetElementNumber() const                        { return m_elementNumber; }
        QColor GetColor() const                                             { return m_color; }

        void SetIsSelected(bool selected)                                   { m_isSelected = selected; }
        void SetStartTime(double startTime)                                 { m_startTime = startTime; }
        void SetEndTime(double endTime)                                     { m_endTime = endTime; }
        void SetName(const char* name)                                      { m_name = name; }
        void SetToolTip(const char* toolTip)                                { m_toolTip = toolTip; }
        void SetTrack(TimeTrack* track)                                     { m_track = track; }
        void SetElementNumber(size_t elementNumber)                         { m_elementNumber = elementNumber; }
        void SetColor(QColor color)                                         { m_color = color; }

        const QString& GetName() const                                      { return m_name; }
        const QString& GetToolTip() const                                   { return m_toolTip; }
        const QFont& GetFont() const                                        { return m_font; }

        virtual void Render(QPainter& painter, bool isTrackEnabled);
        virtual bool SnapTime(double* inOutTime, double snapTreshold) const;
        virtual bool FindResizePoint(int32 x, int32 y, uint32* outID);
        virtual uint32 HandleResize(uint32 id, double timeDelta, double snapThreshold);

        bool CheckIfIsInside(int32 x, int32 y) const;
        void MoveRelative(double timeDelta);
        void CalcDimensions(int32* outStartX, int32* outStartY, int32* outWidth, int32* outHeight) const;
        QRect CalcRect();

        bool GetShowTimeHandles() const                                     { return m_showTimeHandles; }
        void SetShowTimeHandles(bool show)                                  { m_showTimeHandles = show; }
        void SetShowToolTip(bool show)                                      { m_showToolTip = show; }
        bool GetShowToolTip() const                                         { return m_showToolTip; }

        bool GetIsVisible() const                                           { return m_visible; }
        void SetIsVisible(bool visible)                                     { m_visible = visible; }

        bool GetIsCut() const                                               { return m_isCut; }
        void SetIsCut(bool cut)                                             { m_isCut = cut; }

        MCORE_INLINE bool GetIsHighlighted() const                          { return m_isHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool enabled)                    { m_isHighlighted = enabled; }

    protected:
        QFont               m_font;
        QBrush              m_brush;
        TimeTrack*          m_track;
        double              m_startTime;
        double              m_endTime;
        QString             m_name;
        QString             m_toolTip;
        QColor              m_color;
        size_t              m_elementNumber;
        QPoint              m_tickPoints[6];

        bool                m_visible;
        bool                m_isCut;
        bool                m_isSelected;
        bool                m_showTimeHandles;
        bool                m_showToolTip;
        bool                m_isHighlighted;

        static QColor       s_highlightedColor;
        static QColor       s_textColor;
        static QColor       s_highlightedTextColor;
        static int32        s_tickHalfWidth;
    };
}   // namespace EMStudio
