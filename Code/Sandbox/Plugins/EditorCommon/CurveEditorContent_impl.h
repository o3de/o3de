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
#include "CurveEditorContent.h"

SERIALIZATION_ENUM_BEGIN_NESTED(SCurveEditorKey, ETangentType, "TangentType")
SERIALIZATION_ENUM_VALUE_NESTED(SCurveEditorKey, eTangentType_Custom, "Custom")
SERIALIZATION_ENUM_VALUE_NESTED(SCurveEditorKey, eTangentType_Auto, "Smooth")
SERIALIZATION_ENUM_VALUE_NESTED(SCurveEditorKey, eTangentType_Zero, "Zero")
SERIALIZATION_ENUM_VALUE_NESTED(SCurveEditorKey, eTangentType_Step, "Step")
SERIALIZATION_ENUM_VALUE_NESTED(SCurveEditorKey, eTangentType_Linear, "Linear")
SERIALIZATION_ENUM_END()
