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
#include "TimelineContent.h"
#include "Serialization.h"

SERIALIZATION_ENUM_BEGIN_NESTED(STimelineElement, ECaps, "Capabilities")
SERIALIZATION_ENUM_VALUE_NESTED(STimelineElement, CAP_SELECT, "Select")
SERIALIZATION_ENUM_VALUE_NESTED(STimelineElement, CAP_MOVE, "Move")
SERIALIZATION_ENUM_VALUE_NESTED(STimelineElement, CAP_DELETE, "Delete")
SERIALIZATION_ENUM_VALUE_NESTED(STimelineElement, CAP_CHANGE_DURATION, "Change Duration")
SERIALIZATION_ENUM_END()
