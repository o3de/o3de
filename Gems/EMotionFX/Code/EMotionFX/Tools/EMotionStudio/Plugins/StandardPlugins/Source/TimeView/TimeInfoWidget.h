/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        bool GetIsOverwriteMode()                       { return mOverwriteMode; }
        void SetIsOverwriteMode(bool active)            { mOverwriteMode = active; }
        void SetOverwriteTime(double startTime, double endTime);

    protected:
        void paintEvent(QPaintEvent* event);
        QSize sizeHint() const;

    private:
        QFont           mFont;
        QFont           mOverwriteFont;
        QBrush          mBrushBackground;
        QPen            mPenText;
        QPen            mPenTextFocus;
        AZStd::string   mCurTimeString;
        AZStd::string   mOverwriteTimeString;
        TimeViewPlugin* mPlugin;
        double          mOverwriteStartTime;
        double          mOverwriteEndTime;
        bool            mOverwriteMode;
        bool            mShowOverwriteStartTime = false;

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    };
}   // namespace EMStudio