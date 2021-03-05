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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_LOCALFRAMEIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_LOCALFRAMEIMPL_H
#pragma once

#include "LocalFrame.h"
#include "Serialization/IArchive.h"
#include "Serialization/MathImpl.h"

namespace Serialization
{
    inline void LocalPosition::Serialize(Serialization::IArchive& ar)
    {
        ar(value->x, "x", "^");
        ar(value->y, "y", "^");
        ar(value->z, "z", "^");
    }

    inline void LocalOrientation::Serialize(Serialization::IArchive& ar)
    {
        ar(Serialization::AsAng3(*value), "q", "^");
    }

    inline void LocalFrame::Serialize(Serialization::IArchive& ar)
    {
        ar(*position, "t", "<T");
        ar(AsAng3(*rotation), "q", "<R");
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::LocalPosition& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct(value), name, label);
        }
        else
        {
            return ar(*value.value, name, label);
        }
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::LocalOrientation& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct(value), name, label);
        }
        else
        {
            return ar(*value.value, name, label);
        }
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::LocalFrame& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct(value), name, label);
        }
        else
        {
            QuatT t(*value.rotation, *value.position);
            if (!ar(t, name, label))
            {
                return false;
            }
            if (ar.IsInput())
            {
                *value.position = t.t;
                *value.rotation = t.q;
            }
            return true;
        }
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_LOCALFRAMEIMPL_H
