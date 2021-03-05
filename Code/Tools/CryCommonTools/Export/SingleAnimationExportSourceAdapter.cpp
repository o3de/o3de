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

#include "StdAfx.h"
#include "SingleAnimationExportSourceAdapter.h"
#include "IGeometryFileData.h"
#include <cassert>

SingleAnimationExportSourceAdapter::SingleAnimationExportSourceAdapter(IExportSource* source, IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex)
    : ExportSourceDecoratorBase(source)
    , animationIndex(animationIndex)
    , geometryFileData(geometryFileData)
    , geometryFileIndex(geometryFileIndex)
{
    assert(this->animationIndex < this->source->GetAnimationCount());
}

float SingleAnimationExportSourceAdapter::GetDCCFrameRate() const
{
    return this->source->GetDCCFrameRate();
}

void SingleAnimationExportSourceAdapter::ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData)
{
    const int geometryFileIndex = geometryFileData->AddGeometryFile(
            this->geometryFileData->GetGeometryFileHandle(this->geometryFileIndex),
            this->geometryFileData->GetGeometryFileName(this->geometryFileIndex),
            this->geometryFileData->GetProperties(this->geometryFileIndex));
}

void SingleAnimationExportSourceAdapter::ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData)
{
    assert(geometryFileIndex == 0);
    this->source->ReadModels(this->geometryFileData, this->geometryFileIndex, modelData);
}

void SingleAnimationExportSourceAdapter::ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* const modelData, int modelIndex, ISkeletonData* skeletonData)
{
    this->source->ReadSkinning(context, skinningData, modelData, modelIndex, skeletonData);
}

bool SingleAnimationExportSourceAdapter::ReadSkeleton(const IGeometryFileData* const geometryFileData, int geometryFileIndex, const IModelData* const modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData)
{
    assert(geometryFileIndex == 0);
    return this->source->ReadSkeleton(this->geometryFileData, this->geometryFileIndex, modelData, modelIndex, materialData, skeletonData);
}

int SingleAnimationExportSourceAdapter::GetAnimationCount() const
{
    return 1;
}

std::string SingleAnimationExportSourceAdapter::GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const
{
    assert(geometryFileIndex == 0);
    assert(animationIndex == 0);
    return this->source->GetAnimationName(this->geometryFileData, this->geometryFileIndex, this->animationIndex);
}

void SingleAnimationExportSourceAdapter::GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const
{
    assert(animationIndex == 0);
    this->source->GetAnimationTimeSpan(start, stop, this->animationIndex);
}

void SingleAnimationExportSourceAdapter::ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const
{
    assert(animationIndex == 0);
    this->source->ReadAnimationFlags(context, animationData, geometryFileData, modelData, modelIndex, skeletonData, this->animationIndex);
}

IAnimationData* SingleAnimationExportSourceAdapter::ReadAnimation(IExportContext* context, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const
{
    assert(animationIndex == 0);
    return this->source->ReadAnimation(context, geometryFileData, modelData, modelIndex, skeletonData, this->animationIndex, fps);
}
