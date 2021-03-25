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