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

#ifndef __DEFERRED_RENDER_UTILS_H__
#define __DEFERRED_RENDER_UTILS_H__

#define SDeferMeshVert SVF_P3F_C4B_T2F

typedef std::vector<SVF_P3F_C4B_T2F> t_arrDeferredMeshVertBuff;
typedef std::vector<uint16> t_arrDeferredMeshIndBuff;

class CDeferredRenderUtils
{
public:
    static void CreateUnitFrustumMesh(int tessx, int tessy, t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
    static void CreateUnitFrustumMeshTransformed(SRenderLight* pLight, ShadowMapFrustum* pFrustum, int nAxis, int tessx, int tessy, t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
    static void CreateUnitSphere(int rec, /*SRenderLight* pLight, int depth, */ t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);

    static void CreateSimpleLightFrustumMesh(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
    static void CreateSimpleLightFrustumMeshTransformed(ShadowMapFrustum* pFrustum, int nFrustNum, t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
    static void CreateUnitBox(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);

    static void CreateQuad(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);

    CDeferredRenderUtils();
    ~CDeferredRenderUtils();
private:
    static void SphereTess(Vec3& v0, Vec3& v1, Vec3& v2, t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
    static void SphereTessR(Vec3& v0, Vec3& v1, Vec3& v2, int depth, t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
};

#endif
