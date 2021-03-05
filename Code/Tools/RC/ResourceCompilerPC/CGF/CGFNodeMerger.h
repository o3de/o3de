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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_CGFNODEMERGER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_CGFNODEMERGER_H
#pragma once


class CContentCGF;
class CMesh;
struct CMaterialCGF;
struct CNodeCGF;

namespace CGFNodeMerger
{
    bool SetupMeshSubsets(CContentCGF* pCGF, CMesh& mesh, CMaterialCGF* pMaterialCGF, string& errorMessage);
    bool MergeNodes(CContentCGF* pCGF, std::vector<CNodeCGF*> nodes, string& errorMessage, CMesh* pOutMesh);
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_CGFNODEMERGER_H
