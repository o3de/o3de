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

#include "FBXPlugin_precompiled.h"


// FBX SDK Help: http://download.autodesk.com/global/docs/fbxsdk2012/en_us/index.html

#include "FBXExporter.h"
#include "FBXSettingsDlg.h"

#include "IEditor.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants used to convert cameras to and from Maya

static const float      s_CONVERSION_BAKING_SAMPLE_RATE = 30.0f;

// IMPORT_PRE/POST_ROTATIONS determined by experimentation with fbx2015 in Maya and Max.
// We are transforming a Z-axis forward (Max/Maya) to a negative Y-axis forward camera (Editor).
// Analytically this shold involve a preRotation of -90 degrees around the x-axis.
// For Y-Up scenes, our Open 3D Engine Tools inserts an additional 180 rotation around the Y-axis when orienting .cgf files. We need to
// 'undo' for FBX anim curves. This does not apply to cameras which do not use .cgf files
static const FbxVector4 s_PRE_ROTATION_FOR_YUP_SCENES (-90.0f, .0f, .0f);
static const FbxVector4 s_POST_ROTATION_FOR_YUP_OBJECTS(-90.0f, 180.0f, .0f);
static const FbxVector4 s_POST_ROTATION_FOR_ZFORWARD_CAMERAS(-90.0f, .0f, .0f);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
    std::string GetFilePath(const std::string& filename)
    {
        int pos = filename.find_last_of("/\\");
        if (pos > 0)
        {
            return filename.substr(0, pos + 1);
        }

        return filename;
    }


    std::string GetFileName(const std::string& fullFilename)
    {
        int pos = fullFilename.find_last_of("/\\");
        if (pos > 0)
        {
            return fullFilename.substr(pos + 1);
        }

        return fullFilename;
    }
} // namespace


#ifdef DEBUG
///////////////////////////////////////////////////////////////////////
// debugging function to print all the keys in an Fbx Curve
void DebugPrintCurveKeys(FbxAnimCurve* pCurve, const QString& name)
{
    if (pCurve)
    {
        ILog* log = GetIEditor()->GetSystem()->GetILog();
        log->Log("\n%s", name);
        for (int keyID = 0; keyID < pCurve->KeyGetCount(); ++keyID)
        {
            FbxAnimCurveKey key = pCurve->KeyGet(keyID);

            log->Log("%.2g:%g, ", key.GetTime().GetSecondDouble(), key.GetValue());
        }
    }
}
#endif

// CFBXExporter
CFBXExporter::CFBXExporter()
    : m_pFBXManager(0)
    , m_pFBXScene(0)
{
    m_settings.bCopyTextures = true;
    m_settings.bEmbedded = false;
    m_settings.bAsciiFormat = false;
    m_settings.bConvertAxesForMaxMaya = false;
}


const char* CFBXExporter::GetExtension() const
{
    return "fbx";
}


const char* CFBXExporter::GetShortDescription() const
{
    return "FBX format";
}


bool CFBXExporter::ExportToFile(const char* filename, const Export::IData* pData)
{
    if (!m_pFBXManager)
    {
        m_pFBXManager = FbxManager::Create();
    }

    bool bAnimationExport = false;

    for (int objectID = 0; objectID < pData->GetObjectCount(); ++objectID)
    {
        Export::Object* pObject = pData->GetObject(objectID);

        if (pObject->GetEntityAnimationDataCount() > 0)
        {
            bAnimationExport = true;
            break;
        }
    }

    // do nothing if the user cancels
    if (!OpenFBXSettingsDlg(m_settings))
    {
        return false;
    }

    m_path = GetFilePath(filename);
    std::string name = GetFileName(filename);

    // create an IOSettings object
    FbxIOSettings* pSettings = FbxIOSettings::Create(m_pFBXManager, IOSROOT);
    m_pFBXManager->SetIOSettings(pSettings);

    // Create a scene object
    FbxScene* pFBXScene = FbxScene::Create(m_pFBXManager, "Test");
    pFBXScene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::Max);
    pFBXScene->GetGlobalSettings().SetOriginalUpAxis(FbxAxisSystem::Max);

    // Create document info
    FbxDocumentInfo* pDocInfo = FbxDocumentInfo::Create(m_pFBXManager, "DocInfo");
    pDocInfo->mTitle = name.c_str();
    pDocInfo->mSubject = "Exported geometry from editor application.";
    pDocInfo->mAuthor = "Editor FBX exporter.";
    pDocInfo->mRevision = "rev. 1.0";
    pDocInfo->mKeywords = "Editor FBX export";
    pDocInfo->mComment = "";
    pFBXScene->SetDocumentInfo(pDocInfo);

    // Create nodes from objects and add them to the scene
    FbxNode* pRootNode = pFBXScene->GetRootNode();

    FbxAnimStack* pAnimStack = FbxAnimStack::Create(pFBXScene, "AnimStack");
    FbxAnimLayer* pAnimBaseLayer = FbxAnimLayer::Create(pFBXScene, "AnimBaseLayer");
    pAnimStack->AddMember(pAnimBaseLayer);

    int numObjects = pData->GetObjectCount();

    m_nodes.resize(0);
    m_nodes.reserve(numObjects);

    for (int i = 0; i < numObjects; ++i)
    {
        Export::Object* pObj = pData->GetObject(i);
        FbxNode* newNode = NULL;

        if (!bAnimationExport)
        {
            newNode = CreateFBXNode(pObj);
        }
        else
        {
            newNode = CreateFBXAnimNode(pFBXScene, pAnimBaseLayer, pObj);
        }

        m_nodes.push_back(newNode);
    }

    // solve hierarchy
    for (int i = 0; i < m_nodes.size() && i < numObjects; ++i)
    {
        const Export::Object* pObj = pData->GetObject(i);
        FbxNode* pNode = m_nodes[i];
        FbxNode* pParentNode = 0;
        if (pObj->nParent >= 0 && pObj->nParent < m_nodes.size())
        {
            pParentNode = m_nodes[pObj->nParent];
        }
        if (pParentNode)
        {
            pParentNode->AddChild(pNode);
        }
        else
        {
            pRootNode->AddChild(pNode);
        }
    }

    int nFileFormat = -1;

    if (m_settings.bAsciiFormat)
    {
        if (nFileFormat < 0 || nFileFormat >= m_pFBXManager->GetIOPluginRegistry()->GetWriterFormatCount())
        {
            nFileFormat = m_pFBXManager->GetIOPluginRegistry()->GetNativeWriterFormat();
            int lFormatIndex, lFormatCount = m_pFBXManager->GetIOPluginRegistry()->GetWriterFormatCount();
            for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
            {
                if (m_pFBXManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
                {
                    FbxString lDesc = m_pFBXManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
                    const char* lASCII = "ascii";
                    if (lDesc.Find(lASCII) >= 0)
                    {
                        nFileFormat = lFormatIndex;
                        break;
                    }
                }
            }
        }
    }

    pSettings->SetBoolProp(EXP_FBX_EMBEDDED, m_settings.bEmbedded);

    if (m_settings.bConvertAxesForMaxMaya)
    {
        // convert the scene from our Z-Up world to Maya's Y-Up World. This is stored in the FBX scene and both Max & Maya will
        // import accordingly
        FbxAxisSystem::MayaYUp.ConvertScene(pFBXScene);

        // process all camera nodes in the scene to make them look down their negative Z-axis
        convertCamerasForMaxMaya(pFBXScene->GetRootNode());
    }

    //Save a scene
    bool bRet = false;
    FbxExporter* pFBXExporter = FbxExporter::Create(m_pFBXManager, name.c_str());

    // For backward compatilibity choose a widely compatible FBX version that's been out for a while:
    if (pFBXExporter)
    {
        pFBXExporter->SetFileExportVersion(FBX_2014_00_COMPATIBLE);

        if (pFBXExporter->Initialize(filename, nFileFormat, pSettings))
        {
            bRet = pFBXExporter->Export(pFBXScene);
        }
        pFBXExporter->Destroy();
    }

    pFBXScene->Destroy();

    m_materials.clear();
    m_meshMaterialIndices.clear();

    m_pFBXManager->Destroy();
    m_pFBXManager = 0;

    if (bRet)
    {
        GetIEditor()->GetSystem()->GetILog()->Log("\nFBX Exporter: Exported successfully to %s.", name.c_str());
    }
    else
    {
        GetIEditor()->GetSystem()->GetILog()->LogError("\nFBX Exporter: Failed.");
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method finds all cameras (via recursion) in the node tree rooted at pNode and bakes the necessary
// transforms into any cameras it finds to take care of the conversion from Maya Z-forward to our negative-y forward cameras
void CFBXExporter::convertCamerasForMaxMaya(FbxNode* pNode) const
{
    if (pNode)
    {
        // recurse to children to convert all cameras in the level
        for (int i = 0; i < pNode->GetChildCount(); i++)
        {
            convertCamerasForMaxMaya(pNode->GetChild(i));
        }

        // process this node if it's a camera
        convertCameraForMaxMaya(pNode);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Maya cameras look down the negative z-axis. Ours look down the positive-y. This method bakes the necessary transforms
// into the local transforms of the given pNode if it's a camera to take care of the conversion
void CFBXExporter::convertCameraForMaxMaya(FbxNode* pNode) const
{
    // Maya puts in an extra 90 x-rotation in the "Rotate Axis" channel. Adding an extra post-rotation fixes this,
    // determined experimentally
    static const FbxVector4 s_EXTRA_EXPORT_POST_ROTATION_FOR_MAX_MAYA(.0f, -90.0f, .0f);

    if (pNode && pNode->GetNodeAttribute() && pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eCamera)
    {
        // bake in s_PRE/POST_ROTATION_FOR_MAYA into anim curves
        pNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
        pNode->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);

        pNode->SetPreRotation(FbxNode::eSourcePivot, s_PRE_ROTATION_FOR_YUP_SCENES);
        pNode->SetPostRotation(FbxNode::eSourcePivot, s_POST_ROTATION_FOR_ZFORWARD_CAMERAS);

        pNode->ResetPivotSetAndConvertAnimation(s_CONVERSION_BAKING_SAMPLE_RATE, true);   // bake postRotation in for all animStacks

        // Maya puts in an extra 90 x-rotation in the "Rotate Axis" channel. Adding an extra post-rotation after the baking of transforms
        // fixes this - i.e. baked transformes change the camera's local rotation channels themselves, post-bake post-transforms get
        // stuffed into Maya's 'Rotate Axis' channels
        pNode->SetPostRotation(FbxNode::eSourcePivot, s_EXTRA_EXPORT_POST_ROTATION_FOR_MAX_MAYA);
    }
}

FbxMesh* CFBXExporter::CreateFBXMesh(const Export::Object* pObj)
{
    if (!m_pFBXManager)
    {
        return 0;
    }

    int numVertices = pObj->GetVertexCount();
    const Export::Vector3D* pVerts = pObj->GetVertexBuffer();

    int numMeshes = pObj->GetMeshCount();

    int numAllFaces = 0;
    for (int j = 0; j < numMeshes; ++j)
    {
        const Export::Mesh* pMesh = pObj->GetMesh(j);
        int numFaces = pMesh->GetFaceCount();
        numAllFaces += numFaces;
    }

    if (numVertices == 0 || numAllFaces == 0)
    {
        return 0;
    }

    FbxMesh* pFBXMesh = FbxMesh::Create(m_pFBXManager, pObj->name);

    pFBXMesh->InitControlPoints(numVertices);
    FbxVector4* pControlPoints = pFBXMesh->GetControlPoints();

    for (int i = 0; i < numVertices; ++i)
    {
        const Export::Vector3D& vertex = pVerts[i];
        pControlPoints[i] = FbxVector4(vertex.x, vertex.y, vertex.z);
    }

    /*
    // We want to have one normal for each vertex (or control point),
    // so we set the mapping mode to eBY_CONTROL_POINT.
    FbxGeometryElementNormal* lElementNormal= pFBXMesh->CreateElementNormal();

    //FbxVector4 lNormalZPos(0, 0, 1);

    lElementNormal->SetMappingMode(FbxGeometryElement::eBY_CONTROL_POINT);

    // Set the normal values for every control point.
    lElementNormal->SetReferenceMode(FbxGeometryElement::eDIRECT);

    lElementNormal->GetDirectArray().Add(lNormalZPos);
    */


    int numTexCoords = pObj->GetTexCoordCount();
    const Export::UV* pTextCoords = pObj->GetTexCoordBuffer();
    if (numTexCoords)
    {
        // Create UV for Diffuse channel.
        FbxGeometryElementUV* pFBXDiffuseUV = pFBXMesh->CreateElementUV("DiffuseUV");
        FBX_ASSERT(pFBXDiffuseUV != NULL);

        pFBXDiffuseUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
        pFBXDiffuseUV->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

        for (int i = 0; i < numTexCoords; ++i)
        {
            const Export::UV& textCoord = pTextCoords[i];
            pFBXDiffuseUV->GetDirectArray().Add(FbxVector2(textCoord.u, textCoord.v));
        }

        //Now we have set the UVs as eINDEX_TO_DIRECT reference and in eBY_POLYGON_VERTEX  mapping mode
        //we must update the size of the index array.
        pFBXDiffuseUV->GetIndexArray().SetCount(numTexCoords);
    }

    pFBXMesh->ReservePolygonCount(numAllFaces);
    pFBXMesh->ReservePolygonVertexCount(numAllFaces * 3);

    // Set material mapping.
    FbxGeometryElementMaterial* pMaterialElement = pFBXMesh->CreateElementMaterial();
    pMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
    pMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

    for (int j = 0; j < numMeshes; ++j)
    {
        const Export::Mesh* pMesh = pObj->GetMesh(j);

        // Write all faces, convert the indices to one based indices
        int numFaces = pMesh->GetFaceCount();

        int polygonCount = pFBXMesh->GetPolygonCount();
        pMaterialElement->GetIndexArray().SetCount(polygonCount + numFaces);
        int materialIndex = m_meshMaterialIndices[pMesh];

        const Export::Face* pFaces = pMesh->GetFaceBuffer();
        for (int i = 0; i < numFaces; ++i)
        {
            const Export::Face& face = pFaces[i];
            pFBXMesh->BeginPolygon(-1, -1, -1, false);

            pFBXMesh->AddPolygon(face.idx[0], face.idx[0]);
            pFBXMesh->AddPolygon(face.idx[1], face.idx[1]);
            pFBXMesh->AddPolygon(face.idx[2], face.idx[2]);

            //pFBXDiffuseUV->GetIndexArray().SetAt(face.idx[0], face.idx[0]);

            pFBXMesh->EndPolygon();
            //pPolygonGroupElement->GetIndexArray().Add(j);

            pMaterialElement->GetIndexArray().SetAt(polygonCount + i, materialIndex);
        }
    }

    return pFBXMesh;
}


FbxFileTexture* CFBXExporter::CreateFBXTexture(const char* pTypeName, const char* pName)
{
    std::string filename = pName;

    if (m_settings.bCopyTextures)
    {
        // check if file exists
        QFileInfo fi(pName);
        if (!fi.exists() || fi.isDir())
        {
            GetIEditor()->GetSystem()->GetILog()->LogError("\nFBX Exporter: Texture %s is not on the disk.", pName);
        }
        else
        {
            filename = m_path;
            filename += GetFileName(pName);
            if (QFile::copy(pName, filename.c_str()))
            {
                GetIEditor()->GetSystem()->GetILog()->Log("\nFBX Exporter: Texture %s was copied.", pName);
            }
            filename = GetFileName(pName); // It works for Maya, but not good for MAX
        }
    }

    FbxFileTexture* pFBXTexture = FbxFileTexture::Create(m_pFBXManager, pTypeName);
    pFBXTexture->SetFileName(filename.c_str());
    pFBXTexture->SetTextureUse(FbxTexture::eStandard);
    pFBXTexture->SetMappingType(FbxTexture::eUV);
    pFBXTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
    pFBXTexture->SetSwapUV(false);
    pFBXTexture->SetTranslation(0.0, 0.0);
    pFBXTexture->SetScale(1.0, 1.0);
    pFBXTexture->SetRotation(0.0, 0.0);

    return pFBXTexture;
}


FbxSurfaceMaterial* CFBXExporter::CreateFBXMaterial(const std::string& name, const Export::Mesh* pMesh)
{
    auto it = m_materials.find(name);
    if (it != m_materials.end())
    {
        return it->second;
    }

    FbxSurfacePhong* pMaterial = FbxSurfacePhong::Create(m_pFBXManager, name.c_str());

    pMaterial->Diffuse.Set(FbxDouble3(pMesh->material.diffuse.r, pMesh->material.diffuse.g, pMesh->material.diffuse.b));
    pMaterial->DiffuseFactor.Set(1.0);
    pMaterial->Specular.Set(FbxDouble3(pMesh->material.specular.r, pMesh->material.specular.g, pMesh->material.specular.b));
    pMaterial->SpecularFactor.Set(1.0);

    // Opacity
    pMaterial->TransparentColor.Set(FbxDouble3(1.0f, 1.0f, 1.0f)); // need explicitly setup
    pMaterial->TransparencyFactor.Set(1.0f - pMesh->material.opacity);

    // Glossiness
    // CRC TODO: Why was this commented out?
    //pMaterial->Shininess.Set(pow(2, pMesh->material.shininess * 10));

    if (pMesh->material.mapDiffuse[0] != '\0')
    {
        FbxFileTexture* pFBXTexture = CreateFBXTexture("Diffuse", pMesh->material.mapDiffuse);
        pMaterial->Diffuse.ConnectSrcObject(pFBXTexture);
    }

    if (pMesh->material.mapSpecular[0] != '\0')
    {
        FbxFileTexture* pFBXTexture = CreateFBXTexture("Specular", pMesh->material.mapSpecular);
        pMaterial->Specular.ConnectSrcObject(pFBXTexture);
    }

    if (pMesh->material.mapOpacity[0] != '\0')
    {
        FbxFileTexture* pFBXTexture = CreateFBXTexture("Opacity", pMesh->material.mapOpacity);
        pMaterial->TransparentColor.ConnectSrcObject(pFBXTexture);
    }
    // CRC TODO: Why was this commented out?
    /*
    if (pMesh->material.mapBump[0] != '\0')
    {
        FbxFileTexture* pFBXTexture = CreateFBXTexture("Bump", pMesh->material.mapBump);
        pMaterial->Bump.ConnectSrcObject(pFBXTexture);
    }
    */

    if (pMesh->material.mapDisplacement[0] != '\0')
    {
        FbxFileTexture* pFBXTexture = CreateFBXTexture("Displacement", pMesh->material.mapDisplacement);
        pMaterial->DisplacementColor.ConnectSrcObject(pFBXTexture);
    }

    m_materials[name] = pMaterial;

    return pMaterial;
}


FbxNode* CFBXExporter::CreateFBXAnimNode(FbxScene* pScene, FbxAnimLayer* pAnimBaseLayer, const Export::Object* pObj)
{
    FbxNode* pAnimNode = FbxNode::Create(pScene, pObj->name);
    pAnimNode->LclTranslation.GetCurveNode(pAnimBaseLayer, true);
    pAnimNode->LclRotation.GetCurveNode(pAnimBaseLayer, true);
    pAnimNode->LclScaling.GetCurveNode(pAnimBaseLayer, true);

    FbxCamera* pCamera = FbxCamera::Create(pScene, pObj->name);

    if (pObj->entityType == Export::eCamera)
    {
        pCamera->SetApertureMode(FbxCamera::eHorizontal);
        pCamera->SetFormat(FbxCamera::eCustomFormat);
        pCamera->SetAspect(FbxCamera::eFixedRatio, 1.777778f, 1.0);

        pAnimNode->SetNodeAttribute(pCamera);

        if (pObj->cameraTargetNodeName[0])
        {
            for (int i = 0; i < m_nodes.size(); ++i)
            {
                FbxNode* pCameraTargetNode = m_nodes[i];
                const char* nodeName = pCameraTargetNode->GetName();
                if (strcmp(nodeName, pObj->cameraTargetNodeName) == 0)
                {
                    FbxMarker* pMarker = FbxMarker::Create(pScene, "");
                    pCameraTargetNode->SetNodeAttribute(pMarker);
                    pAnimNode->SetTarget(pCameraTargetNode);
                }
            }
        }
    }

    if (pObj->GetEntityAnimationDataCount() > 0)
    {
        int animDataCount = pObj->GetEntityAnimationDataCount();

        for (int animDataIndex = 0; animDataIndex < animDataCount; ++animDataIndex)
        {
            const Export::EntityAnimData* pAnimData = pObj->GetEntityAnimationData(animDataIndex);

            FbxTime time = 0;
            time.SetSecondDouble(pAnimData->keyTime);

            FbxAnimCurve* pCurve = NULL;

            switch (pAnimData->dataType)
            {
            case Export::AnimParamType::PositionX:
                pCurve = pAnimNode->LclTranslation.GetCurve(pAnimBaseLayer, "X", true);
                break;
            case Export::AnimParamType::PositionY:
                pCurve = pAnimNode->LclTranslation.GetCurve(pAnimBaseLayer, "Y", true);
                break;
            case Export::AnimParamType::PositionZ:
                pCurve = pAnimNode->LclTranslation.GetCurve(pAnimBaseLayer, "Z", true);
                break;
            case Export::AnimParamType::RotationX:
                pCurve = pAnimNode->LclRotation.GetCurve(pAnimBaseLayer, "X", true);
                break;
            case Export::AnimParamType::RotationY:
                pCurve = pAnimNode->LclRotation.GetCurve(pAnimBaseLayer, "Y", true);
                break;
            case Export::AnimParamType::RotationZ:
                pCurve = pAnimNode->LclRotation.GetCurve(pAnimBaseLayer, "Z", true);
                break;
            case Export::AnimParamType::FOV:
            {
                pCurve = pCamera->FieldOfView.GetCurve(pAnimBaseLayer, "FieldOfView", true);
            }
            break;

            default:
                break;
            }

            if (pCurve)
            {
                int keyIndex = 0;

                pCurve->KeyModifyBegin();
                keyIndex = pCurve->KeyInsert(time);
                pCurve->KeySet(keyIndex, time, pAnimData->keyValue, FbxAnimCurveDef::eInterpolationCubic, FbxAnimCurveDef::eTangentBreak, pAnimData->rightTangent, pAnimData->leftTangent, FbxAnimCurveDef::eWeightedAll, pAnimData->rightTangentWeight, pAnimData->leftTangentWeight);

                pCurve->KeySetLeftDerivative(keyIndex, pAnimData->leftTangent);
                pCurve->KeySetRightDerivative(keyIndex, pAnimData->rightTangent);

                pCurve->KeySetLeftTangentWeight(keyIndex, pAnimData->leftTangentWeight);
                pCurve->KeySetRightTangentWeight(keyIndex, pAnimData->rightTangentWeight);

                FbxAnimCurveTangentInfo keyLeftInfo;
                keyLeftInfo.mAuto = 0;
                keyLeftInfo.mDerivative = pAnimData->leftTangent;
                keyLeftInfo.mWeight = pAnimData->leftTangentWeight;
                keyLeftInfo.mWeighted = true;
                keyLeftInfo.mVelocity = 0.0f;
                keyLeftInfo.mHasVelocity = false;

                FbxAnimCurveTangentInfo keyRightInfo;
                keyRightInfo.mAuto = 0;
                keyRightInfo.mDerivative = pAnimData->rightTangent;
                keyRightInfo.mWeight = pAnimData->rightTangentWeight;
                keyRightInfo.mWeighted = true;
                keyRightInfo.mVelocity = 0.0f;
                keyRightInfo.mHasVelocity = false;

                pCurve->KeySetLeftDerivativeInfo(keyIndex, keyLeftInfo);
                pCurve->KeySetRightDerivativeInfo(keyIndex, keyRightInfo);

                pCurve->KeyModifyEnd();
            }
        }
    }

    return pAnimNode;
}

FbxNode* CFBXExporter::CreateFBXNode(const Export::Object* pObj)
{
    if (!m_pFBXManager)
    {
        return 0;
    }

    // create a FbxNode
    FbxNode* pNode = FbxNode::Create(m_pFBXManager, pObj->name);
    pNode->LclTranslation.Set(FbxVector4(pObj->pos.x, pObj->pos.y, pObj->pos.z));

    // Rotation: Create Euler angels from quaternion throughout a matrix
    FbxAMatrix rotMat;
    rotMat.SetQ(FbxQuaternion(pObj->rot.v.x, pObj->rot.v.y, pObj->rot.v.z, pObj->rot.w));
    pNode->LclRotation.Set(rotMat.GetR());

    pNode->LclScaling.Set(FbxVector4(pObj->scale.x, pObj->scale.y, pObj->scale.z));


    // collect materials
    int materialIndex = 0;
    if (pObj->GetMeshCount() != 0 && pObj->materialName[0] != '\0')
    {
        for (int i = 0; i < pObj->GetMeshCount(); ++i)
        {
            const Export::Mesh* pMesh = pObj->GetMesh(i);

            if (pMesh->material.name[0] == '\0')
            {
                continue;
            }

            // find if material was created for this object
            int findIndex = -1;
            for (int j = 0; j < i; ++j)
            {
                const Export::Mesh* pTestMesh = pObj->GetMesh(j);
                if (!strcmp(pTestMesh->material.name, pMesh->material.name))
                {
                    findIndex = m_meshMaterialIndices[pTestMesh];
                    break;
                }
            }

            if (findIndex != -1)
            {
                m_meshMaterialIndices[pMesh] = findIndex;
                continue;
            }

            std::string materialName = pObj->materialName;
            if (strcmp(pObj->materialName, pMesh->material.name))
            {
                materialName += ':';
                materialName += pMesh->material.name;
            }

            FbxSurfaceMaterial* pFBXMaterial = CreateFBXMaterial(materialName, pMesh);
            if (pFBXMaterial)
            {
                pNode->AddMaterial(pFBXMaterial);
                m_meshMaterialIndices[pMesh] = materialIndex++;
            }
        }
    }


    FbxMesh* pFBXMesh = CreateFBXMesh(pObj);
    if (pFBXMesh)
    {
        pNode->SetNodeAttribute(pFBXMesh);
        pNode->SetShadingMode(FbxNode::eTextureShading);
    }

    return pNode;
}

void CFBXExporter::Release()
{
    delete this;
}

inline f32 Maya2SandboxFOVDeg(const f32 fov, const f32 ratio) { return RAD2DEG(2.0f * atanf(tan(DEG2RAD(fov) / 2.0f) / ratio)); };

void CFBXExporter::FillAnimationData(Export::Object* pObject, FbxNode* pNode, [[maybe_unused]] FbxAnimLayer* pAnimLayer, FbxAnimCurve* pCurve, Export::AnimParamType paramType)
{
    if (pCurve)
    {
        for (int keyID = 0; keyID < pCurve->KeyGetCount(); ++keyID)
        {
            Export::EntityAnimData entityData;

            FbxAnimCurveKey key = pCurve->KeyGet(keyID);
            entityData.keyValue = key.GetValue();
            FbxTime time = key.GetTime();
            entityData.keyTime = aznumeric_cast<float>(time.GetSecondDouble());

            entityData.leftTangent = pCurve->KeyGetLeftDerivative(keyID);
            entityData.rightTangent = pCurve->KeyGetRightDerivative(keyID);
            entityData.leftTangentWeight = pCurve->KeyGetLeftTangentWeight(keyID);
            entityData.rightTangentWeight = pCurve->KeyGetRightTangentWeight(keyID);

            if (paramType == Export::AnimParamType::FocalLength && pNode && pNode->GetCamera())
            {
                // special handling for Focal Length - we convert it to FoV for use in-engine (including switching the paramType)
                // We handle this because Maya 2015 doesn't save Angle of View or Field of View animation in FBX - it only uses FocalLength.
                entityData.dataType = Export::AnimParamType::FOV;
                // Open 3D Engine field of view is the vertical angle
                pNode->GetCamera()->SetApertureMode(FbxCamera::eVertical);
                entityData.keyValue = aznumeric_cast<float>(pNode->GetCamera()->ComputeFieldOfView(entityData.keyValue));
            }
            else
            {
                entityData.dataType = paramType;
            }

            // If the Exporter is Sandbox or Maya, convert fov, positions
            QString exporterName = m_pFBXScene->GetDocumentInfo()->Original_ApplicationName.Get().Buffer();

            if (!exporterName.toLower().contains("3ds"))
            {
                if (paramType == Export::AnimParamType::PositionX || paramType == Export::AnimParamType::PositionY || paramType == Export::AnimParamType::PositionZ)
                {
                    entityData.rightTangent /= 100.0f;
                    entityData.leftTangent /= 100.0f;
                    entityData.keyValue /= 100.0f;
                }
                else if (paramType == Export::AnimParamType::FOV)                       // Maya 2015 uses FocalLength instead of FoV - assuming this is for legacy Sandbox or Maya?
                {
                    const float kAspectRatio = 1.777778f;
                    entityData.keyValue = Maya2SandboxFOVDeg(entityData.keyValue, kAspectRatio);
                }
            }

            pObject->SetEntityAnimationData(entityData);
        }
    }
}


bool CFBXExporter::ImportFromFile(const char* filename, Export::IData* pData)
{
    FbxManager* pFBXManager = FbxManager::Create();

    FbxScene* pFBXScene = FbxScene::Create(pFBXManager, "Test");
    m_pFBXScene = pFBXScene;
    pFBXScene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::Max);
    pFBXScene->GetGlobalSettings().SetOriginalUpAxis(FbxAxisSystem::Max);

    FbxImporter* pImporter = FbxImporter::Create(pFBXManager, "");

    FbxIOSettings* pSettings = FbxIOSettings::Create(pFBXManager, IOSROOT);

    pFBXManager->SetIOSettings(pSettings);
    pSettings->SetBoolProp(IMP_FBX_ANIMATION, true);

    const bool lImportStatus = pImporter->Initialize(filename, -1, pFBXManager->GetIOSettings());
    if (!lImportStatus)
    {
        return false;
    }

    bool bLoadStatus = pImporter->Import(pFBXScene);

    if (!bLoadStatus)
    {
        return false;
    }

    // record the original axis system used in the import file and then convert the file to Open 3D Engine's coord system,
    // which matches Max's (Z-Up, negative Y-forward cameras)
    int upSign = 1;
    FbxAxisSystem importFileAxisSystem = pFBXScene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem::EUpVector importSceneUpVector = importFileAxisSystem.GetUpVector(upSign);
    FbxAxisSystem::Max.ConvertScene(pFBXScene);

    QString exporterName = pFBXScene->GetDocumentInfo()->Original_ApplicationName.Get().Buffer();
    // const bool fromMaya = (exporterName.contains("maya", Qt::CaseInsensitive));
    // const bool fromMax = (exporterName.contains("max", Qt::CaseInsensitive));

    FbxNode* pRootNode = pFBXScene->GetRootNode();

    if (pRootNode)
    {
        for (int animStackID = 0; animStackID < pFBXScene->GetSrcObjectCount<FbxAnimStack>(); ++animStackID)
        {
            FbxAnimStack* pAnimStack = pFBXScene->GetSrcObject<FbxAnimStack>(animStackID);
            const int animLayersCount = pAnimStack->GetMemberCount<FbxAnimLayer>();

            for (int layerID = 0; layerID < animLayersCount; ++layerID)
            {
                FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(layerID);

                const int nodeCount = pRootNode->GetChildCount();

                for (int nodeID = 0; nodeID < nodeCount; ++nodeID)
                {
                    FbxNode* pNode = pRootNode->GetChild(nodeID);

                    if (pNode)
                    {
                        FbxAnimCurve* pCurve = 0;
                        Export::Object* pObject = pData->AddObject(pNode->GetName());

                        if (pObject)
                        {
                            azstrncpy(pObject->name, sizeof(pObject->name), pNode->GetName(), sizeof(pObject->name) - 1);
                            pObject->name[sizeof(pObject->name) - 1] = '\0';

                            FbxCamera* pCamera = pNode->GetCamera();

                            // convert animation for Y-Up scenes and for Z-forward Cameras
                            if (importSceneUpVector == FbxAxisSystem::eYAxis || pCamera)
                            {
                                pNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
                                pNode->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);

                                if (importSceneUpVector == FbxAxisSystem::eYAxis)
                                {
                                    // Maps RY to -RZ and RZ to RY
                                    pNode->SetPreRotation(FbxNode::eSourcePivot, s_PRE_ROTATION_FOR_YUP_SCENES);
                                }

                                if (pCamera)
                                {
                                    // Converts Y-Up, -Z-forward cameras to Open 3D Engine Z-Up, Y-forward cameras
                                    // It is needed regardless of the scene up vector
                                    pNode->SetPostRotation(FbxNode::eSourcePivot, s_POST_ROTATION_FOR_ZFORWARD_CAMERAS);
                                }
                                else
                                {
                                    // Objects from a Y-Up scene (i.e. not cameras). 'undo' the extra transform that the Open 3D Engine Tool
                                    // bakes in to .cgf files from YUp scenes.
                                    pNode->SetPostRotation(FbxNode::eSourcePivot, s_POST_ROTATION_FOR_YUP_OBJECTS);
                                }

                                // bake the pre/post rotations into the anim curves
                                pNode->ConvertPivotAnimationRecursive(pAnimStack, FbxNode::eSourcePivot, s_CONVERSION_BAKING_SAMPLE_RATE);
                            }

                            if (pCamera)
                            {
                                // extract specialized channels for cameras
                                pCurve = pCamera->FocalLength.GetCurve(pAnimLayer, "FocalLength");
                                FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::FocalLength);
                                pCurve = pCamera->FieldOfView.GetCurve(pAnimLayer, "FieldOfView", true);
                                FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::FOV);
                            }

                            pCurve = pNode->LclTranslation.GetCurve(pAnimLayer, "X");
                            FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::PositionX);
                            pCurve = pNode->LclTranslation.GetCurve(pAnimLayer, "Y");
                            FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::PositionY);
                            pCurve = pNode->LclTranslation.GetCurve(pAnimLayer, "Z");
                            FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::PositionZ);
                            pCurve = pNode->LclRotation.GetCurve(pAnimLayer, "X");
                            FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::RotationX);
                            pCurve = pNode->LclRotation.GetCurve(pAnimLayer, "Y");
                            FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::RotationY);
                            pCurve = pNode->LclRotation.GetCurve(pAnimLayer, "Z");
                            FillAnimationData(pObject, pNode, pAnimLayer, pCurve, Export::AnimParamType::RotationZ);
                        }
                    }
                }
            }
        }
    }

    return true;
}
