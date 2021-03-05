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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYEXPORTSOURCEADAPTER_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYEXPORTSOURCEADAPTER_H
#pragma once


#include "ExportSourceDecoratorBase.h"

class GeometryExportSourceAdapter
    : public ExportSourceDecoratorBase
{
public:
    GeometryExportSourceAdapter(IExportSource* source, IGeometryFileData* geometryFileData, const std::vector<int>& geometryFileIndices);

    virtual void ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData);
    virtual void ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData);
    virtual bool ReadSkeleton(const IGeometryFileData* geometryFileData, int geometryFileIndex, const IModelData* modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData);

private:
    IGeometryFileData* m_geometryFileData;
    std::vector<int> m_geometryFileIndices;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYEXPORTSOURCEADAPTER_H
