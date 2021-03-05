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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYDATA_H
#pragma once


#include "IGeometryData.h"
#include <vector>

class GeometryData
    : public IGeometryData
{
public:
    GeometryData();

    // IGeometryData
    virtual int AddPosition(float x, float y, float z);
    virtual int AddNormal(float x, float y, float z);
    virtual int AddTextureCoordinate(float u, float v);
    virtual int AddVertexColor(float r, float g, float b, float a);

    virtual int AddPolygon(const int* indices, int mtlID);

    virtual int GetNumberOfPositions() const;
    virtual int GetNumberOfNormals() const;
    virtual int GetNumberOfTextureCoordinates() const;
    virtual int GetNumberOfVertexColors() const;

    virtual int GetNumberOfPolygons() const;

    struct Vector
    {
        Vector(float x, float y, float z)
            : x(x)
            , y(y)
            , z(z) {}
        float x, y, z;
    };

    struct TextureCoordinate
    {
        TextureCoordinate(float u, float v)
            : u(u)
            , v(v) {}
        float u, v;
    };

    struct VertexColor
    {
        VertexColor(float r, float g, float b, float a)
            : r(r)
            , g(g)
            , b(b)
            , a(a) {}
        float r, g, b, a;
    };

    struct Polygon
    {
        struct Vertex
        {
            Vertex() {}
            Vertex(int positionIndex, int normalIndex, int textureCoordinateIndex, int vertexColorIndex)
                : positionIndex(positionIndex)
                , normalIndex(normalIndex)
                , textureCoordinateIndex(textureCoordinateIndex)
                , vertexColorIndex(vertexColorIndex) {}
            int positionIndex, normalIndex, textureCoordinateIndex, vertexColorIndex;
        };

        Polygon(int mtlID, const Vertex& v0, const Vertex& v1, const Vertex& v2)
            : mtlID(mtlID)
        {
            v[0] = v0;
            v[1] = v1;
            v[2] = v2;
        }

        int mtlID;
        Vertex v[3];
    };

    std::vector<Vector> positions;
    std::vector<Vector> normals;
    std::vector<TextureCoordinate> textureCoordinates;
    std::vector<VertexColor> vertexColors;
    std::vector<Polygon> polygons;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYDATA_H
