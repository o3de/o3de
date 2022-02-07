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

        void SetHeight(uint32 height)                       { m_height = height; }
        MCORE_INLINE uint32 GetHeight() const               { return m_height; }

        void RenderHeader(QPainter& painter, uint32 width, int32 startY);

        // @param startTime The time in seconds of the left border of the visible area in the widget.
        void RenderData(QPainter& painter, uint32 width, int32 startY, double startTime, double endTime, double animationLength, double clipStartTime, double clipEndTime);

        MCORE_INLINE size_t GetNumElements() const                      { return m_elements.size(); }
        MCORE_INLINE TimeTrackElement* GetElement(size_t index) const   { return m_elements[index]; }
        void AddElement(TimeTrackElement* elem)                         { elem->SetTrack(this); m_elements.push_back(elem); }
        void RemoveElement(TimeTrackElement* elem, bool delFromMem = true)
        {
            m_elements.erase(AZStd::remove(m_elements.begin(), m_elements.end(), elem), m_elements.end());
            if (delFromMem)
            {
                delete elem;
            }
        }
        void RemoveElement(size_t index, bool delFromMem = true)
        {
            if (delFromMem)
            {
                delete m_elements[index];
            }
            m_elements.erase(m_elements.begin() + index);
        }
        void RemoveAllElements(bool delFromMem = true);
        void SetElementCount(size_t count)
        {
            m_elements.resize(count);
        }

        size_t CalcNumSelectedElements() const;
        TimeTrackElement* GetFirstSelectedElement() const;
        void RangeSelectElements(size_t elementStartNr, size_t elementEndNr);
        void SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode);

        MCORE_INLINE TimeViewPlugin* GetPlugin()            { return m_plugin; }
        MCORE_INLINE void SetStartY(uint32 y)               { m_startY = y; }
        MCORE_INLINE uint32 GetStartY() const               { return m_startY; }
        bool GetIsInside(uint32 y) const                    { return (y >= m_startY) && (y <= (m_startY + m_height)); }

        void SetName(const char* name)                      { m_name = name; }
        const char* GetName() const                         { return m_name.c_str(); }

        bool GetIsEnabled() const                           { return m_enabled; }
        void SetIsEnabled(bool enabled)                     { m_enabled = enabled; }

        bool GetIsDeletable() const                         { return m_deletable; }
        void SetIsDeletable(bool isDeletable)               { m_deletable = isDeletable; }

        bool GetIsVisible() const                           { return m_visible; }
        void SetIsVisible(bool visible)                     { m_visible = visible; }

        MCORE_INLINE bool GetIsHighlighted() const          { return m_isHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool enabled)    { m_isHighlighted = enabled; }

        TimeTrackElement* GetElementAt(int32 x, int32 y) const;

    protected:
        AZStd::string                       m_name;
        uint32                              m_height;
        uint32                              m_startY;
        QFont                               m_font;
        QBrush                              m_brushHeaderBg;
        QColor                              m_brushDataBg;
        QColor                              m_brushDataDisabledBg;
        QPen                                m_penText;
        TimeViewPlugin*                     m_plugin;
        AZStd::vector<TimeTrackElement*>    m_elements;
        bool                                m_enabled;
        bool                                m_visible;
        bool                                m_deletable;
        bool                                m_isHighlighted;
    };
}   // namespace EMStudio
