// ----------------------------------------------------------------------------
// Brings together all the TressFX components.
// ----------------------------------------------------------------------------
//
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

#include "stdafx.h"

#include "TressFXSample.h"

#include "TressFXPPLL.h"
#include "TressFXShortCut.h"

#include "AMD_TressFX.h"

#include "TressFXLayouts.h"

#include "HairStrands.h"
#include "SDF.h"
#include "Simulation.h"
#include "Base/Imgui.h"
#include "Base/ImguiHelper.h"
#include "Misc/Misc.h"

#include "EngineInterface.h"

#include <Windows.h>
#include <commdlg.h>

const bool VALIDATION_ENABLED = false;
const uint32_t NUMBER_OF_BACK_BUFFERS = 3;

// This could instead be retrieved as a variable from the
// script manager, or passed as an argument.
static const size_t AVE_FRAGS_PER_PIXEL = 12;
static const size_t PPLL_NODE_SIZE = 16;

TressFXSample::TressFXSample(LPCSTR name)
    : FrameworkWindows(name),
    m_eOITMethod(OIT_METHOD_SHORTCUT), // OIT_METHOD_PPLL
    m_nScreenWidth(0),
    m_nScreenHeight(0),
    m_nPPLLNodes(0)
{
    m_lastFrameTime = 0.0f;
    m_time = 0;
    m_deltaTime = 0.0f;
    m_pShortCut = nullptr;
    m_pPPLL = nullptr;
    m_pSimulation = nullptr;
}

void TressFXSample::Simulate(double fTime, bool bUpdateCollMesh, bool bSDFCollisionResponse, bool bAsyncCompute)
{
    SimulationContext ctx;
    ctx.hairStrands.resize(m_activeScene.objects.size());
    ctx.collisionMeshes.resize(m_activeScene.collisionMeshes.size());
    for (size_t i = 0; i < m_activeScene.objects.size(); ++i)
    {
        ctx.hairStrands[i] = m_activeScene.objects[i].hairStrands.get();
    }
    for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); ++i)
    {
        ctx.collisionMeshes[i] = m_activeScene.collisionMeshes[i].get();
    }
    m_pSimulation->StartSimulation(fTime, ctx, bUpdateCollMesh, bSDFCollisionResponse, bAsyncCompute);
}

void TressFXSample::WaitSimulateDone()
{
    m_pSimulation->WaitOnSimulation();
}

void TressFXSample::ToggleShortCut()
{
    OITMethod newMethod;
    if (m_eOITMethod == OIT_METHOD_PPLL)
        newMethod = OIT_METHOD_SHORTCUT;
    else
        newMethod = OIT_METHOD_PPLL;
    SetOITMethod(newMethod);
}

void TressFXSample::DrawCollisionMesh()
{
    EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();
    for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); i++)
        m_activeScene.collisionMeshes[i]->DrawMesh(commandList);
}

void TressFXSample::GenerateMarchingCubes()
{
#if ENABLE_MARCHING_CUBES
    EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();
    for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); i++)
        m_activeScene.collisionMeshes[i]->GenerateIsoSurface(commandList);
#endif
}

void TressFXSample::DrawSDF()
{
#if ENABLE_MARCHING_CUBES
    EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();

    for (size_t i = 0; i < m_activeScene.collisionMeshes.size(); i++)
        m_activeScene.collisionMeshes[i]->DrawIsoSurface(commandList);
#endif
}

void TressFXSample::InitializeLayouts()
{
    InitializeAllLayouts(GetDevice());
}

void TressFXSample::DestroyLayouts()
{
    DestroyAllLayouts(GetDevice());
}

// TODO: Loading asset function should be called from OnAssetReload?
void TressFXSample::LoadScene(TressFXSceneDescription& desc)
{
    // Since GLTF is the first thing we render, we want to clear on load
    m_activeScene.scene->OnCreate(GetDevice(), m_pGLTFRenderTargetSet.get(), m_pShadowRenderTargetSet.get(), desc.gltfFilePath, desc.gltfFileName, desc.gltfBonePrefix, desc.startOffset);

    // TODO Why?
    DestroyLayouts();

    InitializeLayouts();
   
    m_pPPLL.reset(new TressFXPPLL);
    m_pShortCut.reset(new TressFXShortCut);
    m_pSimulation.reset(new Simulation);

    EI_Device* pDevice = GetDevice();

    // Used for uploading initial data
    EI_CommandContext& uploadCommandContext = pDevice->GetCurrentCommandContext();
    
    for (size_t i = 0; i < desc.objects.size(); ++i)
    {
        HairStrands* hair = new HairStrands(
            m_activeScene.scene.get(),
            desc.objects[i].tfxFilePath.c_str(),
            desc.objects[i].tfxBoneFilePath.c_str(),
            desc.objects[i].hairObjectName.c_str(),
            desc.objects[i].numFollowHairs,
            desc.objects[i].tipSeparationFactor,
            desc.objects[i].mesh, (int)i);
        hair->GetTressFXHandle()->PopulateDrawStrandsBindSet(GetDevice(), &desc.objects[i].initialRenderingSettings);
        m_activeScene.objects.push_back({ std::unique_ptr<HairStrands>(hair), desc.objects[i].initialSimulationSettings, desc.objects[i].initialRenderingSettings, desc.objects[i].name.c_str() });
    }

    for (size_t i = 0; i < desc.collisionMeshes.size(); ++i)
    {
        CollisionMesh* mesh = new CollisionMesh(
            m_activeScene.scene.get(), m_pDebugRenderTargetSet.get(),
            desc.collisionMeshes[i].name.c_str(),
            desc.collisionMeshes[i].tfxMeshFilePath.c_str(),
            desc.collisionMeshes[i].numCellsInXAxis,
            desc.collisionMeshes[i].collisionMargin,
            desc.collisionMeshes[i].mesh,
            desc.collisionMeshes[i].followBone.c_str()
        );
        m_activeScene.collisionMeshes.push_back(std::unique_ptr<CollisionMesh>(mesh));
    }
 }

void TressFXSample::UpdateSimulationParameters()
{
    for (int i = 0; i < m_activeScene.objects.size(); ++i)
    {
        m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdateSimulationParameters(&m_activeScene.objects[i].simulationSettings, m_deltaTime);
    }
}

void TressFXSample::UpdateRenderingParameters()
{
    std::vector<const TressFXRenderingSettings*> RenderSettings;
    for (int i = 0; i < m_activeScene.objects.size(); ++i)
    {
        // For now, just using distance of camera to 0, 0, 0, but should be passing in a root position for the hair object we want to LOD
        float Distance = sqrtf(m_activeScene.scene->GetCameraPos().x * m_activeScene.scene->GetCameraPos().x + m_activeScene.scene->GetCameraPos().y * m_activeScene.scene->GetCameraPos().y + m_activeScene.scene->GetCameraPos().z * m_activeScene.scene->GetCameraPos().z);
        m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdateRenderingParameters(&m_activeScene.objects[i].renderingSettings, m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL, m_deltaTime, Distance);
        RenderSettings.push_back(&m_activeScene.objects[i].renderingSettings);
    }

    // Update shade parameters for correct implementation
    switch (m_eOITMethod)
    {
    case OIT_METHOD_SHORTCUT:
        m_pShortCut->UpdateShadeParameters(RenderSettings);
        break;
    case OIT_METHOD_PPLL:
        m_pPPLL->UpdateShadeParameters(RenderSettings);
        break;
    default:
        break;
    }
}

void TressFXSample::UpdateRenderShadowParameters(AMD::float4& CameraPos)
{
    for (int i = 0; i < m_activeScene.objects.size(); ++i)
    {
        // For now, just using distance of camera to 0, 0, 0, but should be passing in a root position for the hair object we want to LOD
        float Distance = sqrtf(CameraPos.x * CameraPos.x + CameraPos.y * CameraPos.y + CameraPos.z * CameraPos.z);
        m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdateRenderingParameters(&m_activeScene.objects[i].renderingSettings, m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL, m_deltaTime, Distance, true);
    }
}

void TressFXSample::OnResize(uint32_t width, uint32_t height)
{
    if (m_nScreenWidth != width || m_nScreenHeight != height)
    {
        m_nScreenWidth = width;
        m_nScreenHeight = height;
        RecreateSizeDependentResources();
    }
    EI_Device* pDevice = GetDevice();

    // Create PSO for the hair shadow pass (doesn't matter if it's shortcut or PPLL) if it hasn't been done yet
    if (!m_pHairShadowPSO)
    {
        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleList;
        psoParams.colorWriteEnable = false;
        psoParams.depthTestEnable = true;
        psoParams.depthWriteEnable = true;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;
        psoParams.colorBlendParams.colorBlendEnabled = false;

        EI_BindLayout* ShadowPassLayouts[] = { GetTressFXParamLayout(), GetRenderPosTanLayout(), GetShadowViewLayout() };
        psoParams.layouts = ShadowPassLayouts;
        psoParams.numLayouts = 3;
        psoParams.renderTargetSet = m_pShadowRenderTargetSet.get();

        m_pHairShadowPSO = GetDevice()->CreateGraphicsPSO("TressFXShadow.hlsl", "HairShadowVS", "TressFXShadow.hlsl", "HairShadowPS", psoParams);
    }
}

void TressFXSample::RecreateSizeDependentResources()
{
    GetDevice()->FlushGPU();
    GetDevice()->OnResize(m_nScreenWidth, m_nScreenHeight);

    // Whenever there is a resize, we need to re-create on render pass set as it depends on
    // the main color/depth buffers which get resized
    const EI_Resource* ResourceArray[] = { GetDevice()->GetColorBufferResource(), GetDevice()->GetDepthBufferResource() };
    m_pGLTFRenderTargetSet->SetResources(ResourceArray);
    m_pDebugRenderTargetSet->SetResources(ResourceArray);

    m_activeScene.scene->OnResize(m_nScreenWidth, m_nScreenHeight);

    // Needs to be created in OnResize in case we have debug buffers bound which vary by screen width/height
    EI_BindSetDescription lightSet =
    {
        { m_activeScene.lightConstantBuffer.GetBufferResource(), GetDevice()->GetShadowBufferResource() }
    };
    m_activeScene.lightBindSet = GetDevice()->CreateBindSet(GetLightLayout(), lightSet);

    // TressFX Usage
    switch (m_eOITMethod)
    {
    case OIT_METHOD_PPLL:
        m_nPPLLNodes = m_nScreenWidth * m_nScreenHeight * AVE_FRAGS_PER_PIXEL;
        m_pPPLL.reset(new TressFXPPLL);
        m_pPPLL->Initialize(m_nScreenWidth, m_nScreenHeight, m_nPPLLNodes, PPLL_NODE_SIZE);
        break;
    case OIT_METHOD_SHORTCUT:
        m_pShortCut.reset(new TressFXShortCut);
        m_pShortCut->Initialize(m_nScreenWidth, m_nScreenHeight);
        break;
    };
}

void TressFXSample::DrawHairShadows()
{
    int numHairStrands = (int)m_activeScene.objects.size();
    std::vector<HairStrands*> hairStrands(numHairStrands);
    EI_BindSet* ExtraBindSets[] = { m_activeScene.shadowViewBindSet.get() };

    for (int i = 0; i < numHairStrands; ++i)
    {
        hairStrands[i] = m_activeScene.objects[i].hairStrands.get();
        if (hairStrands[i]->GetTressFXHandle())
                hairStrands[i]->GetTressFXHandle()->DrawStrands(GetDevice()->GetCurrentCommandContext(), *m_pHairShadowPSO.get(), ExtraBindSets, 1);
    }
    EI_CommandContext& pRenderCommandList = GetDevice()->GetCurrentCommandContext();
}

void TressFXSample::DrawHair()
{
    int numHairStrands = (int)m_activeScene.objects.size();
    std::vector<HairStrands*> hairStrands(numHairStrands);
    for (int i = 0; i < numHairStrands; ++i)
    {
        hairStrands[i] = m_activeScene.objects[i].hairStrands.get();
    }

    EI_CommandContext& pRenderCommandList = GetDevice()->GetCurrentCommandContext();

    switch (m_eOITMethod)
    {
    case OIT_METHOD_PPLL:
        m_pPPLL->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
        break;
    case OIT_METHOD_SHORTCUT:
        m_pShortCut->Draw(pRenderCommandList, numHairStrands, hairStrands.data(), m_activeScene.viewBindSet.get(), m_activeScene.lightBindSet.get());
        break;
    };

    for (size_t i = 0; i < hairStrands.size(); i++)
        hairStrands[i]->TransitionRenderingToSim(pRenderCommandList);
}

void TressFXSample::SetOITMethod(OITMethod method)
{
    if (method == m_eOITMethod)
        return;

    // Flush the GPU before switching
    GetDevice()->FlushGPU();

    // Destroy old resources
    DestroyOITResources(m_eOITMethod);

    m_eOITMethod = method;
    RecreateSizeDependentResources();
}

void TressFXSample::DestroyOITResources(OITMethod method)
{
    // TressFX Usage
    switch (m_eOITMethod)
    {
    case OIT_METHOD_PPLL:
        m_pPPLL.reset();
        break;
    case OIT_METHOD_SHORTCUT:
        m_pShortCut.reset();
        break;
    };
}

void TressFXSample::OnCreate(HWND hWnd)
{
    GetDevice()->SetVSync(m_vSync);
    GetDevice()->OnCreate(hWnd, NUMBER_OF_BACK_BUFFERS, VALIDATION_ENABLED, "TressFX");

    m_activeScene.scene.reset(new EI_Scene);

    // Create a renderpass for GLTF (needed for PSO creation)
    const EI_ResourceFormat FormatArray[] = { GetDevice()->GetColorBufferFormat(), GetDevice()->GetDepthBufferFormat() };
    {
        const EI_AttachmentParams AttachmentParams[] = { { EI_RenderPassFlags::Load | EI_RenderPassFlags::Clear | EI_RenderPassFlags::Store },
                                                         { EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Clear | EI_RenderPassFlags::Store } };
        float ClearValues[] = { 0.f, 0.f, 0.f, 0.f, // Color
                                1.f, 0.f };         // Depth

        m_pGLTFRenderTargetSet = GetDevice()->CreateRenderTargetSet(FormatArray, 2, AttachmentParams, ClearValues);
    }

    // Create a renderpass for shadow rendering (needed for PSO creation)
    {
        const EI_Resource* ResourceArray[] = { GetDevice()->GetShadowBufferResource() };
        const EI_AttachmentParams AttachmentParams[] = { { EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Clear | EI_RenderPassFlags::Store } };
        float ClearValues[] = { 1.f, 0.f };         // Depth Clear
        m_pShadowRenderTargetSet = GetDevice()->CreateRenderTargetSet(ResourceArray, 1, AttachmentParams, ClearValues);
    }
    // Create a debug render pass
    {
        const EI_AttachmentParams AttachmentParams[] = { { EI_RenderPassFlags::Load | EI_RenderPassFlags::Store },
                                                         { EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Store } };

        m_pDebugRenderTargetSet = GetDevice()->CreateRenderTargetSet(FormatArray, 2, AttachmentParams, nullptr);
    }

    // init GUI (non gfx stuff)
    ImGUI_Init((void *)hWnd);
    LoadScene(0);

    m_activeScene.viewConstantBuffer.CreateBufferResource("viewConstants");
    EI_BindSetDescription set = { { m_activeScene.viewConstantBuffer.GetBufferResource() } };
    m_activeScene.viewBindSet = GetDevice()->CreateBindSet(GetViewLayout(), set);

    m_activeScene.shadowViewConstantBuffer.CreateBufferResource("shadowViewConstants");
    EI_BindSetDescription shadowSet = { { m_activeScene.shadowViewConstantBuffer.GetBufferResource() } };
    m_activeScene.shadowViewBindSet = GetDevice()->CreateBindSet(GetViewLayout(), shadowSet);

    m_activeScene.lightConstantBuffer.CreateBufferResource("LightConstants");

    GetDevice()->EndAndSubmitCommandBuffer();
    GetDevice()->FlushGPU();
}

void TressFXSample::LoadScene(int sceneNumber)
{
    TressFXSceneDescription sds[2];
    {
        sds[0].gltfFilePath = "../../Assets/Objects/RatBoy/";
        sds[0].gltfFileName = "babylon.gltf";
        sds[0].gltfBonePrefix = "";
        sds[0].startOffset = 2.3f;

        // initialize settings with default settings
        TressFXSimulationSettings mohawkSettings;
        TressFXSimulationSettings furSettings;
        mohawkSettings.m_vspCoeff = 0.758f;
        mohawkSettings.m_vspAccelThreshold = 1.208f;
        mohawkSettings.m_localConstraintStiffness = 0.908f;
        mohawkSettings.m_localConstraintsIterations = 3;
        mohawkSettings.m_globalConstraintStiffness = 0.408f;
        mohawkSettings.m_globalConstraintsRange = 0.308f;
        mohawkSettings.m_lengthConstraintsIterations = 3;
        mohawkSettings.m_damping = 0.068f;
        mohawkSettings.m_gravityMagnitude = 0.09f;
        furSettings.m_vspCoeff = 0.758f;
        furSettings.m_vspAccelThreshold = 1.208f;
        furSettings.m_localConstraintStiffness = 0.908f;
        furSettings.m_localConstraintsIterations = 2;
        furSettings.m_globalConstraintStiffness = 0.408f;
        furSettings.m_globalConstraintsRange = 0.308f;
        furSettings.m_lengthConstraintsIterations = 2;
        furSettings.m_damping = 0.068f;
        furSettings.m_gravityMagnitude = 0.09f;

        TressFXRenderingSettings mohawkRenderSettings;
        mohawkRenderSettings.m_BaseAlbedoName = "..\\..\\Assets\\Objects\\RatBoy\\ratBoySubstanceReady_main_BaseColor.png";
        mohawkRenderSettings.m_EnableThinTip = true;
        mohawkRenderSettings.m_FiberRadius = 0.002f;
        mohawkRenderSettings.m_FiberRatio = 0.06f;
        mohawkRenderSettings.m_HairKDiffuse = 0.22f;
        mohawkRenderSettings.m_HairKSpec1 = 0.012f;
        mohawkRenderSettings.m_HairSpecExp1 = 14.40f;
        mohawkRenderSettings.m_HairKSpec2 = 0.136f;
        mohawkRenderSettings.m_HairSpecExp2 = 11.80f;

        TressFXRenderingSettings furRenderSettings;
        furRenderSettings.m_BaseAlbedoName = "..\\..\\Assets\\Objects\\RatBoy\\ratBoySubstanceReady_main_BaseColor.png";
        furRenderSettings.m_EnableThinTip = true;
        furRenderSettings.m_FiberRadius = 0.001f;
        furRenderSettings.m_FiberRatio = 0.16f;
        furRenderSettings.m_HairKDiffuse = 0.22f;
        furRenderSettings.m_HairKSpec1 = 0.02f;
        furRenderSettings.m_HairSpecExp1 = 14.40f;
        furRenderSettings.m_HairKSpec2 = 0.3f;
        furRenderSettings.m_HairSpecExp2 = 11.80f;
        furRenderSettings.m_HairShadowAlpha = 0.034f;

        TressFXObjectDescription mohawkDesc =
        {
            "Mohawk",
            "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_mohawk.tfx",
            "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_mohawk.tfxbone",
            "mohawk",
            2, // This is number of follow hairs per one guide hair. It could be zero if there is no follow hair at all. 
            2.0f,
            0, // mesh number
            mohawkSettings,
            mohawkRenderSettings
        };

        TressFXObjectDescription furDesc =
        {
            "Fur",
            "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_short.tfx",
            "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_short.tfxbone",
            "hairShort",
            1, // Filling out a little more fur.
            1.0f,
            0,
            furSettings,
            furRenderSettings
        };
        sds[0].objects.push_back(mohawkDesc);
        sds[0].objects.push_back(furDesc);

        TressFXCollisionMeshDescription collisionMeshBody =
        {
            "RatBoy_body",
            "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_body.tfxmesh",
             50,
             0.0f,
             0,
             "frenchHornMonster_root_JNT"
        };
        TressFXCollisionMeshDescription collisionMeshLeftHand =
        {
            "RatBoy_left_hand",
             "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_left_hand.tfxmesh",
             32,
             0.5f,
             0,
             "frenchHornMonster_L_LowerArm_JNT"
        };
        TressFXCollisionMeshDescription collisionMeshRightHand =
        {
            "RatBoy_right_hand",
             "..\\..\\Assets\\Objects\\HairAsset\\Ratboy\\Ratboy_right_hand.tfxmesh",
             32,
             0.5f,
             0,
             "frenchHornMonster_R_LowerArm_JNT"
        };
        sds[0].collisionMeshes.push_back(collisionMeshBody);
        sds[0].collisionMeshes.push_back(collisionMeshLeftHand);
        sds[0].collisionMeshes.push_back(collisionMeshRightHand);
    }
    LoadScene(sds[sceneNumber]);
}

void TressFXSample::OnDestroy()
{
    // Get everything out of the pipeline before we start nuking everything.
    GetDevice()->FlushGPU();

    // Destroy hair resources based on what method we are using
    DestroyOITResources(m_eOITMethod);

    EI_Device* pDevice = GetDevice();
    m_activeScene.collisionMeshes.clear();
    m_activeScene.objects.clear();
    m_activeScene.scene.reset();
    m_activeScene.viewConstantBuffer.Reset();
    m_activeScene.viewBindSet.reset();
    m_activeScene.shadowViewConstantBuffer.Reset();
    m_activeScene.shadowViewBindSet.reset();
    m_activeScene.lightConstantBuffer.Reset();
    m_activeScene.lightBindSet.reset();

    m_pPPLL.reset();
    m_pShortCut.reset();
    m_pSimulation.reset();

    m_pGLTFRenderTargetSet.reset();
    m_pDebugRenderTargetSet.reset();
    m_pShadowRenderTargetSet.reset();

    m_pHairShadowPSO.reset();

    DestroyLayouts();

    // Need to properly shut everything down
    GetDevice()->OnDestroy();
}

bool TressFXSample::OnEvent(MSG msg)
{
    if (ImGUI_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam))
        return true;
    return true;
}

void TressFXSample::SetFullScreen(bool fullscreen)
{
}

void TressFXSample::DisplaySimulationParameters(const char* name, TressFXSimulationSettings* simulationSettings)
{
    // Rendering params
    bool opened = true;
    std::string title = std::string(name) + " Simulation Parameters";
    {
        ImGui::SliderFloat("VSP Coefficient", &simulationSettings->m_vspCoeff, 0.0f, 1.0f);
        ImGui::SliderFloat("VSP Threshold", &simulationSettings->m_vspAccelThreshold, 0.0f, 1.0f);

        ImGui::SliderFloat("Damping", &simulationSettings->m_damping, 0.0f, 1.0f);
        ImGui::SliderFloat("Local Constraint Stiffness", &simulationSettings->m_localConstraintStiffness, 0.0f, 1.0f);
        ImGui::SliderInt("Local Constraint Iterations", &simulationSettings->m_localConstraintsIterations, 1, 4);
        ImGui::SliderFloat("Global Constraints Stiffness", &simulationSettings->m_globalConstraintStiffness, 0.0f, 1.0f);
        ImGui::SliderFloat("Global Constraints Range", &simulationSettings->m_globalConstraintsRange, 0.0f, 1.0f);

        ImGui::SliderFloat("Gravity Magnitude", &simulationSettings->m_gravityMagnitude, 0.0f, 1.0f);
        ImGui::SliderFloat("Tip Separation", &simulationSettings->m_tipSeparation, 0.0f, 2.0f);
        ImGui::SliderFloat("Clamp Position Delta", &simulationSettings->m_clampPositionDelta, 0.0f, 20.0f);
    }
}

bool TextureSelectionButton(char* Title, std::string& DisplayString)
{
    bool HasChanged = false;

    if (ImGui::Button(Title))
    {
        // Popup windows file selection dialog      
        OPENFILENAME ofn;
        char fileName[1024];
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = fileName;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(fileName);
        ofn.lpstrFilter = "Images\0*.png\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileName(&ofn))
        {
            // If we got a valid file name, try to load in the texture
            DisplayString = fileName;
            HasChanged = true;
        }
    }
    ImGui::SameLine(); ImGui::Text(DisplayString.c_str());

    return HasChanged;
}

void TressFXSample::DisplayRenderingParameters(const char* name, TressFXHairObject * object, TressFXRenderingSettings* RenderSettings)
{
    // Rendering params
    bool opened = true;
    std::string title = std::string(name) + " Rendering Parameters";
    {
        // Geometry
        if (ImGui::CollapsingHeader("Geometry Params", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
        {
            ImGui::Checkbox("Enable Hair LOD", &RenderSettings->m_EnableHairLOD);
            ImGui::SliderFloat("LOD Start Distance", &RenderSettings->m_LODStartDistance, 0.0f, 25.0f);
            ImGui::SliderFloat("LOD End Distance", &RenderSettings->m_LODEndDistance, 0.0f, 25.0f);
            ImGui::SliderFloat("LOD Strand Reduction", &RenderSettings->m_LODPercent, 0.0f, 1.0f);
            ImGui::SliderFloat("LOD Width Multiplier", &RenderSettings->m_LODWidthMultiplier, 1.0f, 5.0f);

            ImGui::SliderFloat("Fiber Radius", &RenderSettings->m_FiberRadius, 0.0005f, 0.005f);
            ImGui::Checkbox("Enable Thin Tip", &RenderSettings->m_EnableThinTip);
            ImGui::SliderFloat("Fiber Ratio", &RenderSettings->m_FiberRatio, 0.0f, 1.0f);
        }
        // Shading
        if (ImGui::CollapsingHeader("Shading", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
        {
            ImGui::ColorEdit4("Hair Base Color", RenderSettings->m_HairMatBaseColor.v, ImGuiColorEditFlags__OptionsDefault);
            ImGui::ColorEdit4("Hair Tip Color", RenderSettings->m_HairMatTipColor.v, ImGuiColorEditFlags__OptionsDefault);
            ImGui::SliderFloat("Tip Percentage", &RenderSettings->m_TipPercentage, 0.0f, 1.0f);
            ImGui::SliderFloat("Diffuse Factor", &RenderSettings->m_HairKDiffuse, 0.0f, 1.0f);
            ImGui::SliderFloat("Spec1 Factor", &RenderSettings->m_HairKSpec1, 0.0f, 1.0f);
            ImGui::SliderFloat("Spec Exponent 1", &RenderSettings->m_HairSpecExp1, 1.0f, 32.0f, "%.1f", 1.f);
            ImGui::SliderFloat("Spec2 Factor", &RenderSettings->m_HairKSpec2, 0.0f, 1.0f);
            ImGui::SliderFloat("Spec Exponent 2", &RenderSettings->m_HairSpecExp2, 1.0f, 32, "%.1f", 1.f);
        }
        // Shadowing
        if (ImGui::CollapsingHeader("Shadowing", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
        {
            ImGui::SliderFloat("Shadow Alpha", &RenderSettings->m_HairShadowAlpha, 0.0f, 1.0f);
            ImGui::SliderFloat("Fiber Spacing", &RenderSettings->m_HairFiberSpacing, 0.000001f, 1.f, "%.6f", 2.f);

            ImGui::Checkbox("Enable Hair Shadow LOD", &RenderSettings->m_EnableShadowLOD);
            ImGui::SliderFloat("Shadow LOD Start Distance", &RenderSettings->m_ShadowLODStartDistance, 0.0f, 25.0f);
            ImGui::SliderFloat("Shadow LOD End Distance", &RenderSettings->m_ShadowLODEndDistance, 0.0f, 25.0f);
            ImGui::SliderFloat("Shadow LOD Strand Reduction", &RenderSettings->m_ShadowLODPercent, 0.0f, 1.0f);
            ImGui::SliderFloat("Shadow LOD Width Multiplier", &RenderSettings->m_ShadowLODWidthMultiplier, 1.0f, 5.0f);
        }
        // Texturing
        if (ImGui::CollapsingHeader("Texturing", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
        {
            // Base Albedo texture picking
            if (TextureSelectionButton("Base Albedo Texture", RenderSettings->m_BaseAlbedoName))
            {
                GetDevice()->FlushGPU();
                object->PopulateDrawStrandsBindSet(GetDevice(), RenderSettings);
            }

            ImGui::Checkbox("Enable Strand UV", &RenderSettings->m_EnableStrandUV);
            ImGui::SliderFloat("Strand UV Tiling Factor", &RenderSettings->m_StrandUVTilingFactor, 0.0f, 30.0f);

            // Strand Albedo texture picking
            if (TextureSelectionButton("Strand Albedo Texture", RenderSettings->m_StrandAlbedoName))
            {
                GetDevice()->FlushGPU();
                object->PopulateDrawStrandsBindSet(GetDevice(), RenderSettings);
            }

            ImGui::Checkbox("Enable Strand Tangent", &RenderSettings->m_EnableStrandTangent);
        }
    }
}

void TressFXSample::OnRender()
{
    // Get timings
    double timeNow = MillisecondsNow();
    if (m_lastFrameTime == 0.0f)
    {
        m_lastFrameTime = timeNow;
    }
    m_deltaTime = float(timeNow - m_lastFrameTime) / 1000.0f;
    m_lastFrameTime = timeNow;
    m_time += m_deltaTime;
    // clamp deltatime so that sim doesnt freak out
    m_deltaTime = std::min(m_deltaTime, 0.05f);

    ImGUI_UpdateIO();
    ImGui::NewFrame();

    bool opened = false;
    ImGui::Begin("Menu", &opened);
    ImGui::Checkbox("Pause Animation", &m_PauseAnimation);
    ImGui::Checkbox("Pause Simulation", &m_PauseSimulation);
    ImGui::Checkbox("Draw Hair", &m_drawHair);
    ImGui::Checkbox("Draw Model", &m_drawModel);
    ImGui::Checkbox("Generate SDF", &m_generateSDF);
    ImGui::Checkbox("Collision Response", &m_collisionResponse);
    ImGui::Checkbox("Draw Collision Mesh", &m_drawCollisionMesh);
    ImGui::Checkbox("Draw Marching Cubes", &m_drawMarchingCubes);
    ImGui::Checkbox("Use Depth Approximation", &m_useDepthApproximation);
    if (ImGui::Checkbox("Use Vsync", &m_vSync))
    {
        GetDevice()->SetVSync(m_vSync);
        RecreateSizeDependentResources();
    }

    if (ImGui::Button("Reset Positions"))
    {
        for (int i = 0; i < m_activeScene.objects.size(); ++i)
        {
            m_activeScene.objects[i].hairStrands->GetTressFXHandle()->ResetPositions();
        }
    }

    const char * drawingControl[] = { "ShortCut", "PPLL" };
    static int drawingControlSelected = 0;
    int oldDrawingControlSelected = drawingControlSelected;
    ImGui::Combo("Drawing Method", &drawingControlSelected, drawingControl, _countof(drawingControl));
    if (drawingControlSelected != oldDrawingControlSelected) {
        ToggleShortCut();
    }

    GetDevice()->OnBeginFrame(m_AsyncCompute);

    // Set all the data to render out the scene
    m_activeScene.scene->OnBeginFrame(m_PauseAnimation ? 0.f : m_deltaTime, (float)m_nScreenWidth / m_nScreenHeight);

    std::vector<const char*> objectNames(m_activeScene.objects.size());
    for (int i = 0; i < m_activeScene.objects.size(); ++i)
    {
        objectNames[i] = m_activeScene.objects[i].name.c_str();
    }
    if (ImGui::CollapsingHeader("Rendering Parameters"))
    {
        static int objectSelected = 0;
        ImGui::Combo("Object", &objectSelected, objectNames.data(), (int)objectNames.size());
        DisplayRenderingParameters(m_activeScene.objects[objectSelected].name.c_str(), m_activeScene.objects[objectSelected].hairStrands->GetTressFXHandle(), &m_activeScene.objects[objectSelected].renderingSettings);
    }
    if (ImGui::CollapsingHeader("Simulation Parameters"))
    {
        static int objectSelected = 0;
        ImGui::Combo("Object", &objectSelected, objectNames.data(), (int)objectNames.size());
        DisplaySimulationParameters(m_activeScene.objects[objectSelected].name.c_str(), &m_activeScene.objects[objectSelected].simulationSettings);
    }

    if (ImGui::CollapsingHeader("Stats", 0)) {
        for (int i = 0; i < GetDevice()->GetNumTimeStamps(); ++i) {
            ImGui::Text("%s: %.1f ms\n", GetDevice()->GetTimeStampName(i), (float)GetDevice()->GetTimeStampValue(i) / 1000.0f);
        }
        ImGui::Text("%s: %.1f ms", "TotalGPUTime", (float)GetDevice()->GetAverageGpuTime() / 1000.0f);

        {
            static float values[16];
            for (uint32_t i = 0; i < 16 - 1; i++) { values[i] = values[i + 1]; }
            values[15] = 1.0f / m_deltaTime;
            float average = 0;
            for (uint32_t i = 0; i < 16; i++) { average += values[i]; }
            average /= 16.0f;

            ImGui::Text("Framerate: %.0f", average);
        }

    }
    ImGui::End();
    GetDevice()->GetTimeStamp("Gui Updates");

    UpdateSimulationParameters();
    m_activeScene.viewConstantBuffer->vEye = m_activeScene.scene->GetCameraPos();
    m_activeScene.viewConstantBuffer->mVP = m_activeScene.scene->GetMVP();
    m_activeScene.viewConstantBuffer->mInvViewProj = m_activeScene.scene->GetInvViewProjMatrix();
    m_activeScene.viewConstantBuffer-> vViewport = { 0, 0, (float)m_nScreenWidth, (float)m_nScreenHeight };
    m_activeScene.viewConstantBuffer.Update(GetDevice()->GetCurrentCommandContext());

    // Signal from graphics queue that compute can start.
    // Must call before Simulate() and before submitting
    // graphics commands to overlap with compute.
    if (m_AsyncCompute)
        GetDevice()->SignalComputeStart();

    if (!m_PauseSimulation) {
        Simulate(m_deltaTime, m_generateSDF, m_collisionResponse, m_AsyncCompute);
    }
    // Have compute work wait for signal from graphics queue that we can start 
    // issuing the sim commands.
    if (m_AsyncCompute)
    {
        GetDevice()->SubmitComputeCommandList();
        WaitSimulateDone(); // Waiting on this to be done really defies the point of doing this on another queue. 
                            // TODO: Double buffer needed resources to avoid synchronization issues in the middle of the frame
                            //       and don't wait on the queue.
    }

    // Update lighting constants for active scene
    m_activeScene.lightConstantBuffer->NumLights = m_activeScene.scene->GetSceneLightCount();
    for (int i = 0; i < m_activeScene.lightConstantBuffer->NumLights; ++i)
    {
        const Light& lightInfo = m_activeScene.scene->GetSceneLightInfo(i);
        m_activeScene.lightConstantBuffer->LightInfo[i].LightColor = { lightInfo.color[0], lightInfo.color[1], lightInfo.color[2] };
        m_activeScene.lightConstantBuffer->LightInfo[i].LightDirWS = { lightInfo.direction[0], lightInfo.direction[1], lightInfo.direction[2] };
        m_activeScene.lightConstantBuffer->LightInfo[i].LightInnerConeCos = lightInfo.innerConeCos;
        m_activeScene.lightConstantBuffer->LightInfo[i].LightIntensity = lightInfo.intensity;
        m_activeScene.lightConstantBuffer->LightInfo[i].LightOuterConeCos = lightInfo.outerConeCos;
        m_activeScene.lightConstantBuffer->LightInfo[i].LightPositionWS = { lightInfo.position[0], lightInfo.position[1], lightInfo.position[2] };
        m_activeScene.lightConstantBuffer->LightInfo[i].LightRange = lightInfo.range;
        m_activeScene.lightConstantBuffer->LightInfo[i].LightType = lightInfo.type;
        m_activeScene.lightConstantBuffer->LightInfo[i].ShadowMapIndex = lightInfo.shadowMapIndex;
        m_activeScene.lightConstantBuffer->LightInfo[i].ShadowProjection = *(AMD::float4x4*)&lightInfo.mLightViewProj; // ugh .. need a proper math library
        m_activeScene.lightConstantBuffer->LightInfo[i].ShadowParams = { lightInfo.depthBias, .1f, 100.0f, 0.f };	// Near and Far are currently hard-coded because we are hard-coding them elsewhere
        m_activeScene.lightConstantBuffer->LightInfo[i].ShadowMapSize = GetDevice()->GetShadowBufferResource()->GetWidth() / 2;
        m_activeScene.lightConstantBuffer->UseDepthApproximation = m_useDepthApproximation;
    }
    m_activeScene.lightConstantBuffer.Update(GetDevice()->GetCurrentCommandContext());

    // Render shadow passes to shadow buffer (and clear)
    uint32_t shadowMapIndex = 0;
    uint32_t sceneLightCount = m_activeScene.scene->GetSceneLightCount();
    for (uint32_t i = 0; i < sceneLightCount; i++)
    {
        const Light& LightInfo = m_activeScene.scene->GetSceneLightInfo(i);
        if (LightInfo.type != LightType_Spot && LightInfo.type != LightType_Directional &&(shadowMapIndex < 4))
            continue;

        uint32_t viewportOffsetsX[4] = { 0, 1, 0, 1 };
        uint32_t viewportOffsetsY[4] = { 0, 0, 1, 1 };
        uint32_t viewportWidth = GetDevice()->GetShadowBufferResource()->GetWidth() / 2;
        uint32_t viewportHeight = GetDevice()->GetShadowBufferResource()->GetHeight() / 2;

        // Setup shadow constants for this light
        m_activeScene.shadowViewConstantBuffer->vEye = { LightInfo.position[0], LightInfo.position[1], LightInfo.position[2], 0 };
        m_activeScene.shadowViewConstantBuffer->mVP = *(AMD::float4x4*) & LightInfo.mLightViewProj;
        m_activeScene.shadowViewConstantBuffer->vViewport = { 0, 0, (float)viewportWidth, (float)viewportHeight };
        m_activeScene.shadowViewConstantBuffer.Update(GetDevice()->GetCurrentCommandContext());

        // Update parameters (updates LOD shadow params)
        AMD::float4 ShadowCam = { LightInfo.position[0], LightInfo.position[1], LightInfo.position[2], 0 };
        UpdateRenderShadowParameters(ShadowCam);
        for (int i = 0; i < m_activeScene.objects.size(); ++i)
        {
            if (m_activeScene.objects[i].hairStrands->GetTressFXHandle())
                m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdatePerObjectRenderParams(GetDevice()->GetCurrentCommandContext());
        }
        GetDevice()->BeginRenderPass(GetDevice()->GetCurrentCommandContext(), m_pShadowRenderTargetSet.get(), L"ShadowPass", GetDevice()->GetShadowBufferResource()->GetWidth(), GetDevice()->GetShadowBufferResource()->GetHeight());

        // Set the RT's quadrant where to render the shadow map (these viewport offsets need to match the ones in shadowFiltering.h)
        GetDevice()->SetViewportAndScissor(GetDevice()->GetCurrentCommandContext(), viewportOffsetsX[shadowMapIndex] * viewportWidth, viewportOffsetsY[shadowMapIndex] * viewportHeight, viewportWidth, viewportHeight);

        // Render GLTF
        if (m_drawModel)
            m_activeScene.scene->OnRenderLight(i);

        GetDevice()->GetTimeStamp("Render GLTF Shadows");

        // Render Hair
        if (m_drawHair)
            DrawHairShadows();

        GetDevice()->GetTimeStamp("Render Hair Shadows");

        GetDevice()->EndRenderPass(GetDevice()->GetCurrentCommandContext());
        shadowMapIndex++;
    }

    // Transition shadow map to read
    EI_Barrier writeToRead = { GetDevice()->GetShadowBufferResource(), EI_STATE_DEPTH_STENCIL, EI_STATE_SRV };
    GetDevice()->GetCurrentCommandContext().SubmitBarrier(1, &writeToRead);

    // Render GLTF passes to main render target (and clear)
    GetDevice()->BeginRenderPass(GetDevice()->GetCurrentCommandContext(), m_pGLTFRenderTargetSet.get(), L"GLTFRender Pass");
    if (m_drawModel)
    {
        m_activeScene.scene->OnRender();
    }
    GetDevice()->EndRenderPass(GetDevice()->GetCurrentCommandContext());
    GetDevice()->GetTimeStamp("glTF Render");

    // Update rendering parameters (updates hair LOD params)
    UpdateRenderingParameters();
    for (int i = 0; i < m_activeScene.objects.size(); ++i)
    {
        if (m_activeScene.objects[i].hairStrands->GetTressFXHandle())
            m_activeScene.objects[i].hairStrands->GetTressFXHandle()->UpdatePerObjectRenderParams(GetDevice()->GetCurrentCommandContext());
    }

    // Do hair draw - will pick correct render approach
    if (m_drawHair)
    {
        DrawHair();
    }
    // Render debug collision mesh if needed
    EI_CommandContext& commandList = GetDevice()->GetCurrentCommandContext();

    if (m_drawCollisionMesh || m_drawMarchingCubes)
    {
        if (m_drawMarchingCubes)
        {
            GenerateMarchingCubes();
        }

        GetDevice()->BeginRenderPass(commandList, m_pDebugRenderTargetSet.get(), L"DrawCollisionMesh Pass");
        if (m_drawCollisionMesh)
        {
            DrawCollisionMesh();
        }
        if (m_drawMarchingCubes)
        {
            DrawSDF();
        }
        GetDevice()->EndRenderPass(GetDevice()->GetCurrentCommandContext());
    }

    // Transition shadow map to write
    EI_Barrier readToWrite = { GetDevice()->GetShadowBufferResource(), EI_STATE_SRV, EI_STATE_DEPTH_STENCIL };
    GetDevice()->GetCurrentCommandContext().SubmitBarrier(1, &readToWrite);
    GetDevice()->OnEndFrame();
}

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
#if defined(TRESSFX_DX12)
    LPCSTR Name = "TressFX v4.1 DX12";
#else
    LPCSTR Name = "TressFX v4.1 Vulkan";
#endif
    uint32_t Width = 1280;
    uint32_t Height = 800;

    // create new DX sample
    return RunFramework(hInstance, lpCmdLine, nCmdShow, Width, Height, new TressFXSample(Name));
}
