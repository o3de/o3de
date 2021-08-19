// ----------------------------------------------------------------------------
// Signed distance fields are generated from triangle meshes.
// This is the interface to a triangle mesh for use by TressFXSDFCollision 
// to generate the SDF.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "Math/Vector3D.h"
#include "EngineInterface.h"

class TressFXSDFInputMeshInterface
{
public:
    virtual ~TressFXSDFInputMeshInterface() {}

    // Returns the GPU buffer containing the mesh data. It will be an input to SDF generator.
    virtual EI_Resource& GetMeshBuffer() = 0;

    // Return the GPU index buffer of the mesh.
    virtual EI_Resource& GetTrimeshVertexIndicesBuffer() = 0;

    // Returns the number of vertices of the mesh
    virtual int GetNumMeshVertices() = 0;

    // Returns the number of triangles of the mesh
    virtual int GetNumMeshTriangle() = 0;

    // Returns the byte size of mesh buffer data element.
    virtual size_t GetSizeOfMeshElement() = 0;

    // Returns the bounding box of the current mesh.
    virtual void GetBoundingBox(Vector3& min, Vector3& max) = 0;

    virtual void GetInitialBoundingBox(Vector3& min, Vector3& max) = 0;
};