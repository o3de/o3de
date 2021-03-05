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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IGEOMETRYDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IGEOMETRYDATA_H
#pragma once


class IGeometryData
{
public:
    virtual int AddPosition(float x, float y, float z) = 0;
    virtual int AddNormal(float x, float y, float z) = 0;
    virtual int AddTextureCoordinate(float u, float v) = 0;
    virtual int AddVertexColor(float r, float g, float b, float a) = 0;
    virtual int AddPolygon(const int* indices, int mtlID) = 0;

    virtual int GetNumberOfPositions() const = 0;
    virtual int GetNumberOfNormals() const = 0;
    virtual int GetNumberOfTextureCoordinates() const = 0;
    virtual int GetNumberOfVertexColors() const = 0;
    virtual int GetNumberOfPolygons() const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IGEOMETRYDATA_H
