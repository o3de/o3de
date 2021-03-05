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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IEXPORTSOURCE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IEXPORTSOURCE_H
#pragma once


#include "Exceptions.h"

class ISkeletonData;
class IAnimationData;
class IExportContext;
class IModelData;
class IGeometryFileData;
class IGeometryData;
class IMaterialData;
class ISkinningData;
class IMorphData;
class IGeometryMaterialData;

namespace ExportGlobal
{
    const float g_defaultFrameRate = 30.f;
};

struct SExportMetaData
{
    enum EAxisUp
    {
        X_UP,
        Y_UP,
        Z_UP
    };

    char authoring_tool[128];
    char source_data[1024];    // Filename of the source.
    char author[128];         // Name of the author.
    char revision[64];
    EAxisUp up_axis;
    float fMeterUnit;
    float fFramesPerSecond;

    SExportMetaData()
    {
        fMeterUnit = 1.0f;
        up_axis = Z_UP;
        fFramesPerSecond = ExportGlobal::g_defaultFrameRate;
        strcpy(authoring_tool, "CryENGINE Collada Exporter");
        strcpy(source_data, "");
        strcpy(author, "");
        strcpy(revision, "1.4.1");
    }
};

class IExportSource
{
public:
    virtual ~IExportSource()
    {
    }
    virtual std::string GetResourceCompilerPath() const = 0;
    virtual void GetMetaData(SExportMetaData& metaData) const = 0;
    virtual std::string GetDCCFileName() const = 0;
    virtual float GetDCCFrameRate() const{ return ExportGlobal::g_defaultFrameRate; }
    virtual std::string GetExportDirectory() const = 0;
    virtual void ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData) = 0;
    virtual bool ReadMaterials(IExportContext* context, const IGeometryFileData* geometryFileData, IMaterialData* materialData) = 0;
    virtual void ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData) = 0;
    virtual void ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* modelData, int modelIndex, ISkeletonData* skeletonData) = 0;
    virtual bool ReadSkeleton(const IGeometryFileData* geometryFileData, int geometryFileIndex, const IModelData* modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData) = 0;
    virtual int GetAnimationCount() const = 0;
    virtual std::string GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const = 0;
    virtual void GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const = 0;
    virtual void ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const = 0;
    virtual IAnimationData* ReadAnimation(IExportContext* context, const IGeometryFileData* geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const = 0;
    virtual bool ReadGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* modelData, const IMaterialData* materialData, int modelIndex) = 0;
    virtual bool ReadGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, const IModelData* modelData, const IMaterialData* materialData, int modelIndex) const = 0;
    virtual bool ReadBoneGeometry(IExportContext* context, IGeometryData* geometry, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData) = 0;
    virtual bool ReadBoneGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData) const = 0;
    virtual void ReadMorphs(IExportContext* context, IMorphData* morphData, const IModelData* modelData, int modelIndex) = 0;
    virtual bool ReadMorphGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* modelData, int modelIndex, const IMorphData* morphData, int morphIndex, const IMaterialData* materialData) = 0;
    virtual bool HasValidPosController(const IModelData* modelData, int modelIndex) const = 0;
    virtual bool HasValidRotController(const IModelData* modelData, int modelIndex) const = 0;
    virtual bool HasValidSclController(const IModelData* modelData, int modelIndex) const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IEXPORTSOURCE_H
