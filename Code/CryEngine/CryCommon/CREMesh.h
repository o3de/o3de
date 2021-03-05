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

#ifndef __CREMESH_H__
#define __CREMESH_H__

class CREMesh
    : public CRendElementBase
{
public:

    struct CRenderChunk* m_pChunk;
    class CRenderMesh* m_pRenderMesh;

    // Copy of Chunk to avoid indirections
    int32 m_nFirstIndexId;
    int32 m_nNumIndices;

    uint32 m_nFirstVertId;
    uint32 m_nNumVerts;

protected:
    CREMesh()
    {
        mfSetType(eDATA_Mesh);
        mfUpdateFlags(FCEF_TRANSFORM);

        m_pChunk = NULL;
        m_pRenderMesh = NULL;
        m_nFirstIndexId = -1;
        m_nNumIndices = -1;
        m_nFirstVertId = 0;
        m_nNumVerts = 0;
    }

    virtual ~CREMesh()
    {
    }

    // Ideally should be declared and left unimplemented to prevent slicing at compile time
    // but this would prevent auto code gen in renderer later on.
    // To track potential slicing, uncomment the following (and their equivalent in CREMeshImpl)
    //CREMesh(CREMesh&);
    //CREMesh& operator=(CREMesh& rhs);
};

#endif // __CREMESH_H__
