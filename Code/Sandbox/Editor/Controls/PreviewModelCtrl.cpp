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

#include "PreviewModelCtrl.h"

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// Editor
#include "Settings.h"
#include "Util/Image.h"
#include "Include/IIconManager.h"

struct CPreviewModelCtrl::SPreviousContext
{
    CCamera renderCamera;
    CCamera systemCamera;
    int width;
    int height;
    HWND window;
    bool isMainViewport;
};

CPreviewModelCtrl::CPreviewModelCtrl(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    OnCreate();
}

void CPreviewModelCtrl::OnCreate()
{
    m_bShowObject = true;
    m_pRenderer = 0;
    m_pObj = 0;
    m_pEntity = 0;
    m_nTimer = 0;
    m_size = Vec3(0, 0, 0);

    m_bRotate = false;
    m_rotateAngle = 0;

    m_backgroundTextureId = 0;

    m_pRenderer = GetIEditor()->GetRenderer();

    m_fov = 60;
    m_camera.SetFrustum(800, 600, DEG2RAD(m_fov), 0.02f, 10000.0f);

    m_bInRotateMode = false;
    m_bInMoveMode = false;

    CDLight l;

    float L = 1.0f;
    l.m_fRadius = 10000;
    l.m_Flags |= DLF_SUN | DLF_DIRECTIONAL;
    l.SetLightColor(ColorF(L, L, L, 1));
    l.SetPosition(Vec3(100, 100, 100));
    m_lights.push_back(l);

    m_bUseBacklight = false;
    m_bContextCreated = false;
    m_bHaveAnythingToRender = false;
    m_bGrid = true;
    m_bAxis = true;
    m_bAxisParticleEditor = false;
    m_bUpdate = false;
    m_bShowNormals = false;
    m_bShowPhysics = false;
    m_bShowRenderInfo = false;

    m_cameraAngles.Set(0, 0, 0);

    m_clearColor.Set(0.5f, 0.5f, 0.5f);
    m_ambientColor.Set(1.0f, 1.0f, 1.0f);
    m_ambientMultiplier = 0.5f;

    m_cameraChangeCallback = NULL;
    m_bPrecacheMaterial = false;
    m_bDrawWireFrame = false;

    m_tileX = 0.0f;
    m_tileY = 0.0f;
    m_tileSizeX = 1.0f;
    m_tileSizeY = 1.0f;

    m_aabb = AABB(2);
    FitToScreen();

    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);

    GetIEditor()->RegisterNotifyListener(this);
}

CPreviewModelCtrl::~CPreviewModelCtrl()
{
    OnDestroy();
    ReleaseObject();
    GetIEditor()->UnregisterNotifyListener(this);
}

bool CPreviewModelCtrl::CreateContext()
{
    // Create context.
    if (m_pRenderer && !m_bContextCreated)
    {
        StorePreviousContext();
        m_bContextCreated = true;

        // save the old context, because CreateContext sets it, and we don't actually want that:
        StorePreviousContext();
        m_pRenderer->CreateContext(m_hWnd);

        RestorePreviousContext();
        return true;
    }
    return false;
}

void CPreviewModelCtrl::ReleaseObject()
{
    m_pObj = NULL;
    m_pEntity = 0;
    m_bHaveAnythingToRender = false;
}

void CPreviewModelCtrl::LoadFile(const QString& modelFile, bool changeCamera)
{
    m_bHaveAnythingToRender = false;
    if (!m_hWnd)
    {
        return;
    }
    if (!m_pRenderer)
    {
        return;
    }

    ReleaseObject();

    if (modelFile.isEmpty())
    {
        if (m_nTimer != 0)
        {
            killTimer(m_nTimer);
        }
        m_nTimer = 0;
        update();
        return;
    }

    m_loadedFile = modelFile;

    QString strFileExt = QFileInfo(modelFile).suffix();
    const bool isSKEL = strFileExt.compare(QStringLiteral(CRY_SKEL_FILE_EXT), Qt::CaseInsensitive) == 0;
    const bool isSKIN = strFileExt.compare(QStringLiteral(CRY_SKIN_FILE_EXT), Qt::CaseInsensitive) == 0;
    const bool isCDF = strFileExt.compare(QStringLiteral(CRY_CHARACTER_DEFINITION_FILE_EXT), Qt::CaseInsensitive) == 0;
    const bool isCGA = strFileExt.compare(QStringLiteral(CRY_ANIM_GEOMETRY_FILE_EXT), Qt::CaseInsensitive) == 0;
    const bool isCGF = strFileExt.compare(QStringLiteral(CRY_GEOMETRY_FILE_EXT), Qt::CaseInsensitive) == 0;

    if (isCGF)
    {
        Warning("Loading of geometry object %s failed.", modelFile.toUtf8().constData());
    }
    else
    {
        if (m_nTimer != 0)
        {
            killTimer(m_nTimer);
        }
        m_nTimer = 0;
        update();
        return;
    }

    m_bHaveAnythingToRender = true;

    if (changeCamera)
    {
        FitToScreen();
    }

    update();
}

void CPreviewModelCtrl::SetAspectRatio(float aspectRatio)
{
    if (aspectRatio != m_aspectRatio)
    {
        m_aspectRatio = aspectRatio;
        m_useAspectRatio = true;
        updateGeometry();
    }
}

bool CPreviewModelCtrl::hasHeightForWidth() const
{
    return m_useAspectRatio;
}

int CPreviewModelCtrl::heightForWidth(int w) const
{
    if (m_useAspectRatio)
    {
        return static_cast<int>(static_cast<float>(w) / m_aspectRatio);
    }

    return QWidget::heightForWidth(w);
}

void CPreviewModelCtrl::SetEntity(IRenderNode* entity)
{
    m_bHaveAnythingToRender = false;
    if (m_pEntity != entity)
    {
        m_pEntity = entity;
        if (m_pEntity)
        {
            m_bHaveAnythingToRender = true;
            m_aabb = m_pEntity->GetBBox();
        }
        update();
    }
}

void CPreviewModelCtrl::SetObject(IStatObj* pObject)
{
    if (m_pObj != pObject)
    {
        m_bHaveAnythingToRender = false;
        m_pObj = pObject;
        if (m_pObj)
        {
            m_bHaveAnythingToRender = true;
            m_aabb = m_pObj->GetAABB();
        }
        update();
    }
}

void CPreviewModelCtrl::SetCameraRadius(float fRadius)
{
    m_cameraRadius = fRadius;

    Matrix34 m = m_camera.GetMatrix();
    Vec3 dir = m.TransformVector(Vec3(0, 1, 0));
    Matrix34 tm = Matrix33::CreateRotationVDir(dir, 0);
    tm.SetTranslation(m_cameraTarget - dir * m_cameraRadius);
    m_camera.SetMatrix(tm);
    if (m_cameraChangeCallback)
    {
        m_cameraChangeCallback(m_pCameraChangeUserData, this);
    }
}

void CPreviewModelCtrl::SetCameraLookAt(float fRadiusScale, const Vec3& fromDir)
{
    m_cameraTarget = m_aabb.GetCenter();
    m_cameraRadius = m_aabb.GetRadius() * fRadiusScale;

    Vec3 dir = fromDir.GetNormalized();
    Matrix34 tm = Matrix33::CreateRotationVDir(dir, 0);
    tm.SetTranslation(m_cameraTarget - dir * m_cameraRadius);
    m_camera.SetMatrix(tm);
    if (m_cameraChangeCallback)
    {
        m_cameraChangeCallback(m_pCameraChangeUserData, this);
    }
}

CCamera& CPreviewModelCtrl::GetCamera()
{
    return m_camera;
}

void CPreviewModelCtrl::UseBackLight(bool bEnable)
{
    if (bEnable)
    {
        m_lights.resize(1);
        CDLight l;
        l.SetPosition(Vec3(-100, 100, -100));
        float L = 0.5f;
        l.SetLightColor(ColorF(L, L, L, 1));
        l.m_fRadius = 1000;
        l.m_Flags |= DLF_POINT;
        m_lights.push_back(l);
    }
    else
    {
        m_lights.resize(1);
    }
    m_bUseBacklight = bEnable;
}

void CPreviewModelCtrl::SetCamera(CCamera& cam)
{
    m_camera.SetPosition(cam.GetPosition());

#if defined(AZ_PLATFORM_WINDOWS)
    // Needed for high DPI mode on windows
    const qreal ratio = devicePixelRatioF();
#else
    const qreal ratio = 1.0f;
#endif
    const int w = width() * ratio * m_tileSizeX;
    const int h = height() * ratio * m_tileSizeY;
    m_camera.SetFrustum(w, h, DEG2RAD(m_fov), m_camera.GetNearPlane(), m_camera.GetFarPlane());

    if (m_cameraChangeCallback)
    {
        m_cameraChangeCallback(m_pCameraChangeUserData, this);
    }
}

void CPreviewModelCtrl::SetOrbitAngles([[maybe_unused]] const Ang3& ang)
{
    assert(0);
}

bool CPreviewModelCtrl::Render()
{
    const int w = width();
    const int h = height();

    if (h < 2 || w < 2)
    {
        return false;
    }

    if (!m_bContextCreated)
    {
        if (!CreateContext())
        {
            return false;
        }
    }

    _smart_ptr<IMaterial> pMaterial;

    if (m_bPrecacheMaterial)
    {
        // Precache material - This loads/creates the material, shader and textures
        // Moving this to the top of Render() so precaching precedes current and future
        // rendering code

        _smart_ptr<IMaterial> pCurMat = pMaterial;

        if (!pCurMat)
        {
            pCurMat = GetCurrentMaterial();
        }

        if (pCurMat)
        {
            pCurMat->PrecacheMaterial(0.0f, NULL, true, true);
        }
    }

    SetCamera(m_camera);

    m_pRenderer->SetClearColor(Vec3(m_clearColor.r, m_clearColor.g, m_clearColor.b));
    m_pRenderer->BeginFrame();
    SetCurrentContext();
    m_pRenderer->SetRenderTile(m_tileX, m_tileY, m_tileSizeX, m_tileSizeY);

    // Render grid. Explicitly clear color and depth buffer first
    // (otherwise ->EndEf3D() will do that and thereby clear the grid).
    m_pRenderer->ClearTargetsImmediately(FRT_CLEAR, m_clearColor);

    DrawBackground();
    if (m_bGrid || m_bAxis)
    {
        DrawGrid();
    }

    // save some cvars
    int showNormals = gEnv->pConsole->GetCVar("r_ShowNormals")->GetIVal();
    int showInfo = gEnv->pConsole->GetCVar("r_displayInfo")->GetIVal();

    gEnv->pConsole->GetCVar("r_ShowNormals")->Set((int)m_bShowNormals);
    gEnv->pConsole->GetCVar("r_displayInfo")->Set((int)m_bShowRenderInfo);

    // Render object.
    SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_camera, SRenderingPassInfo::DEFAULT_FLAGS, true);
    m_pRenderer->BeginSpawningGeneratingRendItemJobs(passInfo.ThreadID());
    m_pRenderer->BeginSpawningShadowGeneratingRendItemJobs(passInfo.ThreadID());
    m_pRenderer->EF_StartEf(passInfo);
    m_pRenderer->ResetToDefault();


    {
        CScopedWireFrameMode scopedWireFrame(m_pRenderer, m_bDrawWireFrame ? R_WIREFRAME_MODE : R_SOLID_MODE);

        // Add lights.
        for (size_t i = 0; i < m_lights.size(); i++)
        {
            m_pRenderer->EF_ADDDlight(&m_lights[i], passInfo);
        }

        if (m_bShowObject)
        {
            RenderObject(pMaterial, passInfo);
        }

        m_pRenderer->EF_EndEf3D(SHDF_NOASYNC | SHDF_STREAM_SYNC, -1, -1, passInfo);
    }

    m_pRenderer->EF_RenderTextMessages();
    m_pRenderer->RenderDebug(false);
    m_pRenderer->EndFrame();
    m_pRenderer->SetRenderTile();

    // Restore main context.
    RestorePreviousContext();

    gEnv->pConsole->GetCVar("r_ShowNormals")->Set(showNormals);
    gEnv->pConsole->GetCVar("r_displayInfo")->Set(showInfo);

    return true;
}

void CPreviewModelCtrl::RenderObject(_smart_ptr<IMaterial> pMaterial, SRenderingPassInfo& passInfo)
{
    SRendParams rp;
    rp.dwFObjFlags = 0;
    rp.AmbientColor = m_ambientColor * m_ambientMultiplier;
    rp.dwFObjFlags |= FOB_NO_FOG;
    rp.pMaterial = pMaterial;

    Matrix34 tm;
    tm.SetIdentity();
    rp.pMatrix = &tm;

    if (m_bRotate)
    {
        tm.SetRotationXYZ(Ang3(0, 0, m_rotateAngle));
        m_rotateAngle += 0.1f;
    }

    if (m_pObj)
    {
        m_pObj->Render(rp, passInfo);
    }

    if (m_pEntity)
    {
        m_pEntity->Render(rp, passInfo);
    }
}

void CPreviewModelCtrl::DrawGrid()
{
    // Draw grid.
    float step = 0.1f;
    float XR = 5;
    float YR = 5;

    IRenderAuxGeom* pRag = m_pRenderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags nRendFlags = pRag->GetRenderFlags();

    pRag->SetRenderFlags(e_Def3DPublicRenderflags);
    SAuxGeomRenderFlags nNewFlags = pRag->GetRenderFlags();
    nNewFlags.SetAlphaBlendMode(e_AlphaBlended);
    pRag->SetRenderFlags(nNewFlags);

    int nGridAlpha = 40;
    if (m_bGrid)
    {
        // Draw grid.
        for (float x = -XR; x < XR; x += step)
        {
            if (fabs(x) > 0.01)
            {
                pRag->DrawLine(Vec3(x, -YR, 0), ColorB(150, 150, 150, nGridAlpha), Vec3(x, YR, 0), ColorB(150, 150, 150, nGridAlpha));
            }
        }
        for (float y = -YR; y < YR; y += step)
        {
            if (fabs(y) > 0.01)
            {
                pRag->DrawLine(Vec3(-XR, y, 0), ColorB(150, 150, 150, nGridAlpha), Vec3(XR, y, 0), ColorB(150, 150, 150, nGridAlpha));
            }
        }
    }

    nGridAlpha = 60;
    if (m_bAxis)
    {
        // Draw axis.
        pRag->DrawLine(Vec3(0, 0, 0), ColorB(255, 0, 0, nGridAlpha), Vec3(XR, 0, 0), ColorB(255, 0, 0, nGridAlpha));
        pRag->DrawLine(Vec3(0, 0, 0), ColorB(0, 255, 0, nGridAlpha), Vec3(0, YR, 0), ColorB(0, 255, 0, nGridAlpha));
        pRag->DrawLine(Vec3(0, 0, 0), ColorB(0, 0, 255, nGridAlpha), Vec3(0, 0, YR), ColorB(0, 0, 255, nGridAlpha));
    }
    pRag->Flush();
    pRag->SetRenderFlags(nRendFlags);
}

void CPreviewModelCtrl::timerEvent(QTimerEvent* event)
{
    if (isVisible())
    {
        if (m_bHaveAnythingToRender)
        {
            update();
        }
    }

    QWidget::timerEvent(event);
}

void CPreviewModelCtrl::SetCameraTM(const Matrix34& cameraTM)
{
    m_camera.SetMatrix(cameraTM);
    if (m_cameraChangeCallback)
    {
        m_cameraChangeCallback(m_pCameraChangeUserData, this);
    }
}

void CPreviewModelCtrl::GetCameraTM(Matrix34& cameraTM)
{
    cameraTM = m_camera.GetMatrix();
}

void CPreviewModelCtrl::DeleteRenderContex()
{
    ReleaseObject();

    // Destroy render context.
    if (m_pRenderer && m_bContextCreated)
    {
        m_pRenderer->DeleteContext(m_hWnd);
        m_bContextCreated = false;
    }
}

void CPreviewModelCtrl::OnDestroy()
{
    DeleteRenderContex();

    if (m_nTimer)
    {
        killTimer(m_nTimer);
    }
}

void CPreviewModelCtrl::OnLButtonDown(QPoint point)
{
    m_bInRotateMode = true;
    m_mousePosition = point;
    m_previousMousePosition = m_mousePosition;

    setFocus();
    update();
}

void CPreviewModelCtrl::OnLButtonUp(QPoint point)
{
    m_bInRotateMode = false;
    update();
    m_mousePosition = point;
    m_previousMousePosition = m_mousePosition;
}

void CPreviewModelCtrl::OnMButtonDown(QPoint point)
{
    m_bInPanMode = true;
    update();
    m_mousePosition = point;
    m_previousMousePosition = m_mousePosition;
}

void CPreviewModelCtrl::OnMButtonUp(QPoint point)
{
    m_bInPanMode = false;
    update();
    m_mousePosition = point;
    m_previousMousePosition = m_mousePosition;
}

void CPreviewModelCtrl::mouseMoveEvent(QMouseEvent* event)
{
    // TODO: Add your message handler code here and/or call default
    QWidget::mouseMoveEvent(event);

    const QPoint& point = event->pos();
    if (point == m_previousMousePosition)
    {
        return;
    }

    if (m_bInMoveMode)
    {
        // Zoom.
        Matrix34 m = m_camera.GetMatrix();
        Vec3 xdir(0, 0, 0);
        Vec3 zdir = m.GetColumn1().GetNormalized();

        float step = 0.002f;
        float dx = (point.x() - m_previousMousePosition.x());
        float dy = (point.y() - m_previousMousePosition.y());
        m_camera.SetPosition(m_camera.GetPosition() + step * xdir * dx +  step * zdir * dy);
        SetCamera(m_camera);

        if (gSettings.stylusMode)
        {
            m_previousMousePosition = point;
        }
        else
        {
            AzQtComponents::SetCursorPos(mapToGlobal(m_previousMousePosition));
        }

        update();
    }
    else if (m_bInRotateMode)
    {
        Vec3 pos = m_camera.GetMatrix().GetTranslation();
        m_cameraRadius = Vec3(m_camera.GetMatrix().GetTranslation() - m_cameraTarget).GetLength();
        // Look
        Ang3 angles(-point.y() + m_previousMousePosition.y(), 0, -point.x() + m_previousMousePosition.x());
        angles = angles * 0.002f;

        Matrix34 camtm = m_camera.GetMatrix();
        Matrix33 Rz = Matrix33::CreateRotationXYZ(Ang3(0, 0, angles.z)); // Rotate around vertical axis.
        Matrix33 Rx = Matrix33::CreateRotationAA(angles.x, camtm.GetColumn0()); // Rotate with angle around x axis in camera space.

        Vec3 dir = camtm.TransformVector(Vec3(0, 1, 0));
        Vec3 newdir = (Rx * Rz).TransformVector(dir).GetNormalized();
        camtm = Matrix34(Matrix33::CreateRotationVDir(newdir, 0), m_cameraTarget - newdir * m_cameraRadius);
        m_camera.SetMatrix(camtm);
        if (m_cameraChangeCallback)
        {
            m_cameraChangeCallback(m_pCameraChangeUserData, this);
        }

        if (gSettings.stylusMode)
        {
            m_previousMousePosition = point;
        }
        else
        {
            AzQtComponents::SetCursorPos(mapToGlobal(m_previousMousePosition));
        }

        update();
    }
    else if (m_bInPanMode)
    {
        // Slide.
        float speedScale = 0.001f;
        Matrix34 m = m_camera.GetMatrix();
        Vec3 xdir = m.GetColumn0().GetNormalized();
        Vec3 zdir = m.GetColumn2().GetNormalized();

        Vec3 pos = m_cameraTarget;
        pos += 0.1f * xdir * (point.x() - m_previousMousePosition.x()) * speedScale + 0.1f * zdir * (m_previousMousePosition.y() - point.y()) * speedScale;
        m_cameraTarget = pos;

        Vec3 dir = m.TransformVector(Vec3(0, 1, 0));
        m.SetTranslation(m_cameraTarget - dir * m_cameraRadius);
        m_camera.SetMatrix(m);
        if (m_cameraChangeCallback)
        {
            m_cameraChangeCallback(m_pCameraChangeUserData, this);
        }

        if (gSettings.stylusMode)
        {
            m_previousMousePosition = point;
        }
        else
        {
            AzQtComponents::SetCursorPos(mapToGlobal(m_previousMousePosition));
        }

        update();
    }
}

void CPreviewModelCtrl::OnRButtonDown(QPoint point)
{
    m_bInMoveMode = true;
    m_mousePosition = point;
    m_previousMousePosition = point;
    update();
}

void CPreviewModelCtrl::OnRButtonUp(QPoint point)
{
    m_bInMoveMode = false;

    m_mousePosition = point;
    m_previousMousePosition = point;

    update();
}

void CPreviewModelCtrl::wheelEvent(QWheelEvent* event)
{
    const short zDelta = event->angleDelta().y();
    // TODO: Add your message handler code here and/or call default
    Matrix34 m = m_camera.GetMatrix();
    Vec3 zdir = m.GetColumn1().GetNormalized();

    //m_camera.SetPosition( m_camera.GetPos() + ydir*(m_mousePos.y-point.y),xdir*(m_mousePos.x-point.x) );
    m_camera.SetPosition(m_camera.GetPosition() + 0.002f * zdir * (zDelta));
    SetCamera(m_camera);
    update();
}

void CPreviewModelCtrl::EnableUpdate(bool bUpdate)
{
    m_bUpdate = bUpdate;
    if (m_bUpdate && !m_nTimer)
    {
        m_nTimer = startTimer(1000);
    }
    else if (!m_bUpdate && m_nTimer)
    {
        killTimer(m_nTimer), m_nTimer = 0;
    }
}

void CPreviewModelCtrl::Update(bool bForceUpdate)
{
    ProcessKeys();

    if (m_bUpdate && m_bHaveAnythingToRender || bForceUpdate)
    {
        if (isVisible())
        {
            update();
        }
    }
}

void CPreviewModelCtrl::SetRotation(bool bEnable)
{
    m_bRotate = bEnable;
}

void CPreviewModelCtrl::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
        Update();
        break;
    }
}

void CPreviewModelCtrl::GetImageOffscreen(CImageEx& image, const QSize& customSize)
{
    // hiding a window can cause this to be dropped, since it no longer associates with
    // an actual operating system window handle.
    if (!m_hWnd)
    {
        return;
    }
    if (!m_pRenderer)
    {
        return;
    }

    m_pRenderer->EnableSwapBuffers(false);
    Render();
    m_pRenderer->EnableSwapBuffers(true);

    int w;
    int h;

    if (customSize.isValid())
    {
        w = customSize.width();
        h = customSize.height();
    }
    else
    {
        w = width();
        h = height();
    }

    image.Allocate(w, h);

    // the renderer will read the frame buffer of the current render context, so we need to set ours as the current before we execute this command.
    SetCurrentContext();
    m_pRenderer->ReadFrameBufferFast(image.GetData(), w, h);
    RestorePreviousContext();
}

void CPreviewModelCtrl::SetClearColor(const ColorF& color)
{
    m_clearColor = color;
}


namespace PreviewModelControl
{
    struct MaterialId
    {
        const void* ptr;
        int id;

        MaterialId(const void* a_ptr, int a_id)
            : ptr(a_ptr)
            , id(a_id)
        {
        }

        bool operator<(const MaterialId& a) const
        {
            return ptr < a.ptr || id < a.id;
        }
    };

    static int GetFaceCountRecursively(IStatObj* p)
    {
        if (!p)
        {
            return 0;
        }
        int n = 0;
        if (p->GetRenderMesh())
        {
            n += p->GetRenderMesh()->GetIndicesCount() / 3;
        }
        for (int i = 0; i < p->GetSubObjectCount(); ++i)
        {
            IStatObj::SSubObject* const pS = p->GetSubObject(i);
            if (pS)
            {
                n += GetFaceCountRecursively(pS->pStatObj);
            }
        }
        return n;
    }

    static int GetVertexCountRecursively(IStatObj* p)
    {
        if (!p)
        {
            return 0;
        }
        int n = 0;
        if (p->GetRenderMesh())
        {
            n += p->GetRenderMesh()->GetVerticesCount();
        }
        for (int i = 0; i < p->GetSubObjectCount(); ++i)
        {
            IStatObj::SSubObject* const pS = p->GetSubObject(i);
            if (pS)
            {
                n += GetVertexCountRecursively(pS->pStatObj);
            }
        }
        return n;
    }

    static int GetMaxLodRecursively(IStatObj* p)
    {
        if (!p)
        {
            return 0;
        }
        int n = 0;
        for (int i = 1; i < 10; i++)
        {
            if (p->GetLodObject(i))
            {
                n = i;
            }
        }
        for (int i = 0; i < p->GetSubObjectCount(); ++i)
        {
            IStatObj::SSubObject* const pS = p->GetSubObject(i);
            if (pS)
            {
                const int n2 = GetMaxLodRecursively(pS->pStatObj);
                n = (n < n2) ? n2 : n;
            }
        }
        return n;
    }

    static void CollectMaterialsRecursively(std::set<PreviewModelControl::MaterialId>& mats, IStatObj* p)
    {
        if (!p)
        {
            return;
        }
        if (p->GetRenderMesh())
        {
            TRenderChunkArray& ch = p->GetRenderMesh()->GetChunks();
            for (size_t i = 0; i < ch.size(); ++i)
            {
                mats.insert(PreviewModelControl::MaterialId(p->GetMaterial(), ch[i].m_nMatID));
            }
        }
        for (int i = 0; i < p->GetSubObjectCount(); ++i)
        {
            IStatObj::SSubObject* const pS = p->GetSubObject(i);
            if (pS)
            {
                CollectMaterialsRecursively(mats, pS->pStatObj);
            }
        }
    }
}


int CPreviewModelCtrl::GetFaceCount()
{
    if (m_pObj)
    {
        return PreviewModelControl::GetFaceCountRecursively(m_pObj);
    }
    return 0;
}

int CPreviewModelCtrl::GetVertexCount()
{
    if (m_pObj)
    {
        return PreviewModelControl::GetVertexCountRecursively(m_pObj);
    }
    return 0;
}

int CPreviewModelCtrl::GetMaxLod()
{
    if (m_pObj)
    {
        return PreviewModelControl::GetMaxLodRecursively(m_pObj);
    }
    return 0;
}

int CPreviewModelCtrl::GetMtlCount()
{
    if (m_pObj)
    {
        std::set<PreviewModelControl::MaterialId> mats;
        CollectMaterialsRecursively(mats, m_pObj);
        return (int)mats.size();
    }
    return 0;
}

void CPreviewModelCtrl::FitToScreen()
{
    SetCameraLookAt(2.0f, Vec3(1, 1, -0.5));
}

void CPreviewModelCtrl::ProcessKeys()
{
    if (!hasFocus())
    {
        return;
    }

    int moveSpeed = 1;

    Matrix34 m = m_camera.GetMatrix();

    Vec3 ydir = m.GetColumn2().GetNormalized();
    Vec3 xdir = m.GetColumn0().GetNormalized();

    Vec3 pos = m.GetTranslation();

    float speedScale = 60.0f * GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();
    if (speedScale > 20)
    {
        speedScale = 20;
    }

    speedScale *= 0.04f;

    if (CheckVirtualKey(Qt::Key_Shift))
    {
        speedScale *= gSettings.cameraFastMoveSpeed;
    }

    bool isDirty = false;

    if (CheckVirtualKey(Qt::Key_Up) || CheckVirtualKey(Qt::Key_W))
    {
        // move forward
        m_camera.SetPosition(pos + speedScale * moveSpeed * ydir);
        SetCamera(m_camera);
        isDirty = true;
    }

    if (CheckVirtualKey(Qt::Key_Down) || CheckVirtualKey(Qt::Key_S))
    {
        // move backward
        m_camera.SetPosition(pos   -   speedScale * moveSpeed * ydir);
        SetCamera(m_camera);
        isDirty = true;
    }

    if (isDirty)
    {
        if (!m_bUpdate)
        {
            // if we're not going to be autoupdating then we need to do a one-time invalidation here.
            update();
        }
    }
}

void CPreviewModelCtrl::SetBackgroundTexture(const QString& textureFilename)
{
    m_backgroundTextureId = GetIEditor()->GetIconManager()->GetIconTexture(textureFilename.toUtf8().constData());
}

void CPreviewModelCtrl::DrawBackground()
{
    if (!m_backgroundTextureId)
    {
        return;
    }

    int rcw = width();
    int rch = height();

    TransformationMatrices backupSceneMatrices;

    m_pRenderer->Set2DMode(rcw, rch, backupSceneMatrices, 0.0f, 1.0f);

    m_pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);

    float uvs[4], uvt[4];
    uvs[3] = 0;
    uvt[3] = 1;
    uvs[2] = 1;
    uvt[2] = 1;
    uvs[1] = 1;
    uvt[1] = 0;
    uvs[0] = 0;
    uvt[0] = 0;

    float color[4] = {1, 1, 1, 1};

    m_pRenderer->DrawImageWithUV(0, 0, 0.5f, rcw, rch, m_backgroundTextureId, uvs, uvt, color[0], color[1], color[2], color[3]);
    m_pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

    m_pRenderer->Unset2DMode(backupSceneMatrices);
}

_smart_ptr<IMaterial> CPreviewModelCtrl::GetCurrentMaterial()
{
    if (m_pObj)
    {
        return m_pObj->GetMaterial();
    }
    else if (m_pEntity)
    {
        return m_pEntity->GetMaterial();
    }

    return NULL;
}

void CPreviewModelCtrl::StorePreviousContext()
{
    SPreviousContext previous;
    previous.width = m_pRenderer->GetWidth();
    previous.height = m_pRenderer->GetHeight();
    previous.window = (HWND)m_pRenderer->GetCurrentContextHWND();
    previous.renderCamera = m_pRenderer->GetCamera();
    previous.systemCamera = gEnv->pSystem->GetViewCamera();
    previous.isMainViewport = m_pRenderer->IsCurrentContextMainVP();
    m_previousContexts.push_back(previous);
}

// Copied from the non-crashing QViewport.cpp
void CPreviewModelCtrl::SetCurrentContext()
{
    StorePreviousContext();

    m_pRenderer->SetCurrentContext(m_hWnd);

#if defined(AZ_PLATFORM_WINDOWS)
    // Needed for high DPI mode on windows
    const qreal ratio = devicePixelRatioF();
#else
    const qreal ratio = 1.0f;
#endif
    m_pRenderer->ChangeViewport(0, 0, width() * ratio, height() * ratio);
    m_pRenderer->SetCamera(m_camera);
    gEnv->pSystem->SetViewCamera(m_camera);
}

void CPreviewModelCtrl::RestorePreviousContext()
{
    if (m_previousContexts.empty())
    {
        assert(0);
        return;
    }

    SPreviousContext x = m_previousContexts.back();
    m_previousContexts.pop_back();
    m_pRenderer->SetCurrentContext(x.window);
    m_pRenderer->ChangeViewport(0, 0, x.width, x.height, x.isMainViewport);
    m_pRenderer->SetCamera(x.renderCamera);
    gEnv->pSystem->SetViewCamera(x.systemCamera);
}

void CPreviewModelCtrl::showEvent([[maybe_unused]] QShowEvent* event)
{
    m_hWnd = reinterpret_cast<HWND>(effectiveWinId());
}

QSize CPreviewModelCtrl::minimumSizeHint() const
{
    return QSize(50, 50);
}

QPaintEngine* CPreviewModelCtrl::paintEngine() const
{
    return nullptr;
}

void CPreviewModelCtrl::paintEvent(QPaintEvent* event)
{
    event->accept();
    Render();
}

void CPreviewModelCtrl::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->pos());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->pos());
        break;
    }
}

void CPreviewModelCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->pos());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->pos());
        break;
    }
}

#include <Controls/moc_PreviewModelCtrl.cpp>
