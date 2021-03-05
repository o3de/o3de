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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_TOGGLEBUTTONIMPL_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_TOGGLEBUTTONIMPL_H
#pragma once

namespace Serialization
{
    inline bool Serialize(Serialization::IArchive& ar, Serialization::ToggleButton& button, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct::ForEdit(button), name, label);
        }
        else
        {
            return ar(*button.value, name, label);
        }
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::RadioButton& button, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct::ForEdit(button), name, label);
        }
        else
        {
            return false;
        }
    }
}
#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_TOGGLEBUTTONIMPL_H
