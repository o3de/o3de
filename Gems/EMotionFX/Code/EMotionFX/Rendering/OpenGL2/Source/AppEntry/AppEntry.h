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

#include <AzCore/Math/Vector2.h>
//#define EMFX_EXAMPLE_YZSWAP
#include <MCore/Source/StandardHeaders.h>
#include <ThirdParty/Glew/include/GL/glew.h>
#ifdef MCORE_PLATFORM_WINDOWS
    #include <ThirdParty/Glew/include/GL/wglew.h>
#endif
#include <EMotionFX/Rendering/OpenGL2/Source/RenderGLConfig.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GraphicsManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GBuffer.h>
#include <MCore/Source/Timer.h>
#include <MCore/Source/Random.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/MemoryTracker.h>
#include <MCore/Source/MultiThreadManager.h>
#include <EMotionFX/Rendering/Common/OrbitCamera.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GLActor.h>

// example-implemented externs
extern bool InitEMotionFX();
extern bool Init();
extern void InitGUI();
extern void Update(float timeDelta);
extern void Cleanup();
extern void ShutdownEMotionFX();

// rendering global variables
RenderGL::GraphicsManager*  gEngine                 = nullptr;
RenderGL::GLRenderUtil*     gRenderUtil             = nullptr;
bool                        gRenderGrid             = true;
bool                        gRenderWireframe        = false;
bool                        gRenderAABBs            = false;
bool                        gRenderSkeleton         = false;
bool                        gRenderOBBs             = false;
bool                        gRenderTangents         = false;
bool                        gRenderVertexNormals    = false;
bool                        gRenderFaceNormals      = false;
bool                        gRenderCollisionMeshes  = false;
bool                        gRenderHelp             = false;
bool                        gRenderSolid            = true;

// character follow mode
bool                        gFollowCharacter        = false;
EMotionFX::ActorInstance*   gFollowActorInstance    = nullptr;
float                       gFollowCharacterHeight  = 0.0f;

// misc global variables
MCore::Timer                gTimer;
float                       gFPS                    = 0.0f;
MCommon::Camera*            gCamera                 = nullptr;
MCommon::OrbitCamera*       gOrbitCamera            = nullptr;
MCommon::LookAtCamera*      gLookAtCamera           = nullptr;
uint32                      gScreenWidth            = 1280;
uint32                      gScreenHeight           = 720;
bool                        gAutomaticCameraZoom    = true;
float                       gGridSpacing            = 0.2f;


void SetCamera(MCommon::Camera* camera)
{
    delete gCamera;
    gCamera = camera;
    gEngine->SetCamera(gCamera);

    if (gCamera->GetType() == MCommon::OrbitCamera::TYPE_ID)
    {
        gOrbitCamera = static_cast<MCommon::OrbitCamera*>(gCamera);
    }
    else
    {
        gOrbitCamera = nullptr;
    }

    if (gCamera->GetType() == MCommon::LookAtCamera::TYPE_ID)
    {
        gLookAtCamera = static_cast<MCommon::LookAtCamera*>(gCamera);
    }
    else
    {
        gLookAtCamera = nullptr;
    }
}


// render the camera orientation to the left bottom corner showing an axis
void RenderCameraOrientationAxis()
{
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    MCORE_ASSERT(gEngine != nullptr && gCamera != nullptr);
    RenderGL::GLRenderUtil* renderUtil = gEngine->GetRenderUtil();

    const uint32 axisSize = 3.0f;
    MCore::Vector3 axisPosition = Unproject(axisSize + 20.0f, gScreenHeight - 20.0f, gScreenWidth, gScreenHeight, gCamera->GetNearClipDistance() + 100.0f, gCamera->GetProjectionMatrix().Inversed(), gCamera->GetViewMatrix().Inversed());

    MCore::Matrix inverseCameraMatrix = gCamera->GetViewMatrix();
    inverseCameraMatrix.Inverse();

    MCore::Matrix worldTM;
    worldTM.SetTranslationMatrix(axisPosition);

    MCommon::RenderUtil::AxisRenderingSettings axisRenderingSettings;
    axisRenderingSettings.mRenderXAxis      = true;
    axisRenderingSettings.mRenderYAxis      = true;
    axisRenderingSettings.mRenderZAxis      = true;
    axisRenderingSettings.mSize             = axisSize;
    axisRenderingSettings.mGlobalTM         = worldTM;
    axisRenderingSettings.mCameraRight      = inverseCameraMatrix.GetRight().Normalized();
    axisRenderingSettings.mCameraUp         = inverseCameraMatrix.GetUp().Normalized();
    axisRenderingSettings.mRenderXAxisName  = true;
    axisRenderingSettings.mRenderYAxisName  = true;
    axisRenderingSettings.mRenderZAxisName  = true;

    // render directly as we have to disable the depth test, hope the additional render call won't slow down so much
    renderUtil->RenderLineAxis(axisRenderingSettings);
    ((MCommon::RenderUtil*)renderUtil)->RenderLines();

    glPopAttrib();
}


// render the grid
void RenderGrid()
{
    if (gRenderGrid == false)
    {
        return;
    }

    assert(gEngine != nullptr && gCamera != nullptr);
    RenderGL::GLRenderUtil* renderUtil = gEngine->GetRenderUtil();

    float gridUnitSize = gGridSpacing;
    AZ::Vector2 gridStart, gridEnd;

    renderUtil->CalcVisibleGridArea(gCamera, gScreenWidth, gScreenHeight, gridUnitSize, &gridStart, &gridEnd);

    if (gEngine->GetAdvancedRendering() == true)
    {
        renderUtil->RenderGrid(gridStart, gridEnd, MCore::Vector3(0.0f, 1.0f, 0.0f), gridUnitSize, MCore::RGBAColor(0.4f), MCore::RGBAColor(0.19f), MCore::RGBAColor(0.22f));
    }
    else
    {
        renderUtil->RenderGrid(gridStart, gridEnd, MCore::Vector3(0.0f, 1.0f, 0.0f), gridUnitSize, MCore::RGBAColor(0.0f, 0.008f, 0.03921), MCore::RGBAColor(0.32f, 0.357f, 0.4f), MCore::RGBAColor(0.24f, 0.282f, 0.329f));
    }

    // debug output
    //static MCore::String debugText;
    //debugText = AZStd::string::format("GridArea: start=(%f, %f), end=(%f, %f)", gridStart.x, gridStart.y, gridEnd.x, gridEnd.y);
    //renderUtil->DrawFont( 5, gScreenHeight/2, 0.0f, MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f).ToDWORD(), 10, debugText.AsChar() );
}


// render the help HUD
void RenderHelpScreen(RenderGL::GLRenderUtil* renderUtil)
{
    const uint32        textSize            = 8;
    const uint32        color               = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f).ToInt();
    const uint32        enabledColor        = MCore::RGBAColor(0.0f, 1.0f, 0.0f, 1.0f).ToInt();
    const uint32        disabledColor       = MCore::RGBAColor(1.0f, 0.0f, 0.0f, 1.0f).ToInt();

    if (gRenderHelp == false)
    {
        renderUtil->RenderText(gScreenWidth - 115, gScreenHeight - 20, "Press H to show help", color, textSize);
        return;
    }

    uint32 startY = gScreenHeight - 65;
    renderUtil->RenderText(75, startY, "Camera Controls:", color, textSize);
    renderUtil->RenderText(75, startY + 15, "ALT + Left Mouse Button + Mouse Movement: Rotate Camera", color, textSize);
    renderUtil->RenderText(75, startY + 27, "ALT + Right Mouse Button + Mouse Movement: Zoom Camera", color, textSize);
    renderUtil->RenderText(75, startY + 39, "ALT + Middle Mouse Button + Mouse Movement: Translate Camera", color, textSize);

    startY = gScreenHeight - 200;
    renderUtil->RenderText(5, startY, "Rendering:", color, textSize);
    renderUtil->RenderText(5, startY + 15, "G: Grid", gRenderGrid ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 27, "W: Wireframe", gRenderWireframe ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 39, "A: AABBs", gRenderAABBs ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 51, "S: Skeleton", gRenderSkeleton ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 63, "O: OBBs", gRenderOBBs ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 75, "T: Tangents & Binormals", gRenderTangents ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 87, "V: Vertex Normals", gRenderVertexNormals ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 99, "F: Face Normals", gRenderFaceNormals ? enabledColor : disabledColor, textSize);
    renderUtil->RenderText(5, startY + 111, "C: Collision Meshes", gRenderCollisionMeshes ? enabledColor : disabledColor, textSize);
}


// render helper geometry like wireframe, normals, tangents, AABBs, collision meshes etc.
void RenderHelpers(EMotionFX::ActorInstance* actorInstance)
{
    // if none of the features is active, return directly
    if (gRenderAABBs == false && gRenderOBBs == false && gRenderSkeleton == false &&
        gRenderVertexNormals == false && gRenderFaceNormals == false && gRenderTangents == false &&
        gRenderWireframe == false && gRenderCollisionMeshes == false)
    {
        return;
    }

    RenderGL::GLRenderUtil* renderUtil = gEngine->GetRenderUtil();

    EMotionFX::Actor* actor = actorInstance->GetActor();

    // render the AABBs
    if (gRenderAABBs == true)
    {
        MCommon::RenderUtil::AABBRenderSettings settings;
        renderUtil->RenderAABBs(actorInstance, settings);
    }

    // render the OBBs
    if (gRenderOBBs == true)
    {
        renderUtil->RenderOBBs(actorInstance);
    }

    // render skeleton
    if (gRenderSkeleton == true)
    {
        renderUtil->RenderSimpleSkeleton(actorInstance);
    }
    //bool cullingEnabled = glIsEnabled(GL_CULL_FACE);
    //  glDisable(GL_CULL_FACE);
    //  if (gRenderSkeleton == true)    renderUtil->RenderSkeleton( actorInstance, boneList );
    //if (cullingEnabled == true) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    if (gRenderVertexNormals == true || gRenderFaceNormals == true || gRenderTangents == true || gRenderWireframe == true || gRenderCollisionMeshes == true)
    {
        // iterate through all enabled nodes
        MCore::Matrix*  worldMatrices   = actorInstance->GetTransformData()->GetGlobalInclusiveMatrices();
        const uint32    geomLODLevel    = actorInstance->GetLODLevel();
        const uint32    numEnabled      = actorInstance->GetNumEnabledNodes();
        EMotionFX::Skeleton* skeleton   = actor->GetSkeleton();
        for (uint32 i = 0; i < numEnabled; ++i)
        {
            EMotionFX::Node*        node            = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            EMotionFX::Mesh*        mesh            = actor->GetMesh(geomLODLevel, node->GetNodeIndex());
            //EMotionFX::Mesh*      collisionMesh   = actor->GetCollisionMesh(geomLODLevel, node->GetNodeIndex() );
            const MCore::Matrix&    worldTM         = worldMatrices[ node->GetNodeIndex() ];

            renderUtil->ResetCurrentMesh();

            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->GetIsCollisionMesh() == false)
            {
                renderUtil->RenderNormals(mesh, worldTM, gRenderVertexNormals, gRenderFaceNormals);
                if (gRenderTangents == true)
                {
                    renderUtil->RenderTangents(mesh, worldTM);
                }
                if (gRenderWireframe == true)
                {
                    renderUtil->RenderWireframe(mesh, worldTM, MCore::RGBAColor(1.0f, 1.0f, 1.0f));
                }
            }
            else
            if (gRenderCollisionMeshes == true)
            {
                renderUtil->RenderWireframe(mesh, worldTM);
            }
        }
    }
}


// update all visibility flags
// this is where you do the frustum cull and occlusion tests etc
void UpdateVisibilityFlags()
{
    uint32 num = 0;

    // iterate over all actor instances
    const EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
    const uint32 numActorInstances = actorManager.GetNumRootActorInstances();
    for (uint32 i = 0; i < numActorInstances; ++i)
    {
        EMotionFX::ActorInstance* actorInstance = actorManager.GetRootActorInstance(i);
        const bool isVisible = gCamera->GetFrustum().PartiallyContains(actorInstance->GetAABB());   // test the bounding box against the camera view frustum
        actorInstance->SetIsVisible(isVisible);
    }
}


// render the actor instances
void RenderActorInstances(float timePassedInSeconds)
{
    // iterate over all actor instances
    const EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
    const uint32 numActorInstances = actorManager.GetNumActorInstances();
    for (uint32 i = 0; i < numActorInstances; ++i)
    {
        // check if its visible and if we really want to render this one, if not skip rendering
        EMotionFX::ActorInstance* actorInstance = actorManager.GetActorInstance(i);
        if (actorInstance->GetIsVisible() == false || actorInstance->GetRender() == false || actorInstance->GetIsEnabled() == false)
        {
            continue;
        }

        // update meshes that are still deformed on the cpu
        // you generally do not want to do this, but in our example rendering we force skinning and morphing to the cpu when there is a morphed mesh
        // this is obviously very slow, as its also not multithreaded now
        actorInstance->UpdateMeshDeformers(timePassedInSeconds);

        // get the glActor from the custom data pointer
        RenderGL::GLActor* glActor = static_cast<RenderGL::GLActor*>(actorInstance->GetCustomData());
        glActor->Render(actorInstance);

        //gRenderUtil->RenderAABB( actorInstance->GetAABB(), MCore::RGBAColor(0.0f, 1.0f, 1.0f) );
    }

    //if (gOrbitCamera != nullptr)
    //{
    //MCore::Vector3 target = gOrbitCamera->GetTarget();
    //MCore::LogInfo("gOrbitCamera->Set(%.2ff, %.2ff, %.2ff, MCore::Vector3(%.2ff, %.2ff, %.2ff));", gOrbitCamera->GetAlpha(), gOrbitCamera->GetBeta(), gOrbitCamera->GetCurrentDistance(), target.x, target.y, target.z);
    //}
}


// initialize EMotion FX
bool InitEMotionFX()
{
    // try to initialize MCore
    // we use custom memory allocation functions here, which also registers the allocations at our memory tracker (see MCore::MemoryTracker)
    // the tracking is used to get some memory usage statistics
    // you can also leave all on its default values to just use malloc/realloc/free without memory tracking or custom allocators
    MCore::Initializer::InitSettings coreSettings;
    //coreSettings.mMemAllocFunction    = gMemInfoAllocate;
    //coreSettings.mMemReallocFunction  = gMemInfoRealloc;
    //coreSettings.mMemFreeFunction     = gMemInfoFree;
    coreSettings.mJobExecutionFunction  = MCore::JobListExecuteMCoreJobSystem;  // use the MCore job system to execute jobs, other options are JobListExecuteOpenMP (using OpenMP) and JobListExecuteSerial (no multi threading at all), or a custom job execute function
    //coreSettings.mNumThreads          = 4;    // init with a given amount of threads, you can also use EMotionFX::GetEMotionFX().SetNumThreads(...) to adjust this later on. on default this uses the number of logical processors available
    // if you set this to 0, no threads will be created by the MCore job system. However if set to 0, you should also not use the MCore::JobListExecuteMCoreJobSystem function as job execution function.
    //#ifdef MCORE_DEBUG
    coreSettings.mTrackMemoryUsage  = true;     // disabled on default, when enabled every allocation/reallocation and free will be passed through a memory tracker which can show mem usage statistics (at the cost of some performance and memory usage)
    //#endif

    if (MCore::Initializer::Init(&coreSettings) == false)   // you could just leave the coreSettings parameter away to use defaults
    {
        return false;   // probably a license issue if this fails
    }
    // create the log file callback
    MCore::GetLogManager().CreateLogFile("ExampleLog.txt");

    // you can initialize to a different coordinate system here
    // all data inside EMotion FX will be in this coordinate system
    // on default we use the left handed DirectX style system, where x+ points right, y+ points up and z+ points into the depth
    //MCore::GetCoordinateSystem().InitFor3DStudioMax();        // Swap Y and Z (x+ right, y+ into depth, z+ up), right handed
    //MCore::GetCoordinateSystem().InitForMaya();               // OpenGL style (x+ right, y+ up, z- into depth), right handed
    //MCore::GetCoordinateSystem().Init( MCore::Vector3(1.0f, 0.0f, 0.0f), MCore::Vector3(0.0f, 0.0f, 1.0f), MCore::Vector3(0.0f, 1.0f, 0.0f), 0, 2, 1 );   // this would be the same InitFor3DStudioMax(), but you can customize it to your own system if you use a non-common one

    // try to init EMotion FX
    EMotionFX::Initializer::InitSettings emfxSettings;
    emfxSettings.mCoordinateSystem  = EMotionFX::Initializer::COORDSYSTEM_LEFTHANDED_Y_UP;      // choose your coordinate system (the left handed y-up is the native one of EMFX)
    emfxSettings.mUnitType          = MCore::Distance::UNITTYPE_METERS;                         // 1 unit is 1 meter
    if (EMotionFX::Initializer::Init(&emfxSettings) == false)
    {
        // probably a license issue if this fails
        MCore::Initializer::Shutdown();
        return false;
    }

    // if you later want to adjust the number of threads EMFX should use, call this (you can call this whenever you want after init, but don't call it every frame)
    //EMotionFX::GetEMotionFX().SetNumThreads( 6 );

    // if you do not use the MCore job system, but a custom one, tell MCore's job manager to stop processing any jobs (this actually stops the threads from running and looking for jobs)
    // quit any existing threads
    //MCore::GetJobManager().StopProcessingJobs();

    // register memory categories, this is optional and not directly part of the EMotion FX init
    // all this does is link a string based name to the memory category IDs
    MCore::GetMemoryTracker().RegisterCategory(MEMCATEGORY_RENDERING,  "RenderGL");
    MCore::GetMemoryTracker().RegisterCategory(MEMCATEGORY_MCOMMON,    "MCommon");
    return true;
}


// shutdown EMotion FX
void ShutdownEMotionFX()
{
    // first shutdown EMotion FX then MCore, it must happen in this order
    EMotionFX::Initializer::Shutdown();
    MCore::Initializer::Shutdown();
    // be sure that after this point there are no MCore::String or MCore::Array's or so anymore that still have memory allocated
}

// include the GUI system
#include "GUI.h"

#ifdef MCORE_PLATFORM_WINDOWS
    #include "Win32AppEntry.h"
#endif
