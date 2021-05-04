
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

#include "EditorDefs.h"

#include "ModelViewport.h"

// Qt
#include <QMessageBox>
#include <QSettings>

// CryCommon
#include <CryCommon/IViewSystem.h>

#include "CryPhysicsDeprecation.h"


// Editor
#include "ThumbnailGenerator.h"         // for CThumbnailGenerator
#include "FileTypeUtils.h"              // for IsPreviewableFileType
#include "ErrorRecorder.h"





uint32 g_ypos = 0;

#define SKYBOX_NAME "InfoRedGal"

/////////////////////////////////////////////////////////////////////////////
// CModelViewport

CModelViewport::CModelViewport(const char* settingsPath, QWidget* parent)
    : CRenderViewport(tr("Model View"), parent)
{
    m_settingsPath = QString::fromLatin1(settingsPath);

    m_bPaused = false;

    m_Camera.SetFrustum(800, 600, 3.14f / 4.0f, 0.02f, 10000);

    m_bInRotateMode = false;
    m_bInMoveMode = false;

    m_object = 0;


    m_weaponModel = 0;

    m_camRadius = 10;

    m_moveSpeed = 0.1f;
    m_LightRotationRadian = 0.0f;

    m_weaponIK = false;

    m_pRESky = 0;
    m_pSkyboxName = 0;
    m_pSkyBoxShader = NULL;

    m_attachBone = QStringLiteral("weapon_bone");

    // Init variable.
    mv_objectAmbientColor = Vec3(0.25f, 0.25f, 0.25f);
    mv_backgroundColor      = Vec3(0.25f, 0.25f, 0.25f);

    mv_lightDiffuseColor = Vec3(0.70f, 0.70f, 0.70f);
    mv_lightMultiplier = 3.0f;
    mv_lightOrbit = 15.0f;
    mv_lightRadius = 400.0f;
    mv_lightSpecMultiplier = 1.0f;

    mv_showPhysics = false;

    m_GridOrigin = Vec3(ZERO);
    m_arrAnimatedCharacterPath.resize(0x200, ZERO);
    m_arrSmoothEntityPath.resize(0x200, ZERO);

    m_arrRunStrafeSmoothing.resize(0x100);
    SetPlayerPos();

    // cache all the variable callbacks, must match order of enum defined in header
    m_onSetCallbacksCache.push_back(AZStd::bind(&CModelViewport::OnCharPhysics, this, AZStd::placeholders::_1));
    m_onSetCallbacksCache.push_back(AZStd::bind(&CModelViewport::OnLightColor, this, AZStd::placeholders::_1));
    m_onSetCallbacksCache.push_back(AZStd::bind(&CModelViewport::OnLightMultiplier, this, AZStd::placeholders::_1));
    m_onSetCallbacksCache.push_back(AZStd::bind(&CModelViewport::OnShowShaders, this, AZStd::placeholders::_1));

    //--------------------------------------------------
    // Register variables.
    //--------------------------------------------------
    m_vars.AddVariable(mv_showPhysics, "Display Physics");
    m_vars.AddVariable(mv_useCharPhysics, "Use Character Physics", &m_onSetCallbacksCache[VariableCallbackIndex::OnCharPhysics]);
    mv_useCharPhysics = true;
    m_vars.AddVariable(mv_showGrid, "ShowGrid");
    mv_showGrid = true;
    m_vars.AddVariable(mv_showBase, "ShowBase");
    mv_showBase = false;
    m_vars.AddVariable(mv_showLocator, "ShowLocator");
    mv_showLocator = 0;
    m_vars.AddVariable(mv_InPlaceMovement, "InPlaceMovement");
    mv_InPlaceMovement = false;
    m_vars.AddVariable(mv_StrafingControl, "StrafingControl");
    mv_StrafingControl = false;

    m_vars.AddVariable(mv_lighting, "Lighting");
    mv_lighting = true;
    m_vars.AddVariable(mv_animateLights, "AnimLights");

    m_vars.AddVariable(mv_backgroundColor, "BackgroundColor", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightColor], IVariable::DT_COLOR);
    m_vars.AddVariable(mv_objectAmbientColor, "ObjectAmbient", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightColor], IVariable::DT_COLOR);

    m_vars.AddVariable(mv_lightDiffuseColor, "LightDiffuse", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightColor], IVariable::DT_COLOR);
    m_vars.AddVariable(mv_lightMultiplier, "Light Multiplier", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightMultiplier], IVariable::DT_SIMPLE);
    m_vars.AddVariable(mv_lightSpecMultiplier, "Light Specular Multiplier", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightMultiplier], IVariable::DT_SIMPLE);
    m_vars.AddVariable(mv_lightRadius, "Light Radius", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightMultiplier], IVariable::DT_SIMPLE);
    m_vars.AddVariable(mv_lightOrbit, "Light Orbit", &m_onSetCallbacksCache[VariableCallbackIndex::OnLightMultiplier], IVariable::DT_SIMPLE);

    m_vars.AddVariable(mv_showWireframe1, "ShowWireframe1");
    m_vars.AddVariable(mv_showWireframe2, "ShowWireframe2");
    m_vars.AddVariable(mv_showTangents, "ShowTangents");
    m_vars.AddVariable(mv_showBinormals, "ShowBinormals");
    m_vars.AddVariable(mv_showNormals, "ShowNormals");

    m_vars.AddVariable(mv_showSkeleton, "ShowSkeleton");
    m_vars.AddVariable(mv_showJointNames, "ShowJointNames");
    m_vars.AddVariable(mv_showJointsValues, "ShowJointsValues");
    m_vars.AddVariable(mv_showStartLocation, "ShowInvStartLocation");
    m_vars.AddVariable(mv_showMotionParam, "ShowMotionParam");
    m_vars.AddVariable(mv_printDebugText, "PrintDebugText");

    m_vars.AddVariable(mv_UniformScaling, "UniformScaling");
    mv_UniformScaling = 1.0f;
    mv_UniformScaling.SetLimits(0.01f, 2.0f);
    m_vars.AddVariable(mv_forceLODNum, "ForceLODNum");
    mv_forceLODNum = 0;
    mv_forceLODNum.SetLimits(0, 10);
    m_vars.AddVariable(mv_showShaders, "ShowShaders", &m_onSetCallbacksCache[VariableCallbackIndex::OnShowShaders]);
    m_vars.AddVariable(mv_AttachCamera, "AttachCamera");

    m_vars.AddVariable(mv_fov, "FOV");
    mv_fov = 60;
    mv_fov.SetLimits(1, 120);

    RestoreDebugOptions();

    m_camRadius = 10;

    //YPR_Angle =   Ang3(0,-1.0f,0);
    //SetViewTM( Matrix34(CCamera::CreateOrientationYPR(YPR_Angle), Vec3(0,-m_camRadius,0))  );
    Vec3 camPos = Vec3(10, 10, 10);
    Matrix34 tm = Matrix33::CreateRotationVDir((Vec3(0, 0, 0) - camPos).GetNormalized());
    tm.SetTranslation(camPos);
    SetViewTM(tm);


    m_AABB.Reset();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SaveDebugOptions() const
{
    QSettings settings;
    for (auto g : m_settingsPath.split('\\'))
        settings.beginGroup(g);

    CVarBlock* vb = GetVarObject()->GetVarBlock();
    int32 vbCount = vb->GetNumVariables();

    settings.setValue("iDebugOptionCount", vbCount);

    char keyType[64], keyValue[64];
    for (int32 i = 0; i < vbCount; ++i)
    {
        IVariable* var = vb->GetVariable(i);
        IVariable::EType vType = var->GetType();
        sprintf_s(keyType, "DebugOption_%s_type", var->GetName().toUtf8().data());
        sprintf_s(keyValue, "DebugOption_%s_value", var->GetName().toUtf8().data());
        switch (vType)
        {
        case IVariable::UNKNOWN:
        {
            break;
        }
        case IVariable::INT:
        {
            int32 value = 0;
            var->Get(value);
            settings.setValue(keyType, IVariable::INT);
            settings.setValue(keyValue, value);

            break;
        }
        case IVariable::BOOL:
        {
            bool value = 0;
            var->Get(value);
            settings.setValue(keyType, IVariable::BOOL);
            settings.setValue(keyValue, value);
            break;
        }
        case IVariable::FLOAT:
        {
            f32 value = 0;
            var->Get(value);
            settings.setValue(keyType, IVariable::FLOAT);
            settings.setValue(keyValue, value);
            break;
        }
        case IVariable::VECTOR:
        {
            Vec3 value;
            var->Get(value);
            f32 valueArray[3];
            valueArray[0] = value.x;
            valueArray[1] = value.y;
            valueArray[2] = value.z;
            settings.setValue(keyType, IVariable::VECTOR);
            settings.setValue(keyValue, QByteArray(reinterpret_cast<const char*>(&value), 3 * sizeof(f32)));

            break;
        }
        case IVariable::QUAT:
        {
            Quat value;
            var->Get(value);
            f32 valueArray[4];
            valueArray[0] = value.w;
            valueArray[1] = value.v.x;
            valueArray[2] = value.v.y;
            valueArray[3] = value.v.z;

            settings.setValue(keyType, IVariable::QUAT);
            settings.setValue(keyValue, QByteArray(reinterpret_cast<const char*>(&value), 4 * sizeof(f32)));

            break;
        }
        case IVariable::STRING:
        {
            QString value;
            var->Get(value);
            settings.setValue(keyType, IVariable::STRING);
            settings.setValue(keyValue, value);

            break;
        }
        case IVariable::ARRAY:
        {
            break;
        }
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::RestoreDebugOptions()
{
    QSettings settings;
    for (auto g : m_settingsPath.split('\\'))
        settings.beginGroup(g);

    QString strRead = "";
    int32 iRead = 0;
    BOOL bRead = FALSE;
    f32 fRead = .0f;
    QByteArray pbtData;

    CVarBlock* vb = m_vars.GetVarBlock();
    int32 vbCount = vb->GetNumVariables();

    char keyType[64], keyValue[64];
    for (int32 i = 0; i < vbCount; ++i)
    {
        IVariable* var = vb->GetVariable(i);
        sprintf_s(keyType, "DebugOption_%s_type", var->GetName().toUtf8().data());

        int32 iType = settings.value(keyType, 0).toInt();

        sprintf_s(keyValue, "DebugOption_%s_value", var->GetName().toUtf8().data());
        switch (iType)
        {
        case IVariable::UNKNOWN:
        {
            break;
        }
        case IVariable::INT:
        {
            iRead = settings.value(keyValue, 0).toInt();
            var->Set(iRead);
            break;
        }
        case IVariable::BOOL:
        {
            bRead = settings.value(keyValue, FALSE).toBool();
            var->Set(bRead);
            break;
        }
        case IVariable::FLOAT:
        {
            fRead = settings.value(keyValue).toDouble();
            var->Set(fRead);
            break;
        }
        case IVariable::VECTOR:
        {
            pbtData = settings.value(keyValue).toByteArray();
            assert(pbtData.count() == 3 * sizeof(f32));
            f32* pfRead = reinterpret_cast<f32*>(pbtData.data());

            Vec3 vecRead(pfRead[0], pfRead[1], pfRead[2]);
            var->Set(vecRead);
            break;
        }
        case IVariable::QUAT:
        {
            pbtData = settings.value(keyValue).toByteArray();
            assert(pbtData.count() == 4 * sizeof(f32));
            f32* pfRead = reinterpret_cast<f32*>(pbtData.data());

            Quat valueRead(pfRead[0], pfRead[1], pfRead[2], pfRead[3]);
            var->Set(valueRead);
            break;
        }
        case IVariable::STRING:
        {
            strRead = settings.value(keyValue, "").toString();
            var->Set(strRead);
            break;
        }
        case IVariable::ARRAY:
        {
            break;
        }
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CModelViewport::~CModelViewport()
{
    OnDestroy();
    ReleaseObject();

    GetIEditor()->FlushUndo();

    SaveDebugOptions();

    // helper offset??
    CRY_PHYSICS_REPLACEMENT_ASSERT();
    GetIEditor()->SetConsoleVar("ca_UsePhysics", 1);
}

/////////////////////////////////////////////////////////////////////////////
// CModelViewport message handlers
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CModelViewport::ReleaseObject()
{
    if (m_object)
    {
        m_object->Release();
        m_object = NULL;
    }

    if (m_weaponModel)
    {
        m_weaponModel->Release();
        m_weaponModel = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::LoadObject(const QString& fileName, [[maybe_unused]] float scale)
{
    m_bPaused = false;

    // Load object.
    QString file = Path::MakeGamePath(fileName);

    bool reload = false;
    if (m_loadedFile == file)
    {
        reload = true;
    }
    m_loadedFile = file;

    SetName(tr("Model View - %1").arg(file));

    ReleaseObject();

    // Enables display of warning after model have been loaded.
    CErrorsRecorder errRecorder;

    if (IsPreviewableFileType(file.toUtf8().data()))
    {
        const QString fileExt = QFileInfo(file).completeSuffix();
        // Try Load character.
        const bool isSKEL = (0 == fileExt.compare(CRY_SKEL_FILE_EXT, Qt::CaseInsensitive));
        const bool isSKIN = (0 == fileExt.compare(CRY_SKIN_FILE_EXT, Qt::CaseInsensitive));
        const bool isCGA = (0 == fileExt.compare(CRY_ANIM_GEOMETRY_FILE_EXT, Qt::CaseInsensitive));
        const bool isCDF = (0 == fileExt.compare(CRY_CHARACTER_DEFINITION_FILE_EXT, Qt::CaseInsensitive));
        if (isSKEL || isSKIN || isCGA || isCDF)
        {
        }
        else
        {
            LoadStaticObject(file);
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Preview Error"), tr("Preview of this file type not supported."));
        return;
    }

    //--------------------------------------------------------------------------------

    if (!reload)
    {
        Vec3 v = m_AABB.max - m_AABB.min;
        float radius = v.GetLength() / 2.0f;
        m_camRadius = radius * 2;
    }

    if (GetIEditor()->IsInPreviewMode())
    {
        Physicalize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::LoadStaticObject(const QString& file)
{
    if (m_object)
    {
        m_object->Release();
    }

    // Load Static object.
    m_object = m_engine->LoadStatObjUnsafeManualRef(file.toUtf8().data(), 0, 0, false);

    if (!m_object)
    {
        CLogFile::WriteLine("Loading of object failed.");
        return;
    }
    m_object->AddRef();

    // Generate thumbnail for this cgf.
    CThumbnailGenerator thumbGen;
    thumbGen.GenerateForFile(file);

    m_AABB.min = m_object->GetBoxMin();
    m_AABB.max = m_object->GetBoxMax();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnRender()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    const QRect rc = contentsRect();
    ProcessKeys();
    if (m_renderer)
    {
        PreWidgetRendering();

        m_Camera.SetFrustum(m_Camera.GetViewSurfaceX(), m_Camera.GetViewSurfaceZ(), m_Camera.GetFov(), 0.02f, 10000, m_Camera.GetPixelAspectRatio());
        const int w = rc.width();
        const int h = rc.height();
        m_Camera.SetFrustum(w, h, DEG2RAD(mv_fov), 0.0101f, 10000.0f);

        if (GetIEditor()->IsInPreviewMode())
        {
            GetISystem()->SetViewCamera(m_Camera);
        }

        Vec3 clearColor = mv_backgroundColor;
        m_renderer->SetClearColor(clearColor);
        m_renderer->SetCamera(m_Camera);

        auto colorf = ColorF(clearColor, 1.0f);
        m_renderer->ClearTargetsImmediately(FRT_CLEAR | FRT_CLEAR_IMMEDIATE, colorf);
        m_renderer->ResetToDefault();

        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_Camera, SRenderingPassInfo::DEFAULT_FLAGS, true);

        {
            CScopedWireFrameMode scopedWireFrame(m_renderer, mv_showWireframe1 ? R_WIREFRAME_MODE : R_SOLID_MODE);
            DrawModel(passInfo);
        }

        PostWidgetRendering();
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawSkyBox(const SRenderingPassInfo& passInfo)
{
    CRenderObject* pObj = m_renderer->EF_GetObject_Temp(passInfo.ThreadID());
    pObj->m_II.m_Matrix.SetTranslationMat(GetViewTM().GetTranslation());

    if (m_pSkyboxName)
    {
        SShaderItem skyBoxShaderItem(m_pSkyBoxShader);
        m_renderer->EF_AddEf(m_pRESky, skyBoxShaderItem, pObj, passInfo, EFSLIST_GENERAL, 1, SRendItemSorter::CreateRendItemSorter(passInfo));
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimBack()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimFastBack()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimFastForward()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimFront()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimPlay()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    // TODO: Add your message handler code here and/or call default

    CRenderViewport::mouseDoubleClickEvent(event);
    if (event->button() != Qt::LeftButton)
    {
        return;
    }
    Matrix34 tm;
    tm.SetIdentity();
    SetViewTM(tm);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnLightColor([[maybe_unused]] IVariable* var)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowNormals([[maybe_unused]] IVariable* var)
{
    bool enable = mv_showNormals;
    GetIEditor()->SetConsoleVar("r_ShowNormals", (enable) ? 1 : 0);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowTangents([[maybe_unused]] IVariable* var)
{
    bool enable = mv_showTangents;
    GetIEditor()->SetConsoleVar("r_ShowTangents", (enable) ? 1 : 0);
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnCharPhysics([[maybe_unused]] IVariable* var)
{
    bool enable = mv_useCharPhysics;
    GetIEditor()->SetConsoleVar("ca_UsePhysics", enable);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowShaders([[maybe_unused]] IVariable* var)
{
    bool bEnable = mv_showShaders;
    GetIEditor()->SetConsoleVar("r_ProfileShaders", bEnable);
}

void CModelViewport::OnDestroy()
{
    ReleaseObject();
    if (m_pRESky)
    {
        m_pRESky->Release(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnActivate()
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnDeactivate()
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Update()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    CRenderViewport::Update();

    DrawInfo();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawInfo() const
{
    if (GetIEditor()->Get3DEngine())
    {
        ICVar* pDisplayInfo = gEnv->pConsole->GetCVar("r_DisplayInfo");
        if (pDisplayInfo && pDisplayInfo->GetIVal() != 0)
        {
            const float fps = gEnv->pTimer->GetFrameRate();
            const float x = (float)gEnv->pRenderer->GetWidth() - 5.0f;

            gEnv->p3DEngine->DrawTextRightAligned(x, 1, "FPS: %.2f", fps);

            int nPolygons, nShadowVolPolys;
            gEnv->pRenderer->GetPolyCount(nPolygons, nShadowVolPolys);
            int nDrawCalls = gEnv->pRenderer->GetCurrentNumberOfDrawCalls();
            gEnv->p3DEngine->DrawTextRightAligned(x, 20, "Tris:%2d,%03d - DP:%d", nPolygons / 1000, nPolygons % 1000, nDrawCalls);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CModelViewport::CanDrop([[maybe_unused]] const QPoint& point, IDataBaseItem* pItem)
{
    if (!pItem)
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Drop([[maybe_unused]] const QPoint& point, [[maybe_unused]] IDataBaseItem* pItem)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Physicalize()
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::RePhysicalize()
{
    Physicalize();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetPaused(bool bPaused)
{
    //return;
    if (m_bPaused != bPaused)
    {
        m_bPaused = bPaused;
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawModel(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    m_vCamPos = GetCamera().GetPosition();
    const QRect rc = contentsRect();
    //GetISystem()->SetViewCamera( m_Camera );
    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    m_renderer->BeginSpawningGeneratingRendItemJobs(passInfo.ThreadID());
    m_renderer->BeginSpawningShadowGeneratingRendItemJobs(passInfo.ThreadID());
    m_renderer->EF_ClearSkinningDataPool();
    m_renderer->EF_StartEf(passInfo);

    //////////////////////////////////////////////////////////////////////////
    // Draw lights.
    //////////////////////////////////////////////////////////////////////////
    if (mv_lighting == true)
    {
        pAuxGeom->DrawSphere(m_VPLight.m_Origin, 0.2f, ColorB(255, 255, 0, 255));
    }

    gEnv->pConsole->GetCVar("ca_DrawWireframe")->Set(mv_showWireframe2);
    gEnv->pConsole->GetCVar("ca_DrawTangents")->Set(mv_showTangents);
    gEnv->pConsole->GetCVar("ca_DrawBinormals")->Set(mv_showBinormals);
    gEnv->pConsole->GetCVar("ca_DrawNormals")->Set(mv_showNormals);

    DrawLights(passInfo);


    //-----------------------------------------------------------------------------
    //-----            Render Static Object (handled by 3DEngine)              ----
    //-----------------------------------------------------------------------------
    // calculate LOD

    f32 fDistance = GetViewTM().GetTranslation().GetLength();
    SRendParams rp;
    rp.fDistance    = fDistance;

    Matrix34 tm;
    tm.SetIdentity();
    rp.pMatrix = &tm;
    rp.pPrevMatrix = &tm;

    Vec3 vAmbient;
    mv_objectAmbientColor.Get(vAmbient);

    rp.AmbientColor.r = vAmbient.x * mv_lightMultiplier;
    rp.AmbientColor.g = vAmbient.y * mv_lightMultiplier;
    rp.AmbientColor.b = vAmbient.z * mv_lightMultiplier;
    rp.AmbientColor.a = 1;

    rp.nDLightMask = 7;
    if (mv_lighting == false)
    {
        rp.nDLightMask = 0;
    }

    rp.dwFObjFlags  = 0;

    //-----------------------------------------------------------------------------
    //-----            Render Static Object (handled by 3DEngine)              ----
    //-----------------------------------------------------------------------------
    if (m_object)
    {
        m_object->Render(rp, passInfo);
        if (mv_showGrid)
        {
            DrawFloorGrid(Quat(IDENTITY), Vec3(ZERO), Matrix33(IDENTITY));
        }

        if (mv_showBase)
        {
            DrawCoordSystem(IDENTITY, 10.0f);
        }
    }

    m_renderer->EF_EndEf3D(SHDF_STREAM_SYNC, -1, -1, passInfo);
}


void CModelViewport::DrawLights(const SRenderingPassInfo& passInfo)
{
    if (mv_animateLights)
    {
        m_LightRotationRadian += m_AverageFrameTime;
    }

    if (m_LightRotationRadian > gf_PI)
    {
        m_LightRotationRadian = -gf_PI;
    }

    Matrix33 LightRot33 =   Matrix33::CreateRotationZ(m_LightRotationRadian);

    Vec3 LPos0 = Vec3(-mv_lightOrbit, mv_lightOrbit, mv_lightOrbit);
    m_VPLight.SetPosition(LightRot33 * LPos0 + m_PhysicalLocation.t);
    Vec3 d = mv_lightDiffuseColor;
    m_VPLight.SetLightColor(ColorF(d.x * mv_lightMultiplier, d.y * mv_lightMultiplier, d.z * mv_lightMultiplier, 0));
    m_VPLight.SetSpecularMult(mv_lightSpecMultiplier);
    m_VPLight.m_fRadius = mv_lightRadius;
    m_VPLight.m_Flags = DLF_SUN | DLF_DIRECTIONAL;

    if (mv_lighting == true)
    {
        m_renderer->EF_ADDDlight(&m_VPLight, passInfo);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::PlayAnimation([[maybe_unused]] const char* szName)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawFloorGrid(const Quat& m33, const Vec3& vPhysicalLocation, const Matrix33& rGridRot)
{
    if (!m_renderer)
    {
        return;
    }

    float XR = 45;
    float YR = 45;





    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);

    Vec3 axis = m33.GetColumn0();

    Matrix33 SlopeMat33 = rGridRot;
    uint32 GroundAlign = 1;
    if (GroundAlign == 0)
    {
        SlopeMat33 = Matrix33::CreateRotationAA(m_absCurrentSlope, axis);
    }


    m_GridOrigin = Vec3(floorf(vPhysicalLocation.x), floorf(vPhysicalLocation.y), vPhysicalLocation.z);


    Matrix33 ScaleMat33 = IDENTITY;
    Vec3 rh = Matrix33::CreateRotationY(m_absCurrentSlope) * Vec3(1.0f, 0.0f, 0.0f);
    if (rh.x)
    {
        Vec3 xback = SlopeMat33.GetRow(0);
        Vec3 yback = SlopeMat33.GetRow(1);
        f32 ratiox = 1.0f / Vec3(xback.x, xback.y, 0.0f).GetLength();
        f32 ratioy = 1.0f / Vec3(yback.x, yback.y, 0.0f).GetLength();

        f32 ratio = 1.0f / rh.x;
        //  Vec3 h=Vec3((m_GridOrigin.x-vPhysicalLocation.x)*ratiox,(m_GridOrigin.y-vPhysicalLocation.y)*ratioy,0.0f);
        Vec3 h = Vec3(m_GridOrigin.x - vPhysicalLocation.x, m_GridOrigin.y - vPhysicalLocation.y, 0.0f);
        Vec3 nh = SlopeMat33 * h;
        m_GridOrigin.z += nh.z * ratio;

        ScaleMat33 = Matrix33::CreateScale(Vec3(ratiox, ratioy, 0.0f));

        //  float color1[4] = {0,1,0,1};
        //  m_renderer->Draw2dLabel(12,g_ypos,1.6f,color1,false,"h: %f %f %f    h.z: %f   ratio: %f  ratiox: %f ratioy: %f",h.x,h.y,h.z,  nh.z,ratio,ratiox,ratioy);
        //  g_ypos+=18;
    }

    Matrix33 _m33;
    _m33.SetIdentity();
    AABB aabb1 = AABB(Vec3(-0.03f, -YR, -0.001f), Vec3(0.03f, YR, 0.001f));
    OBB _obb1 = OBB::CreateOBBfromAABB(SlopeMat33, aabb1);
    AABB aabb2 = AABB(Vec3(-XR, -0.03f, -0.001f), Vec3(XR, 0.03f, 0.001f));
    OBB _obb2 = OBB::CreateOBBfromAABB(SlopeMat33, aabb2);

    SlopeMat33 = SlopeMat33 * ScaleMat33;
    // Draw grid.
    float step = 0.25f;
    for (float x = -XR; x < XR; x += step)
    {
        Vec3 p0 = Vec3(x, -YR, 0);
        Vec3 p1 = Vec3(x, YR, 0);
        //pAuxGeom->DrawLine( SlopeMat33*p0,RGBA8(0x7f,0x7f,0x7f,0x00), SlopeMat33*p1,RGBA8(0x7f,0x7f,0x7f,0x00) );
        int32 intx = int32(x);
        if (fabsf(intx - x) < 0.001f)
        {
            pAuxGeom->DrawOBB(_obb1, SlopeMat33 * Vec3(x, 0.0f, 0.0f) + m_GridOrigin, 1, RGBA8(0x9f, 0x9f, 0x9f, 0x00), eBBD_Faceted);
        }
        else
        {
            pAuxGeom->DrawLine(SlopeMat33 * p0 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00), SlopeMat33 * p1 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00));
        }
    }

    for (float y = -YR; y < YR; y += step)
    {
        Vec3 p0 = Vec3(-XR, y, 0);
        Vec3 p1 = Vec3(XR, y, 0);
        //  pAuxGeom->DrawLine( SlopeMat33*p0,RGBA8(0x7f,0x7f,0x7f,0x00), SlopeMat33*p1,RGBA8(0x7f,0x7f,0x7f,0x00) );
        int32 inty = int32(y);
        if (fabsf(inty - y) < 0.001f)
        {
            pAuxGeom->DrawOBB(_obb2, SlopeMat33 * Vec3(0.0f, y, 0.0f) + m_GridOrigin, 1, RGBA8(0x9f, 0x9f, 0x9f, 0x00), eBBD_Faceted);
        }
        else
        {
            pAuxGeom->DrawLine(SlopeMat33 * p0 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00), SlopeMat33 * p1 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00));
        }
    }

    // TODO - the grid should probably be an IRenderNode at some point
    // flushing grid geometry now so it will not override transparent
    // objects later in the render pipeline.
    pAuxGeom->Commit();
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void CModelViewport::DrawCoordSystem(const QuatT& location, f32 length)
{
    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
    pAuxGeom->SetRenderFlags(renderFlags);

    Vec3 absAxisX   =   location.q.GetColumn0();
    Vec3 absAxisY   =   location.q.GetColumn1();
    Vec3 absAxisZ   = location.q.GetColumn2();

    const f32 scale = 3.0f;
    const f32 size = 0.009f;
    AABB xaabb = AABB(Vec3(-length * scale, -size * scale, -size * scale), Vec3(length * scale, size * scale,   size * scale));
    AABB yaabb = AABB(Vec3(-size * scale, -length * scale, -size * scale), Vec3(size * scale,     length * scale, size * scale));
    AABB zaabb = AABB(Vec3(-size * scale, -size * scale, -length * scale), Vec3(size * scale,     size * scale,     length * scale));

    OBB obb;
    obb =   OBB::CreateOBBfromAABB(Matrix33(location.q), xaabb);
    pAuxGeom->DrawOBB(obb, location.t, 1, RGBA8(0xff, 0x00, 0x00, 0xff), eBBD_Extremes_Color_Encoded);
    pAuxGeom->DrawCone(location.t + absAxisX * length * scale, absAxisX, 0.03f * scale, 0.15f * scale, RGBA8(0xff, 0x00, 0x00, 0xff));

    obb =   OBB::CreateOBBfromAABB(Matrix33(location.q), yaabb);
    pAuxGeom->DrawOBB(obb, location.t, 1, RGBA8(0x00, 0xff, 0x00, 0xff), eBBD_Extremes_Color_Encoded);
    pAuxGeom->DrawCone(location.t + absAxisY * length * scale, absAxisY, 0.03f * scale, 0.15f * scale, RGBA8(0x00, 0xff, 0x00, 0xff));

    obb =   OBB::CreateOBBfromAABB(Matrix33(location.q), zaabb);
    pAuxGeom->DrawOBB(obb, location.t, 1, RGBA8(0x00, 0x00, 0xff, 0xff), eBBD_Extremes_Color_Encoded);
    pAuxGeom->DrawCone(location.t + absAxisZ * length * scale, absAxisZ, 0.03f * scale, 0.15f * scale, RGBA8(0x00, 0x00, 0xff, 0xff));
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnLightMultiplier([[maybe_unused]] IVariable* var)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetSelected(bool const bSelect)
{
    // If a modelviewport gets activated, and listeners will be activated, disable the main viewport listener and re-enable when you lose focus.
    if (gEnv->pSystem)
    {
        IViewSystem* const pIViewSystem = gEnv->pSystem->GetIViewSystem();

        if (pIViewSystem)
        {
            pIViewSystem->SetControlAudioListeners(!bSelect);
        }
    }
}

#include <moc_ModelViewport.cpp>
