//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "Misc/FrameworkWindows.h"
#include "Misc/Camera.h"
#include "EngineInterface.h"
#include "GLTF/GltfCommon.h"

#include "TressFXAsset.h"
#include "AMD_Types.h"
#include "AMD_TressFX.h"
#include "TressFXCommon.h"
#include "TressFXSDFCollision.h"
#include "TressFXSettings.h"

#include <vector>

class TressFXPPLL;
class TressFXShortCut;
class CollisionMesh;
class HairStrands;
class Simulation;
class EI_Scene;

// scene description - no live objects
struct TressFXObjectDescription
{
    std::string name;
    std::string tfxFilePath;
    std::string tfxBoneFilePath;
    std::string hairObjectName;
    int numFollowHairs;
    float tipSeparationFactor;
    int mesh;
    TressFXSimulationSettings initialSimulationSettings;
    TressFXRenderingSettings initialRenderingSettings;
};

struct TressFXCollisionMeshDescription
{
    std::string name;
    std::string tfxMeshFilePath;
    int         numCellsInXAxis;
    float       collisionMargin;
    int         mesh;
    std::string followBone;
};

struct TressFXSceneDescription
{
    std::vector<TressFXObjectDescription>           objects;
    std::vector<TressFXCollisionMeshDescription>    collisionMeshes;

    std::string gltfFilePath;
    std::string gltfFileName;
    std::string gltfBonePrefix;

    float startOffset;
};

// in-memory scene
struct TressFXObject
{
    std::unique_ptr<HairStrands> hairStrands;
    TressFXSimulationSettings simulationSettings;
    TressFXRenderingSettings renderingSettings;
    std::string name;
};

struct TressFXScene
{
    std::vector<TressFXObject> objects;
    std::vector<std::unique_ptr<CollisionMesh>> collisionMeshes;

    TressFXUniformBuffer<TressFXViewParams> viewConstantBuffer;
    std::unique_ptr<EI_BindSet> viewBindSet;

    TressFXUniformBuffer<TressFXViewParams> shadowViewConstantBuffer;
    std::unique_ptr<EI_BindSet> shadowViewBindSet;

    TressFXUniformBuffer<TressFXLightParams> lightConstantBuffer;
    std::unique_ptr<EI_BindSet> lightBindSet;

    std::unique_ptr<EI_Scene> scene;
};

class TressFXSample : public FrameworkWindows
{
public:
    TressFXSample(LPCSTR name);

    void LoadScene(TressFXSceneDescription& desc);

    void DrawHairShadows();

    void OnCreate(HWND hWnd);
    void OnDestroy();
    void OnRender();
    bool OnEvent(MSG msg);
    void OnResize(uint32_t Width, uint32_t Height);
    void RecreateSizeDependentResources();
    void SetFullScreen(bool fullscreen);
    void DrawHair();
    // Simulation and collision
    void Simulate(double fTime, bool bUpdateCollMesh, bool bSDFCollisionResponse, bool bAsyncCompute);
    void WaitSimulateDone();
    void SetSDFCollisionMargin(float collisionMargin);
    void UpdateSimulationParameters();
    void UpdateRenderingParameters();
    void UpdateRenderShadowParameters(AMD::float4& CameraPos);
    void ToggleShortCut();

    // debug drawing
    void DrawCollisionMesh();
    void DrawSDFGrid();
    void DrawSDF();
    void GenerateMarchingCubes();
    void SetSDFIsoLevel(float isoLevel);

    void LoadScene(int sceneNumber);

    int       GetNumTressFXObjects() { return static_cast<int>(m_activeScene.objects.size()); }
private:
    void DisplayRenderingParameters(const char* name, TressFXHairObject * object, TressFXRenderingSettings* RenderSettings);
    void DisplaySimulationParameters(const char* name, TressFXSimulationSettings* simulationSettings);

    // available scene descriptions (not necessary in memory)
    std::vector<TressFXSceneDescription> m_scenes;
    TressFXScene m_activeScene;

    std::unique_ptr<TressFXPPLL>        m_pPPLL;
    std::unique_ptr<TressFXShortCut>    m_pShortCut;
    std::unique_ptr<Simulation>         m_pSimulation;

    std::unique_ptr<EI_RenderTargetSet> m_pGLTFRenderTargetSet = nullptr;
    std::unique_ptr<EI_RenderTargetSet> m_pShadowRenderTargetSet = nullptr;
    std::unique_ptr<EI_RenderTargetSet> m_pDebugRenderTargetSet = nullptr;

    std::unique_ptr<EI_PSO>             m_pHairShadowPSO = nullptr;

    enum OITMethod
    {
        OIT_METHOD_PPLL,
        OIT_METHOD_SHORTCUT
    };

    OITMethod m_eOITMethod;
    int       m_nScreenWidth;
    int       m_nScreenHeight;
    int       m_nPPLLNodes;

    void InitializeLayouts();
    void DestroyLayouts();
    void SetOITMethod(OITMethod method);
    void DestroyOITResources(OITMethod method);

    float   m_time;             // WallClock in seconds.
    float   m_deltaTime;        // The elapsed time in milliseconds since the previous frame.
    double  m_lastFrameTime;
    bool    m_PauseAnimation = false;
    bool    m_PauseSimulation = false;
    bool    m_AsyncCompute = false;
    bool    m_drawHair = true;
    bool    m_drawModel = true;
    bool    m_drawCollisionMesh = false;
    bool    m_drawMarchingCubes = false;
    bool    m_generateSDF = true;
    bool    m_collisionResponse = true;
    bool	m_useDepthApproximation = true;
    bool    m_vSync = false;
};
