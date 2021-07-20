/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ExportManager.h"

// Qt
#include <QMessageBox>

// Maestro
#include <Maestro/Types/AnimParamType.h>

// Editor
#include "ViewManager.h"
#include "OBJExporter.h"
#include "OCMExporter.h"
#include "FBXExporterDialog.h"
#include "RenderViewport.h"
#include "TrackViewExportKeyTimeDlg.h"
#include "AnimationContext.h"
#include "TrackView/DirectorNodeAnimator.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"
#include "QtUI/WaitCursor.h"
#include "WaitProgress.h"
#include "Objects/SelectionGroup.h"
#include "Include/IObjectManager.h"
#include "TrackView/TrackViewTrack.h"
#include "TrackView/TrackViewSequenceManager.h"
#include "Resource.h"
#include "Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h"

#include <IEntityRenderState.h>
#include <IStatObj.h>

namespace
{
    void SetTexture(Export::TPath& outName, IRenderShaderResources* pRes, int nSlot)
    {
        SEfResTexture* pTex = pRes->GetTextureResource(nSlot);
        if (pTex)
        {
            cry_strcat(outName, Path::GamePathToFullPath(pTex->m_Name.c_str()).toUtf8().data());
        }
    }

    inline Export::Vector3D Vec3ToVector3D(const Vec3& vec)
    {
        Export::Vector3D ret;
        ret.x = vec.x;
        ret.y = vec.y;
        ret.z = vec.z;
        return ret;
    }

    const float kTangentDelta = 0.01f;
    const float kAspectRatio = 1.777778f;
    const int kReserveCount = 7; // x,y,z,rot_x,rot_y,rot_z,fov
    const QString kPrimaryCameraName = "PrimaryCamera";
} // namespace



//////////////////////////////////////////////////////////
// CMesh
Export::CMesh::CMesh()
{
    ::ZeroMemory(&material, sizeof(material));
    material.opacity = 1.0f;
}


//////////////////////////////////////////////////////////
// CObject
Export::CObject::CObject(const char* pName)
    : m_MeshHash(0)
{
    pos.x = pos.y = pos.z = 0;
    rot.v.x = rot.v.y = rot.v.z = 0;
    rot.w = 1.0f;
    scale.x = scale.y = scale.z = 1.0f;

    nParent = -1;

    cry_strcpy(name, pName);

    materialName[0] = '\0';

    entityType = Export::eEntity;

    cameraTargetNodeName[0] = '\0';

    m_pLastObject = 0;
}


void Export::CObject::SetMaterialName(const char* pName)
{
    cry_strcpy(materialName, pName);
}


// CExportData
void Export::CData::Clear()
{
    m_objects.clear();
}


// CExportManager
CExportManager::CExportManager()
    : m_isPrecaching(false)
    , m_pBaseObj(0)
    , m_FBXBakedExportFPS(0.0f)
    , m_fScale(100.0f)
    ,                 // this scale is used by CryEngine RC
    m_bAnimationExport(false)
    , m_bExportLocalCoords(false)
    , m_numberOfExportFrames(0)
    , m_pivotEntityObject(0)
    , m_bBakedKeysSequenceExport(true)
    , m_animTimeExportPrimarySequenceCurrentTime(0.0f)
    , m_animKeyTimeExport(true)
    , m_soundKeyTimeExport(true)
    , m_bExportOnlyPrimaryCamera(false)
{
    RegisterExporter(new COBJExporter());
    RegisterExporter(new COCMExporter());
}


CExportManager::~CExportManager()
{
    m_data.Clear();
    for (TExporters::iterator ppExporter = m_exporters.begin(); ppExporter != m_exporters.end(); ++ppExporter)
    {
        (*ppExporter)->Release();
    }
}


bool CExportManager::RegisterExporter(IExporter* pExporter)
{
    if (!pExporter)
    {
        return false;
    }

    m_exporters.push_back(pExporter);
    return true;
}

inline f32 Sandbox2MayaFOVDeg(const f32 fov, const f32 ratio) { return RAD2DEG(2.0f * atan_tpl(tan(DEG2RAD(fov) / 2.0f) * ratio)); };
inline f32 Sandbox2MayaFOVRad2Deg(const f32 fov, const f32 ratio) { return RAD2DEG(2.0f * atan_tpl(tan(fov / 2.0f) * ratio)); };

void CExportManager::AddEntityAnimationData(const CTrackViewTrack* pTrack, Export::CObject* pObj, AnimParamType entityTrackParamType)
{
    int keyCount = pTrack->GetKeyCount();
    pObj->m_entityAnimData.reserve(keyCount * kReserveCount);

    for (int keyNumber = 0; keyNumber < keyCount; keyNumber++)
    {
        const CTrackViewKeyConstHandle currentKeyHandle = pTrack->GetKey(keyNumber);
        const float fCurrentKeytime = currentKeyHandle.GetTime();

        float trackValue = 0.0f;
        pTrack->GetValue(fCurrentKeytime, trackValue);

        ISplineInterpolator* pSpline = pTrack->GetSpline();

        if (pSpline)
        {
            Export::EntityAnimData entityData;
            entityData.dataType = (Export::AnimParamType)pTrack->GetParameterType().GetType();
            entityData.keyValue = trackValue;
            entityData.keyTime = fCurrentKeytime;

            if (entityTrackParamType == AnimParamType::Position)
            {
                entityData.keyValue *= 100.0f;
            }
            else if (entityTrackParamType == AnimParamType::FOV)
            {
                entityData.keyValue = Sandbox2MayaFOVDeg(entityData.keyValue, kAspectRatio);
            }


            ISplineInterpolator::ValueType tin;
            ISplineInterpolator::ValueType tout;
            ZeroStruct(tin);
            ZeroStruct(tout);
            pSpline->GetKeyTangents(keyNumber, tin, tout);

            float fInTantentX = tin[0];
            float fInTantentY = tin[1];

            float fOutTantentX = tout[0];
            float fOutTantentY = tout[1];

            if (fInTantentX == 0.0f)
            {
                fInTantentX = kTangentDelta;
            }

            if (fOutTantentX == 0.0f)
            {
                fOutTantentX = kTangentDelta;
            }

            entityData.leftTangent = fInTantentY / fInTantentX;
            entityData.rightTangent = fOutTantentY / fOutTantentX;

            if (entityTrackParamType == AnimParamType::Position)
            {
                entityData.leftTangent *= 100.0f;
                entityData.rightTangent *= 100.0f;
            }

            float fPrevKeyTime = 0.0f;
            float fNextKeyTime = 0.0f;

            bool bIsFirstKey = false;
            bool bIsMiddleKey = false;
            bool bIsLastKey = false;

            if (keyNumber == 0 && keyNumber < (keyCount - 1))
            {
                const CTrackViewKeyConstHandle nextKeyHandle = pTrack->GetKey(keyNumber + 1);
                fNextKeyTime = nextKeyHandle.GetTime();

                if (fNextKeyTime != 0.0f)
                {
                    bIsFirstKey = true;
                }
            }
            else if (keyNumber > 0)
            {
                const CTrackViewKeyConstHandle prevKeyHandle = pTrack->GetKey(keyNumber - 1);
                fPrevKeyTime = prevKeyHandle.GetTime();

                if (keyNumber < (keyCount - 1))
                {
                    const CTrackViewKeyConstHandle nextKeyHandle = pTrack->GetKey(keyNumber + 1);
                    fNextKeyTime = nextKeyHandle.GetTime();

                    if (fNextKeyTime != 0.0f)
                    {
                        bIsMiddleKey = true;
                    }
                }
                else
                {
                    bIsLastKey = true;
                }
            }

            float fLeftTangentWeightValue = 0.0f;
            float fRightTangentWeightValue = 0.0f;

            if (bIsFirstKey)
            {
                fRightTangentWeightValue = fOutTantentX / fNextKeyTime;
            }
            else if (bIsMiddleKey)
            {
                fLeftTangentWeightValue = fInTantentX / (fCurrentKeytime - fPrevKeyTime);
                fRightTangentWeightValue = fOutTantentX / (fNextKeyTime - fCurrentKeytime);
            }
            else if (bIsLastKey)
            {
                fLeftTangentWeightValue = fInTantentX / (fCurrentKeytime - fPrevKeyTime);
            }

            entityData.leftTangentWeight = fLeftTangentWeightValue;
            entityData.rightTangentWeight = fRightTangentWeightValue;

            pObj->m_entityAnimData.push_back(entityData);
        }
    }
}


void CExportManager::ProcessEntityAnimationTrack(
    const AZ::EntityId entityId, Export::CObject* pObj, AnimParamType entityTrackParamType)
{
    CTrackViewAnimNode* pEntityNode = GetIEditor()->GetSequenceManager()->GetActiveAnimNode(entityId);
    CTrackViewTrack* pEntityTrack = (pEntityNode ? pEntityNode->GetTrackForParameter(entityTrackParamType) : 0);

    if (!pEntityTrack)
    {
        return;
    }

    if (pEntityTrack->GetParameterType() == AnimParamType::FOV)
    {
        AddEntityAnimationData(pEntityTrack, pObj, entityTrackParamType);
        return;
    }

    for (int trackNumber = 0; trackNumber < pEntityTrack->GetChildCount(); ++trackNumber)
    {
        CTrackViewTrack* pSubTrack = static_cast<CTrackViewTrack*>(pEntityTrack->GetChild(trackNumber));

        if (pSubTrack)
        {
            AddEntityAnimationData(pSubTrack, pObj, entityTrackParamType);
        }
    }
}


void CExportManager::AddEntityAnimationData(AZ::EntityId entityId)
{
    Export::CObject* pObj = new Export::CObject(m_pBaseObj->GetName().toUtf8().data());

    ProcessEntityAnimationTrack(entityId, pObj, AnimParamType::Position);
    ProcessEntityAnimationTrack(entityId, pObj, AnimParamType::Rotation);
}


void CExportManager::AddMesh(Export::CObject* pObj, const IIndexedMesh* pIndMesh, Matrix34A* pTm)
{
    if (m_isPrecaching || !pObj)
    {
        return;
    }

    pObj->m_MeshHash    =   reinterpret_cast<size_t>(pIndMesh);
    IIndexedMesh::SMeshDescription meshDesc;
    pIndMesh->GetMeshDescription(meshDesc);

    // if we have subset of meshes we need to duplicate vertices,
    // keep transformation of submesh,
    // and store new offset for indices
    int newOffsetIndex = pObj->GetVertexCount();

    if (meshDesc.m_nVertCount)
    {
        pObj->m_vertices.reserve(meshDesc.m_nVertCount + newOffsetIndex);
        pObj->m_normals.reserve(meshDesc.m_nVertCount + newOffsetIndex);
    }

    for (int v = 0; v < meshDesc.m_nVertCount; ++v)
    {
        Vec3 n = meshDesc.m_pNorms[v].GetN();
        Vec3 tmp = (meshDesc.m_pVerts ? meshDesc.m_pVerts[v] : meshDesc.m_pVertsF16[v].ToVec3());
        if (pTm)
        {
            tmp = pTm->TransformPoint(tmp);
        }

        pObj->m_vertices.push_back(Vec3ToVector3D(tmp * m_fScale));
        pObj->m_normals.push_back(Vec3ToVector3D(n));
    }

    if (meshDesc.m_nCoorCount)
    {
        pObj->m_texCoords.reserve(meshDesc.m_nCoorCount + newOffsetIndex);
    }

    for (int v = 0; v < meshDesc.m_nCoorCount; ++v)
    {
        Export::UV tc;
        meshDesc.m_pTexCoord[v].ExportTo(tc.u, tc.v);
        tc.v = 1.0f - tc.v;
        pObj->m_texCoords.push_back(tc);
    }

    if (pIndMesh->GetSubSetCount() && !(pIndMesh->GetSubSetCount() == 1 && pIndMesh->GetSubSet(0).nNumIndices == 0))
    {
        for (int i = 0; i < pIndMesh->GetSubSetCount(); ++i)
        {
            Export::CMesh* pMesh = new Export::CMesh();

            const SMeshSubset& sms = pIndMesh->GetSubSet(i);
            const vtx_idx* pIndices = &meshDesc.m_pIndices[sms.nFirstIndexId];
            int nTris = sms.nNumIndices / 3;
            pMesh->m_faces.reserve(nTris);
            for (int f = 0; f < nTris; ++f)
            {
                Export::Face face;
                face.idx[0] = *(pIndices++) + newOffsetIndex;
                face.idx[1] = *(pIndices++) + newOffsetIndex;
                face.idx[2] = *(pIndices++) + newOffsetIndex;
                pMesh->m_faces.push_back(face);
            }

            pObj->m_meshes.push_back(pMesh);
        }
    }
    else
    {
        Export::CMesh* pMesh = new Export::CMesh();
        if (meshDesc.m_nFaceCount == 0 && meshDesc.m_nIndexCount != 0 && meshDesc.m_pIndices != 0)
        {
            const vtx_idx* pIndices = &meshDesc.m_pIndices[0];
            int nTris = meshDesc.m_nIndexCount / 3;
            pMesh->m_faces.reserve(nTris);
            for (int f = 0; f < nTris; ++f)
            {
                Export::Face face;
                face.idx[0] = *(pIndices++) + newOffsetIndex;
                face.idx[1] = *(pIndices++) + newOffsetIndex;
                face.idx[2] = *(pIndices++) + newOffsetIndex;
                pMesh->m_faces.push_back(face);
            }
        }
        else
        {
            pMesh->m_faces.reserve(meshDesc.m_nFaceCount);
            for (int f = 0; f < meshDesc.m_nFaceCount; ++f)
            {
                Export::Face face;
                face.idx[0] = meshDesc.m_pFaces[f].v[0];
                face.idx[1] = meshDesc.m_pFaces[f].v[1];
                face.idx[2] = meshDesc.m_pFaces[f].v[2];
                pMesh->m_faces.push_back(face);
            }
        }

        pObj->m_meshes.push_back(pMesh);
    }
}


bool CExportManager::AddStatObj(Export::CObject* pObj, IStatObj* pStatObj, Matrix34A* pTm)
{
    IIndexedMesh* pIndMesh = 0;

    if (pStatObj->GetSubObjectCount())
    {
        for (int i = 0; i < pStatObj->GetSubObjectCount(); i++)
        {
            IStatObj::SSubObject* pSubObj = pStatObj->GetSubObject(i);
            if (pSubObj && pSubObj->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj)
            {
                pIndMesh = 0;
                if (m_isOccluder)
                {
                    if (pSubObj->pStatObj->GetLodObject(2))
                    {
                        pIndMesh = pSubObj->pStatObj->GetLodObject(2)->GetIndexedMesh(true);
                    }
                    if (!pIndMesh && pSubObj->pStatObj->GetLodObject(1))
                    {
                        pIndMesh = pSubObj->pStatObj->GetLodObject(1)->GetIndexedMesh(true);
                    }
                }
                if (!pIndMesh)
                {
                    pIndMesh = pSubObj->pStatObj->GetIndexedMesh(true);
                }
                if (pIndMesh)
                {
                    AddMesh(pObj, pIndMesh, pTm);
                }
            }
        }
    }

    if (!pIndMesh)
    {
        if (m_isOccluder)
        {
            if (pStatObj->GetLodObject(2))
            {
                pIndMesh = pStatObj->GetLodObject(2)->GetIndexedMesh(true);
            }
            if (!pIndMesh && pStatObj->GetLodObject(1))
            {
                pIndMesh = pStatObj->GetLodObject(1)->GetIndexedMesh(true);
            }
        }
        if (!pIndMesh)
        {
            pIndMesh = pStatObj->GetIndexedMesh(true);
        }
        if (pIndMesh)
        {
            AddMesh(pObj, pIndMesh, pTm);
        }
    }

    return true;
}

bool CExportManager::AddMeshes(Export::CObject* pObj)
{
    if (m_pBaseObj->GetType() == OBJTYPE_AZENTITY)
    {
        CEntityObject* pEntityObject = (CEntityObject*)m_pBaseObj;
        IRenderNode* pEngineNode = pEntityObject->GetEngineNode();

        if (pEngineNode)
        {
            if (!m_isPrecaching)
            {
                for (int i = 0; i < pEngineNode->GetSlotCount(); ++i)
                {
                    Matrix34A tm;
                    IStatObj* pStatObj = pEngineNode->GetEntityStatObj(i, 0, &tm);
                    if (pStatObj)
                    {
                        Matrix34A objTM = m_pBaseObj->GetWorldTM();
                        objTM.Invert();
                        tm = objTM * tm;
                        AddStatObj(pObj, pStatObj, &tm);
                    }
                }
            }
        }
    }

    return true;
}


bool CExportManager::AddObject(CBaseObject* pBaseObj)
{
    if (m_isOccluder)
    {
        return false;
    }

    m_pBaseObj = pBaseObj;

    if (m_bAnimationExport)
    {
        if (pBaseObj->GetType() == OBJTYPE_AZENTITY)
        {
            const auto componentEntityObject = static_cast<CComponentEntityObject*>(pBaseObj);
            AddEntityAnimationData(componentEntityObject->GetAssociatedEntityId());
            return true;
        }
    }

    if (m_isPrecaching)
    {
        AddMeshes(0);
        return true;
    }

    Export::CObject* pObj = new Export::CObject(m_pBaseObj->GetName().toUtf8().data());

    AddPosRotScale(pObj, pBaseObj);
    m_data.m_objects.push_back(pObj);

    m_objectMap[pBaseObj] = int(m_data.m_objects.size() - 1);

    AddMeshes(pObj);
    m_pBaseObj = 0;

    return true;
}


void CExportManager::AddPosRotScale(Export::CObject* pObj, const CBaseObject* pBaseObj)
{
    Vec3 pos = pBaseObj->GetPos();
    pObj->pos.x = pos.x * m_fScale;
    pObj->pos.y = pos.y * m_fScale;
    pObj->pos.z = pos.z * m_fScale;

    Quat rot = pBaseObj->GetRotation();
    pObj->rot.v.x = rot.v.x;
    pObj->rot.v.y = rot.v.y;
    pObj->rot.v.z = rot.v.z;
    pObj->rot.w = rot.w;

    Vec3 scale = pBaseObj->GetScale();
    pObj->scale.x = scale.x;
    pObj->scale.y = scale.y;
    pObj->scale.z = scale.z;
}

void CExportManager::AddEntityData(Export::CObject* pObj, Export::AnimParamType dataType, const float fValue, const float fTime)
{
    Export::EntityAnimData entityData;
    entityData.dataType = dataType;
    entityData.leftTangent = kTangentDelta;
    entityData.rightTangent = kTangentDelta;
    entityData.rightTangentWeight = 0.0f;
    entityData.leftTangentWeight = 0.0f;
    entityData.keyValue = fValue;
    entityData.keyTime = fTime;
    pObj->m_entityAnimData.push_back(entityData);
}


void CExportManager::SolveHierarchy()
{
    for (TObjectMap::iterator it = m_objectMap.begin(); it != m_objectMap.end(); ++it)
    {
        CBaseObject* pObj = it->first;
        int index = it->second;
        if (pObj && pObj->GetParent())
        {
            CBaseObject* pParent = pObj->GetParent();
            TObjectMap::iterator itFind = m_objectMap.find(pParent);
            if (itFind != m_objectMap.end())
            {
                int indexOfParent = itFind->second;
                if (indexOfParent >= 0 && index >= 0)
                {
                    m_data.m_objects[index]->nParent = indexOfParent;
                }
            }
        }
    }

    m_objectMap.clear();
}

bool CExportManager::ShowFBXExportDialog()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence)
    {
        return false;
    }

    CFBXExporterDialog fpsDialog;

    CTrackViewNode* pivotObjectNode = pSequence->GetFirstSelectedNode();

    if (pivotObjectNode && !pivotObjectNode->IsGroupNode())
    {
        m_pivotEntityObject = static_cast<CEntityObject*>(GetIEditor()->GetObjectManager()->FindObject(pivotObjectNode->GetName()));

        if (m_pivotEntityObject)
        {
            fpsDialog.SetExportLocalCoordsCheckBoxEnable(true);
        }
    }

    if (fpsDialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    SetFBXExportSettings(fpsDialog.GetExportCoordsLocalToTheSelectedObject(), fpsDialog.GetExportOnlyPrimaryCamera(), fpsDialog.GetFPS());

    return true;
}

bool CExportManager::ProcessObjectsForExport()
{
    Export::CObject* pObj = new Export::CObject(kPrimaryCameraName.toUtf8().data());
    pObj->entityType = Export::eCamera;
    m_data.m_objects.push_back(pObj);

    float fpsTimeInterval = 1.0f / m_FBXBakedExportFPS;
    float timeValue = 0.0f;

    GetIEditor()->GetAnimation()->SetRecording(false);
    GetIEditor()->GetAnimation()->SetPlaying(false);

    CViewport* vp = GetIEditor()->GetViewManager()->GetSelectedViewport();
    if (CRenderViewport* rvp = viewport_cast<CRenderViewport*>(vp))
    {
        rvp->SetSequenceCamera();
    }

    int startFrame = 0;
    timeValue = startFrame * fpsTimeInterval;

    for (int frameID = startFrame; frameID <= m_numberOfExportFrames; ++frameID)
    {
        GetIEditor()->GetAnimation()->SetTime(timeValue);

        for (size_t objectID = 0; objectID < m_data.m_objects.size(); ++objectID)
        {
            Export::CObject* pObj2 =  m_data.m_objects[objectID];
            CBaseObject* pObject = 0;

            if (QString::compare(pObj2->name, kPrimaryCameraName) == 0)
            {
                pObject = GetIEditor()->GetObjectManager()->FindObject(GetIEditor()->GetViewManager()->GetCameraObjectId());
            }
            else
            {
                if (m_bExportOnlyPrimaryCamera && pObj2->entityType != Export::eCameraTarget)
                {
                    continue;
                }

                pObject = pObj2->GetLastObjectPtr();
            }

            if (!pObject)
            {
                continue;
            }

            Quat rotation(pObject->GetRotation());

            if (pObject->GetParent())
            {
                CBaseObject* pParentObject = pObject->GetParent();
                Quat parentWorldRotation;

                const Vec3& parentScale = pParentObject->GetScale();
                float threshold = 0.0003f;

                bool bParentScaled = false;

                if ((fabsf(parentScale.x - 1.0f) + fabsf(parentScale.y - 1.0f) + fabsf(parentScale.z - 1.0f)) >= threshold)
                {
                    bParentScaled = true;
                }

                if (bParentScaled)
                {
                    Matrix34 tm = pParentObject->GetWorldTM();
                    tm.OrthonormalizeFast();
                    parentWorldRotation = Quat(tm);
                }
                else
                {
                    parentWorldRotation = Quat(pParentObject->GetWorldTM());
                }

                rotation = parentWorldRotation * rotation;
            }

            Vec3 objectPos = pObject->GetWorldPos();

            if (m_bExportLocalCoords && m_pivotEntityObject && m_pivotEntityObject != pObject)
            {
                Matrix34 currentObjectTM = pObject->GetWorldTM();
                Matrix34 invParentTM = m_pivotEntityObject->GetWorldTM();
                invParentTM.Invert();
                currentObjectTM = invParentTM * currentObjectTM;

                objectPos = currentObjectTM.GetTranslation();
                rotation = Quat(currentObjectTM);
            }

            Ang3 worldAngles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(rotation)));

            Export::EntityAnimData entityData;
            entityData.keyTime = timeValue;
            entityData.leftTangentWeight = 0.0f;
            entityData.rightTangentWeight = 0.0f;
            entityData.leftTangent = 0.0f;
            entityData.rightTangent = 0.0f;

            entityData.keyValue = objectPos.x;
            entityData.keyValue *= 100.0f;
            entityData.dataType = (Export::AnimParamType)AnimParamType::PositionX;
            pObj2->m_entityAnimData.push_back(entityData);

            entityData.keyValue = objectPos.y;
            entityData.keyValue *= 100.0f;
            entityData.dataType = (Export::AnimParamType)AnimParamType::PositionY;
            pObj2->m_entityAnimData.push_back(entityData);

            entityData.keyValue = objectPos.z;
            entityData.keyValue *= 100.0f;
            entityData.dataType = (Export::AnimParamType)AnimParamType::PositionZ;
            pObj2->m_entityAnimData.push_back(entityData);

            entityData.keyValue = worldAngles.x;
            entityData.dataType = (Export::AnimParamType)AnimParamType::RotationX;
            pObj2->m_entityAnimData.push_back(entityData);

            entityData.keyValue = worldAngles.y;
            entityData.dataType = (Export::AnimParamType)AnimParamType::RotationY;
            pObj2->m_entityAnimData.push_back(entityData);

            entityData.dataType = (Export::AnimParamType)AnimParamType::RotationZ;
            entityData.keyValue = worldAngles.z;
            pObj2->m_entityAnimData.push_back(entityData);
        }

        timeValue += fpsTimeInterval;
    }

    return true;
}

bool CExportManager::IsDuplicateObjectBeingAdded(const QString& newObjectName)
{
    for (int objectID = 0; objectID < m_data.m_objects.size(); ++objectID)
    {
        if (newObjectName.compare(m_data.m_objects[objectID]->name, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }

    return false;
}

QString CExportManager::CleanXMLText(const QString& text)
{
    QString outText(text);
    outText.replace("\\", "_");
    outText.replace("/", "_");
    outText.replace(" ", "_");
    outText.replace(":", "-");
    outText.replace(";", "-");
    return outText;
}

void CExportManager::FillAnimTimeNode(XmlNodeRef writeNode, CTrackViewAnimNode* pObjectNode, [[maybe_unused]] CTrackViewSequence* currentSequence)
{
    if (!writeNode)
    {
        return;
    }

    CTrackViewTrackBundle allTracks = pObjectNode->GetAllTracks();
    const unsigned int numAllTracks = allTracks.GetCount();
    bool bCreatedSubNodes = false;

    if (numAllTracks > 0)
    {
        XmlNodeRef objNode = writeNode->createNode(CleanXMLText(pObjectNode->GetName()).toUtf8().data());
        writeNode->setAttr("time", m_animTimeExportPrimarySequenceCurrentTime);

        for (unsigned int trackID = 0; trackID < numAllTracks; ++trackID)
        {
            CTrackViewTrack* childTrack = allTracks.GetTrack(trackID);

            AnimParamType trackType = childTrack->GetParameterType().GetType();

            if (trackType == AnimParamType::Animation || trackType == AnimParamType::Sound)
            {
                QString childName = CleanXMLText(childTrack->GetName());

                if (childName.isEmpty())
                {
                    continue;
                }

                XmlNodeRef subNode = objNode->createNode(childName.toUtf8().data());
                CTrackViewKeyBundle keyBundle = childTrack->GetAllKeys();
                uint keysNumber = keyBundle.GetKeyCount();

                for (uint keyID = 0; keyID < keysNumber; ++keyID)
                {
                    const CTrackViewKeyHandle& keyHandle = keyBundle.GetKey(keyID);

                    QString keyContentName;
                    float keyStartTime = 0.0f;
                    float keyEndTime = 0.0f;
                    ;
                    float keyTime = 0.0f;
                    float keyDuration = 0.0f;

                    if (trackType == AnimParamType::Animation)
                    {
                        if (!m_animKeyTimeExport)
                        {
                            continue;
                        }

                        ICharacterKey animationKey;
                        keyHandle.GetKey(&animationKey);
                        keyStartTime = animationKey.m_startTime;
                        keyEndTime = animationKey.m_endTime;
                        keyTime = animationKey.time;
                        keyContentName = CleanXMLText(animationKey.m_animation.c_str());
                        keyDuration = animationKey.GetActualDuration();
                    }
                    else if (trackType == AnimParamType::Sound)
                    {
                        if (!m_soundKeyTimeExport)
                        {
                            continue;
                        }

                        ISoundKey soundKey;
                        keyHandle.GetKey(&soundKey);
                        keyTime = soundKey.time;
                        keyContentName = CleanXMLText(soundKey.sStartTrigger.c_str());
                        keyDuration = soundKey.fDuration;
                    }

                    if (keyContentName.isEmpty())
                    {
                        continue;
                    }

                    XmlNodeRef keyNode = subNode->createNode(keyContentName.toUtf8().data());

                    float keyGlobalTime = m_animTimeExportPrimarySequenceCurrentTime + keyTime;
                    keyNode->setAttr("keyTime", keyGlobalTime);

                    if (keyStartTime > 0)
                    {
                        keyNode->setAttr("startTime", keyStartTime);
                    }

                    if (keyEndTime > 0)
                    {
                        keyNode->setAttr("endTime", keyEndTime);
                    }

                    if (keyDuration > 0)
                    {
                        keyNode->setAttr("duration", keyDuration);
                    }

                    subNode->addChild(keyNode);
                    objNode->addChild(subNode);
                    bCreatedSubNodes = true;
                }
            }
        }

        if (bCreatedSubNodes)
        {
            writeNode->addChild(objNode);
        }
    }
}


bool CExportManager::AddObjectsFromSequence(CTrackViewSequence* pSequence, XmlNodeRef seqNode)
{
    CTrackViewAnimNodeBundle allNodes = pSequence->GetAllAnimNodes();
    const unsigned int numAllNodes = allNodes.GetCount();

    for (unsigned int nodeID = 0; nodeID < numAllNodes; ++nodeID)
    {
        CTrackViewAnimNode* pAnimNode = allNodes.GetNode(nodeID);

        if (seqNode && pAnimNode)
        {
            FillAnimTimeNode(seqNode, pAnimNode, pSequence);
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationBus::Events::FindEntity, pAnimNode->GetAzEntityId());

        if (entity)
        {
            QString addObjectName = entity->GetName().c_str();

            if (IsDuplicateObjectBeingAdded(addObjectName))
            {
                continue;
            }

            Export::CObject* pObj = new Export::CObject(entity->GetName().c_str());

            pObj->m_entityAnimData.reserve(m_numberOfExportFrames * kReserveCount);
            m_data.m_objects.push_back(pObj);
        }
    }

    CTrackViewTrackBundle trackBundle = pSequence->GetTracksByParam(AnimParamType::Sequence);

    const uint numSequenceTracks = trackBundle.GetCount();
    for (uint i = 0; i < numSequenceTracks; ++i)
    {
        CTrackViewTrack* pSequenceTrack = trackBundle.GetTrack(i);
        if (pSequenceTrack->IsDisabled())
        {
            continue;
        }

        const uint numKeys = pSequenceTrack->GetKeyCount();
        for (int keyIndex = 0; keyIndex < numKeys; ++keyIndex)
        {
            const CTrackViewKeyHandle& keyHandle = pSequenceTrack->GetKey(keyIndex);
            ISequenceKey sequenceKey;
            keyHandle.GetKey(&sequenceKey);

            CTrackViewSequence* pSubSequence = CDirectorNodeAnimator::GetSequenceFromSequenceKey(sequenceKey);

            if (pSubSequence)
            {
                if (pSubSequence && !pSubSequence->IsDisabled())
                {
                    XmlNodeRef subSeqNode = 0;

                    if (!seqNode)
                    {
                        AddObjectsFromSequence(pSubSequence);
                    }
                    else
                    {
                        // In case of exporting animation/sound times data
                        const QString sequenceName = pSubSequence->GetName();
                        XmlNodeRef subSeqNode2 = seqNode->createNode(sequenceName.toUtf8().data());

                        if (sequenceName == m_animTimeExportPrimarySequenceName)
                        {
                            m_animTimeExportPrimarySequenceCurrentTime = sequenceKey.time;
                        }
                        else
                        {
                            m_animTimeExportPrimarySequenceCurrentTime += sequenceKey.time;
                        }

                        AddObjectsFromSequence(pSubSequence, subSeqNode2);
                        seqNode->addChild(subSeqNode2);
                    }
                }
            }
        }
    }

    return false;
}

bool CExportManager::AddSelectedEntityObjects()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return false;
    }

    CTrackViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();
    const unsigned int numSelectedNodes = selectedNodes.GetCount();

    for (unsigned int nodeNumber = 0; nodeNumber < numSelectedNodes; ++nodeNumber)
    {
        if (m_bAnimationExport)
        {
            CTrackViewAnimNode* pAnimNode = selectedNodes.GetNode(nodeNumber);
            AddEntityAnimationData(pAnimNode->GetAzEntityId());
        }
    }

    return true;
}

bool CExportManager::AddSelectedRegionObjects()
{
    AABB box;
    GetIEditor()->GetSelectedRegion(box);
    if (box.IsEmpty())
    {
        return false;
    }

    std::vector<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->FindObjectsInAABB(box, objects);

    int numObjects = objects.size();
    if (numObjects > m_data.m_objects.size())
    {
        m_data.m_objects.reserve(numObjects + 1); // +1 for terrain
    }
    // First run pipeline to precache geometry
    m_isPrecaching = true;
    for (size_t i = 0; i < numObjects; ++i)
    {
        AddObject(objects[i]);
    }

    // Repeat pipeline to collect geometry
    m_isPrecaching = false;
    for (size_t i = 0; i < numObjects; ++i)
    {
        AddObject(objects[i]);
    }

    return true;
}

bool CExportManager::ExportToFile(const char* filename, bool bClearDataAfterExport)
{
    bool bRet = false;
    QString ext = PathUtil::GetExt(filename);

    if (m_data.GetObjectCount() == 0)
    {
        QMessageBox::warning(QApplication::activeWindow(), QString(), QObject::tr("Track View selection does not exist as an object."));
        return false;
    }

    for (int i = 0; i < m_exporters.size(); ++i)
    {
        IExporter* pExporter = m_exporters[i];
        if (!QString::compare(ext, pExporter->GetExtension(), Qt::CaseInsensitive))
        {
            bRet = pExporter->ExportToFile(filename, &m_data);
            break;
        }
    }

    if (bClearDataAfterExport)
    {
        m_data.Clear();
    }
    return bRet;
}


bool CExportManager::Export(const char* defaultName, const char* defaultExt, const char* defaultPath, [[maybe_unused]] bool isSelectedObjects, bool isSelectedRegionObjects, bool isOccluder, bool bAnimationExport)
{
    m_bAnimationExport = bAnimationExport;

    m_isOccluder    =   isOccluder;
    const float OldScale    =   m_fScale;
    if (isOccluder)
    {
        m_fScale    =   1.f;
    }

    m_data.Clear();
    m_objectMap.clear();

    QString filters;
    for (TExporters::iterator ppExporter = m_exporters.begin(); ppExporter != m_exporters.end(); ++ppExporter)
    {
        IExporter* pExporter = (*ppExporter);
        if (pExporter)
        {
            const QString ext = pExporter->GetExtension();
            const QString newFilter = QString("%1 (*.%2)").arg(pExporter->GetShortDescription(), ext);
            if (filters.isEmpty())
            {
                filters = newFilter;
            }
            else if (ext == defaultExt) // the default extension should be the first item in the list, so it's the default export options
            {
                filters = newFilter + ";;" + filters;
            }
            else
            {
                filters = filters + ";;" + newFilter;
            }
        }
    }
    filters += ";;All files (*)";

    bool returnRes = false;

    QString newFilename(defaultName);
    if (m_bAnimationExport || CFileUtil::SelectSaveFile(filters, defaultExt, defaultPath, newFilename))
    {
        WaitCursor wait;
        if (isSelectedRegionObjects)
        {
            AddSelectedRegionObjects();
        }

        if (!bAnimationExport)
        {
            SolveHierarchy();
        }

        if (m_bAnimationExport)
        {
            CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

            if (pSequence)
            {
                // Save To FBX custom selected nodes
                if (!m_bBakedKeysSequenceExport)
                {
                    returnRes = AddSelectedEntityObjects();
                }
                else
                {
                    // Export the whole sequence with baked keys
                    if (ShowFBXExportDialog())
                    {
                        m_numberOfExportFrames = pSequence->GetTimeRange().end * m_FBXBakedExportFPS;

                        if (!m_bExportOnlyPrimaryCamera)
                        {
                            AddObjectsFromSequence(pSequence);
                        }

                        returnRes = ProcessObjectsForExport();
                        SolveHierarchy();
                    }
                }
            }

            if (returnRes)
            {
                returnRes = ExportToFile(defaultName);
            }
        }
        else
        {
            returnRes = ExportToFile(newFilename.toStdString().c_str());
        }
    }

    m_fScale = OldScale;
    m_bBakedKeysSequenceExport = true;
    m_FBXBakedExportFPS = 0.0f;

    return returnRes;
}

void CExportManager::SetFBXExportSettings(bool bLocalCoordsToSelectedObject, bool bExportOnlyPrimaryCamera, const float fps)
{
    m_bExportLocalCoords = bLocalCoordsToSelectedObject;
    m_bExportOnlyPrimaryCamera = bExportOnlyPrimaryCamera;
    m_FBXBakedExportFPS = fps;
}

Export::Object* Export::CData::AddObject(const char* objectName)
{
    for (size_t objectID = 0; objectID < m_objects.size(); ++objectID)
    {
        if (strcmp(m_objects[objectID]->name, objectName) == 0)
        {
            return m_objects[objectID];
        }
    }

    CObject* pObj = new CObject(objectName);
    m_objects.push_back(pObj);
    return m_objects[m_objects.size() - 1];
}

bool CExportManager::ImportFromFile(const char* filename)
{
    bool bRet = false;
    QString ext = PathUtil::GetExt(filename);

    m_data.Clear();

    for (size_t handlerID = 0; handlerID < m_exporters.size(); ++handlerID)
    {
        IExporter* pExporter = m_exporters[handlerID];
        if (!QString::compare(ext, pExporter->GetExtension(), Qt::CaseInsensitive))
        {
            bRet = pExporter->ImportFromFile(filename, &m_data);
            break;
        }
    }

    return bRet;
}

bool CExportManager::ExportSingleStatObj(IStatObj* pStatObj, const char* filename)
{
    Export::CObject* pObj = new Export::CObject(Path::GetFileName(filename).toUtf8().data());
    AddStatObj(pObj, pStatObj);
    m_data.m_objects.push_back(pObj);
    ExportToFile(filename, true);
    return true;
}

void CExportManager::SaveNodeKeysTimeToXML()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    CTrackViewExportKeyTimeDlg exportDialog;

    if (exportDialog.exec() == QDialog::Accepted)
    {
        m_animKeyTimeExport = exportDialog.IsAnimationExportChecked();
        m_soundKeyTimeExport = exportDialog.IsSoundExportChecked();

        QString filters = "All files (*.xml)";
        QString defaultName = QString(pSequence->GetName()) + ".xml";

        QtUtil::QtMFCScopedHWNDCapture cap;
        CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, QFileDialog::AnyFile, "xml", defaultName, filters, {}, {}, cap);
        if (dlg.exec())
        {
            m_animTimeNode = XmlHelpers::CreateXmlNode(pSequence->GetName());
            m_animTimeExportPrimarySequenceName = pSequence->GetName();

            m_data.Clear();
            m_animTimeExportPrimarySequenceCurrentTime = 0.0;

            AddObjectsFromSequence(pSequence, m_animTimeNode);

            m_animTimeNode->saveToFile(dlg.selectedFiles().constFirst().toStdString().c_str());
            QMessageBox::information(QApplication::activeWindow(), QString(), QObject::tr("Export Finished"));
        }
    }
}
