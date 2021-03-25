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

#include "StandaloneTools_precompiled.h"

#include "ChartNumberFormats.h"

namespace DrillerCharts
{
    QString FriendlyFormat(AZ::s64 n)
    {
        QString str;

        str = QString("%L1").arg(n);

        //if (n < 0)
        //{
        //  str = "-";
        //}

        //AZ::s64 b = n / 1000000000l;
        //n %= 1000000000l;

        //AZ::s64 m = n / 1000000l;
        //n %= 1000000l;

        //AZ::s64 k = n / 1000l;
        //n %= 1000l;

        //if (b)
        //{
        //  str += QString("%0b").arg(abs(b));
        //}
        //if (m)
        //{
        //  str += QString("%0m").arg(abs(m));
        //}
        //if (k)
        //{
        //  str += QString("%0k").arg(abs(k));
        //}
        //if (n)
        //{
        //  str += QString("%0").arg(abs(n));
        //}

        return str;
    }
}
