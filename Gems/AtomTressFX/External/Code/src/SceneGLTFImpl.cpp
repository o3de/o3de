// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "EngineInterface.h"
#include "GLTF/GltfCommon.h"
#include "GLTF/GltfHelpers.h"
#include <cmath>

void EI_Scene::OnCreate(EI_Device* pDevice, EI_RenderTargetSet * renderTargetSet, EI_RenderTargetSet* shadowRenderTargetSet, const std::string& path, const std::string& fileName, const std::string& bonePrefix, float startOffset)
{
    if (m_pGLTFCommon == nullptr)
    {
        m_pGLTFCommon = new GLTFCommon();
        m_pGLTFCommon->Load(path, fileName);
    }

    m_pDevice = pDevice;

    m_startOffset = startOffset;
    
    m_pGLTFTexturesAndBuffers = m_pDevice->CreateGLTFTexturesAndBuffers(m_pGLTFCommon);
    // here we are loading onto the GPU all the textures and the inverse matrices
    // this data will be used to create the PBR and Depth passes       
    m_pGLTFTexturesAndBuffers->LoadTextures();

    // same thing as above but for the PBR pass
    m_gltfPBR = m_pDevice->CreateGLTFPbrPass(m_pGLTFTexturesAndBuffers.get(), renderTargetSet);
    
    m_pDevice->GetVidMemBufferPool()->UploadData(m_pDevice->GetUploadHeap()->GetCommandList());
    m_pDevice->GetUploadHeap()->FlushAndFinish();

    // Create a depth GLTF pass to render shadows
    m_gltfDepth = m_pDevice->CreateGLTFDepthPass(m_pGLTFTexturesAndBuffers.get(), shadowRenderTargetSet);
    
    m_pDevice->GetVidMemBufferPool()->UploadData(m_pDevice->GetUploadHeap()->GetCommandList());
    m_pDevice->GetUploadHeap()->FlushAndFinish();

    // Init Camera, looking at the origin
    m_roll = 0.0f;
    m_pitch = 0.0f;
    m_distance = 2.0f;
    m_animationTime = 0.0f;

    // init GUI state   
    m_state.iblFactor = 2.0f;
    m_state.emmisiveFactor = 1.0f;

    m_bonePrefix = bonePrefix;
    m_state.camera.SetSpeed(0.5f);
    ComputeGlobalIdxToSkinIdx();
}

void EI_Scene::OnDestroy()
{
    if (m_pGLTFTexturesAndBuffers)
        m_pGLTFTexturesAndBuffers->OnDestroy();

    if (m_pGLTFCommon)
    {
        delete m_pGLTFCommon;
        m_pGLTFCommon = nullptr;
    }

    if (m_gltfDepth)
        m_gltfDepth->OnDestroy();

    if (m_gltfPBR)
        m_gltfPBR->OnDestroy();
}

void EI_Scene::OnBeginFrame(float deltaTime, float aspect)
{
    // update gltf data
    m_animationTime += deltaTime;
    m_animationTime = fmod(m_animationTime, m_pGLTFCommon->m_animations[0].m_duration - m_startOffset);
    m_pGLTFCommon->SetAnimationTime(0, m_startOffset + m_animationTime);
    m_pGLTFCommon->TransformScene(0, XMMatrixIdentity());

    //m_ConstantBufferRing.OnBeginFrame();

    const char * cameraControl[] = { "Animated", "WASD", "Orbit" };
    static int cameraControlSelected = 0;
    ImGui::Combo("Camera", &cameraControlSelected, cameraControl, _countof(cameraControl));

    ImGuiIO& io = ImGui::GetIO();
    // If the mouse was not used by the GUI then it's for the camera
    //
    if (io.WantCaptureMouse == false)
    {
        if ((io.KeyCtrl == false) && (io.MouseDown[0] == true))
        {
            m_roll -= io.MouseDelta.x / 100.f;
            m_pitch += io.MouseDelta.y / 100.f;
        }

        // Choose camera movement depending on setting
        //
        if (cameraControlSelected == 0)
        {
            if (m_pGLTFCommon->m_cameras.size())
            {
                XMMATRIX* pMats = m_pGLTFCommon->m_pCurrentFrameTransformedData->m_worldSpaceMats.data();
                XMMATRIX cameraMat = pMats[m_pGLTFCommon->m_cameras[0].m_nodeIndex];
                m_state.camera.SetMatrix(cameraMat);
                m_state.camera.LookAt(m_state.camera.GetPosition(), m_state.camera.GetPosition() - DirectX::XMVector3Normalize(m_state.camera.GetDirection()) * m_distance);
                m_pitch = m_state.camera.GetPitch();
                m_roll = m_state.camera.GetYaw();
                m_distance = m_state.camera.GetDistance();
            }
        }
        if (cameraControlSelected == 1)
        {
            //  WASD
            m_state.camera.UpdateCameraWASD(m_roll, m_pitch, io.KeysDown, io.DeltaTime);
        }
        else if (cameraControlSelected == 2)
        {
            //  Orbiting 
            m_distance -= (float)io.MouseWheel * 0.5f;
            m_distance = std::max<float>(m_distance, 0.1f);

            bool panning = (io.KeyCtrl == true) && (io.MouseDown[0] == true);

            m_state.camera.LookAt(m_state.camera.GetPosition(), DirectX::XMVectorSet(-0.1f, 0.5f, 0.4f, 0.0f));
            m_state.camera.UpdateCameraPolar(m_roll, m_pitch, panning ? -io.MouseDelta.x / 100.0f : 0.0f, panning ? io.MouseDelta.y / 100.0f : 0.0f, m_distance);
        }
    }

    // transform geometry and skinning matrices + upload them as buffers --------------
    per_frame *pPerFrame = NULL;
    if (m_pGLTFTexturesAndBuffers)
    {
        pPerFrame = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->SetPerFrameData(0, 1280.0f / 720.0f);
        pPerFrame->mCameraViewProj = m_state.camera.GetView() * m_state.camera.GetProjection();
        pPerFrame->cameraPos = m_state.camera.GetPosition();
        pPerFrame->iblFactor = m_state.iblFactor;
        pPerFrame->emmisiveFactor = m_state.emmisiveFactor;

        // Store the light information for the frame (as it will be needed later)
        if (pPerFrame->lightCount != m_SceneLightInfo.size())
            m_SceneLightInfo.resize(pPerFrame->lightCount);

        // For now, divide up the shadow map into 4 2k x 2k quadrants. This will change once we decide what shadow scheme we want to support
        // Also, until we can get light exports working well, just support spots as shadow casters for now
        uint32_t shadowMapIndex = 0;
        for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
        {
            assert(shadowMapIndex < 4 && "Too many shadows are enabled, ignoring all shadows after 4th shadow casting light.");
            if ((shadowMapIndex < 4) && (pPerFrame->lights[i].type == LightType_Spot || pPerFrame->lights[i].type == LightType_Directional))
                pPerFrame->lights[i].shadowMapIndex = shadowMapIndex++; //set the shadow map index so the color pass knows which shadow map to use
            else
                pPerFrame->lights[i].shadowMapIndex = -1;

            // Store this information
            m_SceneLightInfo[i] = pPerFrame->lights[i];
        }

        m_pGLTFTexturesAndBuffers->SetPerFrameConstants();
        m_pGLTFTexturesAndBuffers->SetSkinningMatricesForSkeletons();
    }
}

void EI_Scene::OnRender()
{
#if TRESSFX_VK
    m_gltfPBR->Draw(GetDevice()->GetCurrentCommandContext().commandBuffer);
#else
    m_gltfPBR->Draw(GetDevice()->GetCurrentCommandContext().commandBuffer.Get(), GetDevice()->GetShadowBufferResource()->SRView);
#endif // TRESSFX_VK
}

void EI_Scene::OnRenderLight(uint32_t LightIndex)
{
    // Set per frame constant buffer values
#if TRESSFX_VK
    CAULDRON_VK::GltfDepthPass::per_frame* cbPerFrame = m_gltfDepth->SetPerFrameConstants();
    cbPerFrame->mViewProj = m_SceneLightInfo[LightIndex].mLightViewProj;
    m_gltfDepth->Draw(GetDevice()->GetCurrentCommandContext().commandBuffer);
#else 
    CAULDRON_DX12::GltfDepthPass::per_frame* cbPerFrame = m_gltfDepth->SetPerFrameConstants();
    cbPerFrame->mViewProj = m_SceneLightInfo[LightIndex].mLightViewProj;
    m_gltfDepth->Draw(GetDevice()->GetCurrentCommandContext().commandBuffer.Get());
#endif // TRESSFX_VK
}

void EI_Scene::OnResize(uint32_t width, uint32_t height)
{
    m_state.camera.SetFov(XM_PI / 4, width, height, 0.1f, 1000.0f);
}

void EI_Scene::ComputeGlobalIdxToSkinIdx()
{
    m_globalIdxToSkinIdx.resize(m_pGLTFCommon->m_skins.size());
    for (int skin = 0; skin < m_pGLTFCommon->m_skins.size(); ++skin)
    {
        m_globalIdxToSkinIdx[skin].resize(m_pGLTFCommon->m_nodes.size());
        for (int i = 0; i < m_pGLTFCommon->m_skins[skin].m_jointsNodeIdx.size(); ++i)
        {
            m_globalIdxToSkinIdx[skin][m_pGLTFCommon->m_skins[skin].m_jointsNodeIdx[i]] = i;
        }
    }
}

int EI_Scene::GetBoneIdByName(int skinNumber, const char * name)
{
    std::string completeString = m_bonePrefix + name;
    for (int i = 0; i < m_pGLTFCommon->m_nodes.size(); ++i)
    {
        if (completeString == m_pGLTFCommon->m_nodes[i].m_name) {
            return m_globalIdxToSkinIdx[skinNumber][i];
        }
    }
    return -1;
}
