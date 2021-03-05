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

// Description : Export geometry to FBX file format


#ifndef CRYINCLUDE_FBXPLUGIN_FBXEXPORTER_H
#define CRYINCLUDE_FBXPLUGIN_FBXEXPORTER_H

#pragma once
#include <Include/IExportManager.h>
#include "fbxsdk.h"

struct SFBXSettings
{
    bool bCopyTextures;
    bool bEmbedded;
    bool bAsciiFormat;
    bool bConvertAxesForMaxMaya;
};

class CFBXExporter
    : public IExporter
{
public:
    CFBXExporter();

    virtual const char* GetExtension() const;
    virtual const char* GetShortDescription() const;
    virtual bool ExportToFile(const char* filename, const Export::IData* pData);
    virtual bool ImportFromFile(const char* filename, Export::IData* pData);
    virtual void Release();

private:
    // recursively traverse all nodes under pNode and reset prerotations for camera nodes by calling convertCameraForMaxMaya on them
    void convertCamerasForMaxMaya(FbxNode* node) const;
    // reset prerotations for a specified nodes if it's a camera
    void convertCameraForMaxMaya(FbxNode* pNode) const;

    FbxMesh* CreateFBXMesh(const Export::Object* pObj);
    FbxFileTexture* CreateFBXTexture(const char* pTypeName, const char* pName);
    FbxSurfaceMaterial* CreateFBXMaterial(const std::string& name, const Export::Mesh* pMesh);
    FbxNode* CreateFBXNode(const Export::Object* pObj);
    FbxNode* CreateFBXAnimNode(FbxScene* pScene, FbxAnimLayer* pCameraAnimBaseLayer, const Export::Object* pObj);
    void FillAnimationData(Export::Object* pObject, FbxNode* pNode, FbxAnimLayer* pAnimLayer, FbxAnimCurve* pCurve, Export::AnimParamType paramType);

    FbxManager* m_pFBXManager;
    SFBXSettings m_settings;
    FbxScene* m_pFBXScene;
    std::string m_path;
    std::vector<FbxNode*> m_nodes;
    std::map<std::string, FbxSurfaceMaterial*> m_materials;
    std::map<const Export::Mesh*, int> m_meshMaterialIndices;
};
#endif // CRYINCLUDE_FBXPLUGIN_FBXEXPORTER_H
