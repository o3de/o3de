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

#pragma once

///< these are used to ease OnDemandReflection for AZStd::types to ScriptCanvas

#define SCRIPT_CANVAS_PER_DATA_TYPE(MACRO)\
    MACRO(Boolean)\
    MACRO(CRC)\
    MACRO(EntityID)\
    MACRO(Number)\
    MACRO(String)\
    MACRO(AABB)\
    MACRO(Color)\
    MACRO(Matrix3x3)\
    MACRO(Matrix4x4)\
    MACRO(OBB)\
    MACRO(Plane)\
    MACRO(Quaternion)\
    MACRO(Transform)\
    MACRO(Vector2)\
    MACRO(Vector3)\
    MACRO(Vector4)

#define SCRIPT_CANVAS_PER_DATA_TYPE_1(MACRO, EXPOSE_TYPE)\
    MACRO(Boolean, EXPOSE_TYPE)\
    MACRO(CRC, EXPOSE_TYPE)\
    MACRO(EntityID, EXPOSE_TYPE)\
    MACRO(Number, EXPOSE_TYPE)\
    MACRO(String, EXPOSE_TYPE)\
    MACRO(AABB, EXPOSE_TYPE)\
    MACRO(Color, EXPOSE_TYPE)\
    MACRO(Matrix3x3, EXPOSE_TYPE)\
    MACRO(Matrix4x4, EXPOSE_TYPE)\
    MACRO(OBB, EXPOSE_TYPE)\
    MACRO(Plane, EXPOSE_TYPE)\
    MACRO(Quaternion, EXPOSE_TYPE)\
    MACRO(Transform, EXPOSE_TYPE)\
    MACRO(Vector2, EXPOSE_TYPE)\
    MACRO(Vector3, EXPOSE_TYPE)\
    MACRO(Vector4, EXPOSE_TYPE)
