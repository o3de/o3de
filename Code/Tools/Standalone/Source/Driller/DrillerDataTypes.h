/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DATATYPES_H
#define DRILLER_DATATYPES_H

#include <AzCore/base.h>

namespace Driller
{
    typedef AZ::s32 FrameNumberType;
    typedef AZ::s64 EventNumberType;

    static const EventNumberType kInvalidEventIndex = -1;

    enum class CaptureMode
    {
        Unknown,
        Configuration,
        Capturing,
        Inspecting
    };
}

#endif
