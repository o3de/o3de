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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_MATHIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_MATHIMPL_H
#pragma once

#include "Serialization/IArchive.h"

#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Vector4.h"
#include "Cry_Quat.h"
#include "Cry_Matrix34.h"
#include "Cry_Geo.h"


template<typename T>
bool Serialize(Serialization::IArchive& ar, Vec2_tpl<T>& value, const char* name, const char* label)
{
    typedef T (& Array)[2];
    return ar((Array)value, name, label);
}

template<typename T>
bool Serialize(Serialization::IArchive& ar, Vec3_tpl<T>& value, const char* name, const char* label)
{
    typedef T (& Array)[3];
    return ar((Array)value, name, label);
}

template<typename T>
inline bool Serialize(Serialization::IArchive& ar, struct Vec4_tpl<T>& v, const char* name, const char* label)
{
    typedef T (& Array)[4];
    return ar((Array)v, name, label);
}


template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Quat_tpl<T>& value, const char* name, const char* label)
{
    typedef T (& Array)[4];
    return ar((Array)value, name, label);
}

template<typename T>
struct SerializableQuatT
    : QuatT_tpl<T>
{
    void Serialize(Serialization::IArchive& ar)
    {
        ar(this->q, "q", "Quaternion");
        ar(this->t, "t", "Translation");
    }
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct QuatT_tpl<T>& value, const char* name, const char* label)
{
    return Serialize(ar, static_cast<SerializableQuatT<T>&>(value), name, label);
}

struct SerializableAABB
    : AABB
{
    void Serialize(Serialization::IArchive& ar)
    {
        ar(this->min, "min", "Min");
        ar(this->max, "max", "Max");
    }
};

inline bool Serialize(Serialization::IArchive& ar, struct AABB& value, const char* name, const char* label)
{
    return Serialize(ar, static_cast<SerializableAABB&>(value), name, label);
}

template<typename T>
bool Serialize(Serialization::IArchive& ar, Matrix34_tpl<T>& value, const char* name, const char* label)
{
    typedef T (& Array)[3][4];
    return ar((Array)value, name, label);
}

//////////////////////////////////////////////////////////////////////////


namespace Serialization
{
    template<class T>
    bool Serialize(Serialization::IArchive& ar, Serialization::SRadiansAsDeg<T>& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            float degrees = RAD2DEG(*value.radians);
            float oldDegrees = degrees;
            if (!ar(degrees, name, label))
            {
                return false;
            }
            if (oldDegrees != degrees)
            {
                *value.radians = DEG2RAD(degrees);
            }
            return true;
        }
        else
        {
            return ar(*value.radians, name, label);
        }
    }

    template<class T>
    bool Serialize(Serialization::IArchive& ar, Serialization::SRadianAng3AsDeg<T>& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            Ang3 degrees(RAD2DEG(value.ang3->x), RAD2DEG(value.ang3->y), RAD2DEG(value.ang3->z));
            Ang3 oldDegrees = degrees;
            if (!ar(degrees, name, label))
            {
                return false;
            }
            if (oldDegrees != degrees)
            {
                *value.ang3 = Ang3(DEG2RAD(degrees.x), DEG2RAD(degrees.y), DEG2RAD(degrees.z));
            }
            return true;
        }
        else
        {
            return ar(*value.ang3, name, label);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

template<typename T>
bool Serialize(Serialization::IArchive& ar, Ang3_tpl<T>& value, const char* name, const char* label)
{
    typedef T (& Array)[3];
    return ar((Array)value, name, label);
}

//////////////////////////////////////////////////////////////////////////

namespace Serialization
{
    template<class T>
    bool Serialize(Serialization::IArchive& ar, Serialization::QuatAsAng3<T>& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            Ang3 ang3(*value.quat);
            Ang3 oldAng3 = ang3;
            if (!ar(Serialization::RadiansAsDeg(ang3), name, label))
            {
                return false;
            }
            if (ang3 != oldAng3)
            {
                *value.quat = Quat(ang3);
            }
            return true;
        }
        else
        {
            return ar(*value.quat, name, label);
        }
    }

    template<class T>
    bool Serialize(Serialization::IArchive& ar, Serialization::QuatTAsVec3Ang3<T>& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            if (!ar.OpenBlock(name, label))
            {
                return false;
            }

            ar(QuatAsAng3<T>((value.trans)->q), "rot", "Rotation");
            ar.Doc("Euler Angles in degrees");
            ar((value.trans)->t, "t", "Translation");
            ar.CloseBlock();
            return true;
        }
        else
        {
            return ar(*(value.trans), name, label);
        }
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_MATHIMPL_H
