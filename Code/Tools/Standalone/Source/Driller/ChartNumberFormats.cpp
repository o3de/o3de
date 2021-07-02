/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
