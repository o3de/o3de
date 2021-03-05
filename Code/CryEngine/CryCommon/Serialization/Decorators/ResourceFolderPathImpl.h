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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCEFOLDERPATHIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCEFOLDERPATHIMPL_H
#pragma once

#include "Serialization/IArchive.h"

namespace Serialization
{
    inline bool Serialize(Serialization::IArchive& ar, Serialization::ResourceFolderPath& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct::ForEdit(value), name, label);
        }
        else
        {
            return ar(*value.m_path, name, label);
        }
    }
}
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCEFOLDERPATHIMPL_H
