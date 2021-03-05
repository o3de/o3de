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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_TOGGLEBUTTON_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_TOGGLEBUTTON_H
#pragma once

namespace Serialization
{
    class IArchive;

    struct ToggleButton
    {
        bool* value;

        ToggleButton(bool& value)
            : value(&value)
        {
        }
    };

    struct RadioButton
    {
        int* value;
        int buttonValue;

        RadioButton(int& value, int buttonValue)
            : value(&value)
            , buttonValue(buttonValue)
        {
        }
    };

    bool Serialize(Serialization::IArchive& ar, Serialization::ToggleButton& button, const char* name, const char* label);
    bool Serialize(Serialization::IArchive& ar, Serialization::RadioButton& button, const char* name, const char* label);
}

#include "ToggleButtonImpl.h"

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_TOGGLEBUTTON_H
