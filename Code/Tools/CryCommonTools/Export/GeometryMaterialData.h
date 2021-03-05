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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYMATERIALDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYMATERIALDATA_H
#pragma once


#include "IGeometryMaterialData.h"

class GeometryMaterialData
    : public IGeometryMaterialData
{
public:
    // IGeometryMaterialData
    virtual void AddUsedMaterialIndex(int materialIndex);
    virtual int GetUsedMaterialCount() const;
    virtual int GetUsedMaterialIndex(int usedMaterialIndex) const;

private:
    std::vector<int> m_usedMaterialIndices;
    std::map<int, int> m_usedMaterialIndexIndexMap;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYMATERIALDATA_H
