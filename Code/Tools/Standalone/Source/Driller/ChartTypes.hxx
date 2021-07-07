/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef PROFILER_CHARTTYPES_H
#define PROFILER_CHARTTYPES_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#endif

namespace Charts
{
    enum class AxisType
    {
        Horizontal,
        Vertical
    };

    // Plug this guy into the chart if you wish to custom format axes
    class QAbstractAxisFormatter  : public QObject
    {
            Q_OBJECT
    public:
        QAbstractAxisFormatter(QObject *pParent);

        /** convertAxisValueToText
        * Expects you to return a QString with the value for the axis.
        * value : value it wants the label for.
        * minDisplayedValue : what value is at the bottom of the axis in the display
        * maxDisplayedValue : what value is at the top of this axis in the display
        * divisionSize : what each tickmark is, in domain units.
        * So for example, if the axis is from 938 to 2114 and its got a tickmark every 250, and it wants to know what you'd like it to draw for 1250
        * then you'll get Value = 1250, minDisplayedValue = 1000, maxDisplayedValue = 2000, divisionSize = 250.
        */
        virtual QString convertAxisValueToText(AxisType axisType, float value, float minDisplayedValue, float maxDisplayedValue, float divisionSize) = 0;
    };
}

#endif
