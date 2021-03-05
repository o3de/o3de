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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SKELETONDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SKELETONDATA_H
#pragma once


#include "ISkeletonData.h"
#include <string>
#include <vector>
#include <map>

class SkeletonData
    : public ISkeletonData
{
public:
    // ISkeletonData
    virtual int AddBone(const void* handle, const char* name, int parentIndex);
    virtual int FindBone(const char* name) const;
    virtual const void* GetBoneHandle(int boneIndex) const;
    virtual int GetBoneParentIndex(int boneIndex) const;
    virtual int GetBoneCount() const;
    virtual void SetTranslation(int boneIndex, const float* vec);
    virtual void SetRotation(int boneIndex, const float* vec);
    virtual void SetScale(int boneIndex, const float* vec);
    virtual void SetParentFrameTranslation(int boneIndex, const float* vec);
    virtual void SetParentFrameRotation(int boneIndex, const float* vec);
    virtual void SetParentFrameScale(int boneIndex, const float* vec);
    virtual void SetLimit(int boneIndex, Axis axis, Limit extreme, float limit);
    virtual void SetSpringTension(int boneIndex, Axis axis, float springTension);
    virtual void SetSpringAngle(int boneIndex, Axis axis, float springAngle);
    virtual void SetAxisDamping(int boneIndex, Axis axis, float damping);
    virtual void SetPhysicalized(int boneIndex, bool physicalized);
    virtual void SetHasGeometry(int boneIndex, bool hasGeometry);
    virtual void SetBoneProperties(int boneIndex, const char* propertiesString);
    virtual void SetBoneGeomProperties(int boneIndex, const char* propertiesString);

    bool HasParentFrame(int boneIndex) const;
    void GetParentFrameTranslation(int boneIndex, float* vec) const;
    void GetParentFrameRotation(int boneIndex, float* vec) const;
    void GetParentFrameScale(int boneIndex, float* vec) const;
    bool HasLimit(int boneIndex, Axis axis, Limit extreme) const;
    float GetLimit(int boneIndex, Axis axis, Limit extreme) const;
    bool HasSpringTension(int boneIndex, Axis axis) const;
    float GetSpringTension(int boneIndex, Axis axis) const;
    bool HasSpringAngle(int boneIndex, Axis axis) const;
    float GetSpringAngle(int boneIndex, Axis axis) const;
    bool HasAxisDamping(int boneIndex, Axis axis) const;
    float GetAxisDamping(int boneIndex, Axis axis) const;
    bool GetPhysicalized(int boneIndex) const;
    bool HasGeometry(int boneIndex) const;

    int GetRootCount() const;
    int GetRootIndex(int rootIndex) const;
    int GetParentIndex(int boneIndex) const;
    const std::string GetName(int boneIndex) const;
    const std::string GetSafeName(int boneIndex) const;
    int GetChildCount(int boneIndex) const;
    int GetChildIndex(int boneIndex, int childIndexIndex) const;
    void GetTranslation(float* vec, int boneIndex) const;
    void GetRotation(float* vec, int boneIndex) const;
    void GetScale(float* vec, int boneIndex) const;
    const std::string GetBoneProperties(int boneIndex) const;
    const std::string GetBoneGeomProperties(int boneIndex) const;

private:
    void EnsureParentFrameExists(int boneIndex);

    typedef std::pair<Axis, Limit> AxisLimit;
    typedef std::map<AxisLimit, float> AxisLimitLimitMap;
    typedef std::map<Axis, float> AxisSpringTensionMap;
    typedef std::map<Axis, float> AxisSpringAngleMap;
    typedef std::map<Axis, float> AxisDampingMap;

    struct BoneEntry
    {
    public:
        BoneEntry(const void* handle, const std::string& name, int parentIndex);
        const void* handle;
        std::string name;
        int parentIndex;
        AxisLimitLimitMap limits;
        AxisSpringTensionMap springTensions;
        AxisSpringAngleMap springAngles;
        AxisDampingMap dampings;
        bool hasParentFrame;
        float parentFrameTranslation[3];
        float parentFrameRotation[3];
        float parentFrameScale[3];
        bool physicalized;
        std::vector<int> children;
        float translation[3];
        float rotation[3];
        float scale[3];
        bool hasGeometry;
        std::string propertiesString;
        std::string geomPropertiesString;
    };

    std::vector<BoneEntry> m_bones;
    std::vector<int> m_roots;
    std::map<std::string, int> m_nameBoneIndexMap;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SKELETONDATA_H
