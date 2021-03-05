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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SINGLEANIMATIONEXPORTSOURCEADAPTER_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SINGLEANIMATIONEXPORTSOURCEADAPTER_H
#pragma once


#include "ExportSourceDecoratorBase.h"

class SingleAnimationExportSourceAdapter
    : public ExportSourceDecoratorBase
{
public:
    SingleAnimationExportSourceAdapter(IExportSource* source, IGeometryFileData* geometryData, int geometryFileIndex, int animationIndex);

    virtual float GetDCCFrameRate() const;

    virtual void ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData);
    virtual void ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData);
    virtual void ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* modelData, int modelIndex, ISkeletonData* skeletonData);
    virtual bool ReadSkeleton(const IGeometryFileData* geometryFileData, int geometryFileIndex, const IModelData* modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData);
    virtual int GetAnimationCount() const;
    virtual std::string GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const;
    virtual void GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const;
    virtual void ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const;
    virtual IAnimationData* ReadAnimation(IExportContext* context, const IGeometryFileData* geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const;

private:
    int animationIndex;
    IGeometryFileData* geometryFileData;
    int geometryFileIndex;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SINGLEANIMATIONEXPORTSOURCEADAPTER_H
