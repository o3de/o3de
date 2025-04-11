/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    MACRO(MatrixMxN)\
    MACRO(OBB)\
    MACRO(Plane)\
    MACRO(Quaternion)\
    MACRO(Transform)\
    MACRO(Vector2)\
    MACRO(Vector3)\
    MACRO(Vector4)\
    MACRO(VectorN)\
    MACRO(AssetId)

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
    MACRO(MatrixMxN, EXPOSE_TYPE)\
    MACRO(OBB, EXPOSE_TYPE)\
    MACRO(Plane, EXPOSE_TYPE)\
    MACRO(Quaternion, EXPOSE_TYPE)\
    MACRO(Transform, EXPOSE_TYPE)\
    MACRO(Vector2, EXPOSE_TYPE)\
    MACRO(Vector3, EXPOSE_TYPE)\
    MACRO(Vector4, EXPOSE_TYPE)\
    MACRO(VectorN, EXPOSE_TYPE)\
    MACRO(AssetId, EXPOSE_TYPE)
