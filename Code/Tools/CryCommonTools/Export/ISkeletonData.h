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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_ISKELETONDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_ISKELETONDATA_H
#pragma once


class ISkeletonData
{
public:
    enum Axis
    {
        AxisX,
        AxisY,
        AxisZ
    };
    enum Limit
    {
        LimitMin,
        LimitMax
    };

    virtual int AddBone(const void* handle, const char* name, int parentIndex) = 0;
    virtual int FindBone(const char* name) const = 0;
    virtual const void* GetBoneHandle(int boneIndex) const = 0;
    virtual int GetBoneParentIndex(int boneIndex) const = 0;
    virtual int GetBoneCount() const = 0;
    virtual void SetTranslation(int boneIndex, const float* vec) = 0;
    virtual void SetRotation(int boneIndex, const float* vec) = 0;
    virtual void SetScale(int boneIndex, const float* vec) = 0;
    virtual void SetParentFrameTranslation(int boneIndex, const float* vec) = 0;
    virtual void SetParentFrameRotation(int boneIndex, const float* vec) = 0;
    virtual void SetParentFrameScale(int boneIndex, const float* vec) = 0;
    virtual void SetPhysicalized(int boneIndex, bool physicalized) = 0;
    virtual void SetHasGeometry(int boneIndex, bool hasGeometry) = 0;
    virtual void SetBoneProperties(int boneIndex, const char* propertiesString) = 0;
    virtual void SetBoneGeomProperties(int boneIndex, const char* propertiesString) = 0;

    virtual void SetLimit(int boneIndex, Axis axis, Limit extreme, float limit) = 0;
    virtual void SetSpringTension(int boneIndex, Axis axis, float springTension) = 0;
    virtual void SetSpringAngle(int boneIndex, Axis axis, float springAngle) = 0;
    virtual void SetAxisDamping(int boneIndex, Axis axis, float damping) = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_ISKELETONDATA_H
