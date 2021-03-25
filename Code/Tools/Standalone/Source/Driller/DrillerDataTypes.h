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