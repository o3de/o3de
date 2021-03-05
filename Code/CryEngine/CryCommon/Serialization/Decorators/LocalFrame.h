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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_LOCALFRAME_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_LOCALFRAME_H
#pragma once

#include <Cry_Math.h>
#include "Serialization/Math.h"

namespace Serialization
{
    class IArchive;

    struct LocalPosition
    {
        Vec3* value;
        int space;
        const char* parentName;
        const void* handle;

        LocalPosition(Vec3& _vec, int _space, const char* _parentName, const void* _handle)
            : value(&_vec)
            , space(_space)
            , parentName(_parentName)
            , handle(_handle)
        {
        }

        void Serialize(IArchive& ar);
    };

    struct LocalOrientation
    {
        Quat* value;
        int space;
        const char* parentName;
        const void* handle;

        LocalOrientation(Quat& _vec, int _space, const char* _parentName, const void* _handle)
            : value(&_vec)
            , space(_space)
            , parentName(_parentName)
            , handle(_handle)
        {
        }

        void Serialize(IArchive& ar);
    };

    struct LocalFrame
    {
        Quat* rotation;
        Vec3* position;
        const char* parentName;
        int rotationSpace;
        int positionSpace;
        const void* handle;

        LocalFrame(Quat* _rotation, int _rotationSpace, Vec3* _position, int _positionSpace, const char* _parentName, const void* _handle)
            : rotation(_rotation)
            , position(_position)
            , parentName(_parentName)
            , rotationSpace(_rotationSpace)
            , positionSpace(_positionSpace)
            , handle(_handle)
        {
        }

        void Serialize(IArchive& ar);
    };

    enum
    {
        SPACE_JOINT,
        SPACE_ENTITY,
        SPACE_JOINT_WITH_PARENT_ROTATION,
        SPACE_JOINT_WITH_CHARACTER_ROTATION,
        SPACE_SOCKET_RELATIVE_TO_JOINT,
        SPACE_SOCKET_RELATIVE_TO_BINDPOSE
    };



    //position
    inline LocalPosition LocalToEntity(Vec3& position, const void* handle = 0)
    {
        return LocalPosition(position, SPACE_ENTITY, "", handle ? handle : &position);
    }
    inline LocalPosition LocalToJoint(Vec3& position, const string& jointName, const void* handle = 0)
    {
        return LocalPosition(position, SPACE_JOINT, jointName.c_str(), handle ? handle : &position);
    }

    inline LocalPosition LocalToJointCharacterRotation(Vec3& position, const string& jointName, const void* handle = 0)
    {
        return LocalPosition(position, SPACE_JOINT_WITH_CHARACTER_ROTATION, jointName.c_str(), handle ? handle : &position);
    }

    bool Serialize(Serialization::IArchive& ar, Serialization::LocalPosition& value, const char* name, const char* label);
    bool Serialize(Serialization::IArchive& ar, Serialization::LocalOrientation& value, const char* name, const char* label);
    bool Serialize(Serialization::IArchive& ar, Serialization::LocalFrame& value, const char* name, const char* label);
}

#include "LocalFrameImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_LOCALFRAME_H
