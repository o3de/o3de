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

#include "EditorDefs.h"

//AZCore
#include <AzCore/Math/MathUtils.h>

//CRY
#include <ILogFile.h>
#include <IMaterial.h>

//Editor
#include <Settings.h>
#include <IEditor.h>
#include <IResourceSelectorHost.h>
#include <Util/Variable.h>
#include <Util/VariablePropertyType.h>
#include <QViewport.h>
#include <QViewportSettings.h>
#include <DisplaySettings.h>

//EditorCommon
#include <RenderHelpers/AxisHelper.h>

//Local
#include "PreviewModelView.h"

//QT
#include <qapplication.h>



CPreviewModelView::CPreviewModelView(QWidget* parent)
    : QViewport(parent, QViewport::StartupMode_Manual) // Manual since we need to set WA_DontCreateNativeAncestors before QViewport::Startup() creates the internal native window and propagates
    , m_Flags(0)
    , m_GridColor(150, 150, 150, 40)
    , m_BackgroundColor(0.5f, 0.5f, 0.5f)
    , m_TimeScale(1.0f)
    , m_PlayState(PlayState::NONE)
    , m_pStaticModel(nullptr)
    , m_PostUpdateCallback(nullptr)
    , m_ContextMenuCallback(nullptr)
{
#ifdef Q_OS_MACOS
    // Don't propagate the nativeness up, as dockwidgets on macOS don't like it
    setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif
    Startup();

    //////////////////////////////////////////////////////////
    //QViewport
    AddConsumer(this);
    //////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////
    //IEditorNotifyListener
    GetIEditor()->RegisterNotifyListener(this);
    //////////////////////////////////////////////////////////

    FocusOnScreen();//update the camera ...
    SetDefaultFlags();
    UpdateSettings();
}

CPreviewModelView::~CPreviewModelView()
{
    CRY_ASSERT(GetIEditor());

    //////////////////////////////////////////////////////////
    //IEditorNotifyListener
    GetIEditor()->UnregisterNotifyListener(this);
    //////////////////////////////////////////////////////////

    ReleaseModel();
}


bool CPreviewModelView::IsFlagSet(PreviewModelViewFlag flag) const
{
    CRY_ASSERT(flag < PreviewModelViewFlag::END_POSSIBLE_ITEMS);
    return (m_Flags & (1 << static_cast<unsigned int>(flag)));
}

void CPreviewModelView::ToggleFlag(PreviewModelViewFlag flag)
{
    CRY_ASSERT(flag < PreviewModelViewFlag::END_POSSIBLE_ITEMS);
    if (IsFlagSet(flag))
    {
        UnSetFlag(flag);
    }
    else
    {
        SetFlag(flag);
    }
}

void CPreviewModelView::SetFlag(PreviewModelViewFlag flag)
{
    CRY_ASSERT(flag < PreviewModelViewFlag::END_POSSIBLE_ITEMS);
    m_Flags |= (1 << static_cast<unsigned int>(flag));
}

void CPreviewModelView::UnSetFlag(PreviewModelViewFlag flag)
{
    CRY_ASSERT(flag < PreviewModelViewFlag::END_POSSIBLE_ITEMS);
    m_Flags &= ~(1 << static_cast<unsigned int>(flag));
}

void CPreviewModelView::ResetPlaybackControls()
{
    UnSetFlag(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY);
    m_TimeScale = 1.0f;
}

void CPreviewModelView::ResetBackgroundColor()
{
    SetBackgroundColor(ColorF(0.5f, 0.5f, 0.5f));
}

void CPreviewModelView::ResetGridColor()
{
    SetGridColor(ColorF(150, 150, 150, 40));
}

void CPreviewModelView::ResetCamera()
{
    FocusOnScreen();
}

void CPreviewModelView::ResetAll()
{
    ResetPlaybackControls();
    ResetGridColor();
    ResetBackgroundColor();
    ReleaseModel();
    SetDefaultFlags();
}

void CPreviewModelView::SetGridColor(ColorF color)
{
    m_GridColor = color;
}

void CPreviewModelView::SetBackgroundColor(ColorF color)
{
    m_BackgroundColor = color;
}

void CPreviewModelView::SetPlayState(PlayState state)
{
    m_PlayState = state;
}

void CPreviewModelView::SetTimeScale(float scale)
{
    m_TimeScale = scale;
}

CPreviewModelView::PlayState CPreviewModelView::GetPlayState() const
{
    return m_PlayState;
}

float CPreviewModelView::GetTimeScale() const
{
    return m_TimeScale;
}


ColorF CPreviewModelView::GetGridColor() const
{
    return m_GridColor;
}

ColorF CPreviewModelView::GetBackgroundColor() const
{
    return m_BackgroundColor;
}

void CPreviewModelView::ImportModel()
{
    SResourceSelectorContext x;
    x.typeName = Prop::GetPropertyTypeToResourceType(ePropertyModel);

    QString currPath = m_ModelFilename.toLower();
    QString selected = GetIEditor()->GetResourceSelectorHost()->SelectResource(x, currPath);
    LoadModelFile(selected);
}

void CPreviewModelView::SetPostUpdateCallback(PostUpdateCallback callback)
{
    m_PostUpdateCallback = callback;
}

void CPreviewModelView::SetContextMenuCallback(ContextMenuCallback callback)
{
    m_ContextMenuCallback = callback;
}

////////////////////////////////////////////////////////
//QViewportConsumer
void CPreviewModelView::OnViewportRender(const SRenderContext& rc)
{
    ///UPDATE
    UpdateSettings();//Some changes may take effect next frame ...

    //External updating ...
    if (m_PostUpdateCallback)
    {
        m_PostUpdateCallback();
    }

    //RENDER
    CRY_ASSERT(rc.renderParams);
    CRY_ASSERT(rc.passInfo);
    RenderModels(*rc.renderParams, *rc.passInfo);
}

void CPreviewModelView::OnViewportKey([[maybe_unused]] const SKeyEvent& ev)
{
}

void CPreviewModelView::OnViewportMouse([[maybe_unused]] const SMouseEvent& ev)
{
}
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
//IEditorNotifyListener
void CPreviewModelView::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnCloseScene:
    {
        ResetAll();
        break;
    }
    case eNotify_OnIdleUpdate:
    {
        Update();
    }
    break;
    default:
        break;
    }
}
///////////////////////////////////////////////////////


void CPreviewModelView::UpdateSettings()
{
    //Update the settings
    SViewportSettings allsettings;

    allsettings.background.topColor = m_BackgroundColor;
    allsettings.background.useGradient = false;

    allsettings.grid.showGrid = IsFlagSet(PreviewModelViewFlag::SHOW_GRID);
    allsettings.grid.middleColor = m_BackgroundColor;
    allsettings.grid.mainColor = m_GridColor;

    allsettings.rendering.wireframe = IsFlagSet(PreviewModelViewFlag::DRAW_WIREFRAME);
    allsettings.rendering.sunlight = IsFlagSet(PreviewModelViewFlag::ENABLE_TIME_OF_DAY);
    allsettings.rendering.fps = false;


    // Set zoom speed to 1.0f for better UI workflow, instead of the default 0.1f
    allsettings.camera.zoomSpeed = 1.0f;
    allsettings.camera.moveSpeed = GetIEditor()->GetEditorSettings()->cameraMoveSpeed;
    allsettings.camera.fastMoveMultiplier = GetIEditor()->GetEditorSettings()->cameraFastMoveSpeed;
    allsettings.camera.rotationSpeed = GetIEditor()->GetEditorSettings()->cameraRotateSpeed;
    allsettings.camera.showViewportOrientation = IsFlagSet(PreviewModelViewFlag::SHOW_GRID_AXIS);

    //Set them ...
    SetSettings(allsettings);
}



void CPreviewModelView::RenderModels(SRendParams& rendParams, SRenderingPassInfo& passInfo)
{
    if (m_pStaticModel)
    {
        if (IsFlagSet(PreviewModelViewFlag::PRECACHE_MATERIAL))
        {
            _smart_ptr<IMaterial>  pCurMat = m_pStaticModel->GetMaterial();
            if (pCurMat)
            {
                pCurMat->PrecacheMaterial(0.0f, nullptr, true, true);
            }
        }
        m_pStaticModel->Render(rendParams, passInfo);
    }
}

void CPreviewModelView::ReleaseModel()
{
    m_ModelFilename = "";
    SAFE_RELEASE(m_pStaticModel);
}

void CPreviewModelView::SetDefaultFlags()
{
    UnSetFlag(PreviewModelViewFlag::SHOW_OVERDRAW);

    m_Flags = 0;

    SetFlag(PreviewModelViewFlag::SHOW_GRID);
    SetFlag(PreviewModelViewFlag::SHOW_GRID_AXIS);
}

void CPreviewModelView::LoadModelFile(const QString& modelFile)
{
    //Something to load
    if (!modelFile.isEmpty())
    {
        //Make sure we are not loading the same thing ...
        if (m_ModelFilename != modelFile)
        {
            ReleaseModel();//release any old mesh

            QString strFileExt = Path::GetExt(modelFile);
            bool isCGF = (QString::compare(strFileExt, CRY_GEOMETRY_FILE_EXT, Qt::CaseInsensitive) == 0);

            // NOTE: have to create a local buffer and evaluate the full message here due to calling into another address space to evaluate va_args
            // this prevents passing of random data to the log system ... any plugin that does not do it this way is rolling the dice each time.
            char buffer[2046];
            CRY_ASSERT(GetIEditor());
            if (isCGF)
            {
                CRY_ASSERT(GetIEditor()->Get3DEngine());
                // Load object.
                m_pStaticModel = GetIEditor()->Get3DEngine()->LoadStatObjUnsafeManualRef(modelFile.toUtf8().data(), nullptr, nullptr, false);
                if (m_pStaticModel)
                {
                    m_pStaticModel->AddRef();
                }
                else
                {
                    CRY_ASSERT(GetIEditor()->GetLogFile());
                    azsprintf(buffer, "Loading of geometry object %s failed.", modelFile.toUtf8().data());
                    GetIEditor()->GetLogFile()->Warning(buffer);
                }
            }
            else
            {
                CRY_ASSERT(GetIEditor()->GetLogFile());
                azsprintf(buffer, "Unknown model file (%s) attempting to be loaded.", modelFile.toUtf8().data());
                GetIEditor()->GetLogFile()->Warning(buffer);
            }

            //if something was loaded then we store off the model path for the future
            if (m_pStaticModel)
            {
                m_ModelFilename = modelFile;
            }
        }
    }
}

IStatObj* CPreviewModelView::GetStaticModel()
{
    return m_pStaticModel;
}

void CPreviewModelView::FocusOnScreen()
{
    CCamera* camera = Camera();
    if (camera)
    {
        AABB accumulated(2);

        if (m_pStaticModel)
        {
            AABB temp;
            temp.min = m_pStaticModel->GetBoxMin();
            temp.max = m_pStaticModel->GetBoxMax();
            accumulated.Add(temp);
        }

        Vec3 fromDir(1.0f, 1.0f, -0.5f);
        Vec3 target = accumulated.GetCenter();
        float bbRadius = accumulated.GetRadius();

        Vec3 dir = fromDir.GetNormalized();
        Matrix34 tm = Matrix33::CreateRotationVDir(dir, 0);
        tm.SetTranslation(target - dir * bbRadius);
        CameraMoved(QuatT(tm), true);
    }
}

float CPreviewModelView::GetSpeedScale() const
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetSystem());
    CRY_ASSERT(GetIEditor()->GetSystem()->GetITimer());
    //Taken from CRenderViewport to mirror controls from there ...
    float speedScale = 60.0f * GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();
    if (speedScale > 20.0f)
    {
        speedScale = 20.0f;
    }
    return speedScale;
}

