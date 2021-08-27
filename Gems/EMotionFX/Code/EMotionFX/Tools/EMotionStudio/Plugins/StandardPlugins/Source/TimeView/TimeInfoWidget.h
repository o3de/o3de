/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/LogManager.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#include <QPen>
#include <QFont>
#endif

QT_FORWARD_DECLARE_CLASS(QBrush)


namespace EMStudio
{
    // forward declarations
    class TimeViewPlugin;

    class TimeInfoWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(TimeInfoWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        TimeInfoWidget(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TimeInfoWidget();

        bool GetIsOverwriteMode()                       { return m_overwriteMode; }
        void SetIsOverwriteMode(bool active)            { m_overwriteMode = active; }
        void SetOverwriteTime(double startTime, double endTime);

    protected:
        void paintEvent(QPaintEvent* event);
        QSize sizeHint() const;

    private:
        QFont           m_font;
        QFont           m_overwriteFont;
        QBrush          m_brushBackground;
        QPen            m_penText;
        QPen            m_penTextFocus;
        AZStd::string   m_curTimeString;
        AZStd::string   m_overwriteTimeString;
        TimeViewPlugin* m_plugin;
        double          m_overwriteStartTime;
        double          m_overwriteEndTime;
        bool            m_overwriteMode;
        bool            m_showOverwriteStartTime = false;

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    };
}   // namespace EMStudio
