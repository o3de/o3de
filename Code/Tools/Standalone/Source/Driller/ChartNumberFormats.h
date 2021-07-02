/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CHARTNUMBERFORMATS_H
#define CHARTNUMBERFORMATS_H

#include <AzCore/base.h>
#include <QString>

#pragma once

namespace DrillerCharts
{
    QString FriendlyFormat(AZ::s64 n);
}

#endif //CHARTNUMBERFORMATS_H
