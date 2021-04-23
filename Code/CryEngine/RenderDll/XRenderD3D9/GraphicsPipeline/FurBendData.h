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
#pragma once

//#include "Common/Renderer.h"

class FurBendData
{
public:
    static FurBendData& Get();

    void SetupObject(CRenderObject& renderObject, const SRenderingPassInfo& passInfo);
    void GetPrevObjToWorldMat(CRenderObject& renderObject, Matrix44A& res);
    void InsertNewElements();
    void FreeData();

    void OnBeginFrame();

private:
    struct ObjectParameters
    {
        ObjectParameters()
            : m_pRenderObject(nullptr)
        {
        }

        ObjectParameters(CRenderObject& renderObject, const Matrix34A& worldMatrix, AZ::u32 updateFrameId)
            : m_pRenderObject(&renderObject)
            , m_updateFrameId(updateFrameId)
            , m_worldMatrix(worldMatrix)
        {
        }

        CRenderObject* m_pRenderObject;
        AZ::u32 m_updateFrameId;
        Matrix34 m_worldMatrix;
        //uint32 m_numBones;
        //DualQuat* m_pBoneQuatsS;
    };

    typedef VectorMap<uintptr_t, ObjectParameters > ObjectMap;
    ObjectMap m_objects;
    CThreadSafeRendererContainer<ObjectMap::value_type> m_fillData[RT_COMMAND_BUF_COUNT];

    static FurBendData* s_pInstance;
};
