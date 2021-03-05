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

#ifndef STEREORENDERER_H
#define STEREORENDERER_H

#pragma once

#include "IStereoRenderer.h"

class CD3D9Renderer;
class D3DHMDRenderer;

class CD3DStereoRenderer
    : public IStereoRenderer
{
public:
    CD3DStereoRenderer(CD3D9Renderer& renderer, EStereoDevice device);
    virtual ~CD3DStereoRenderer();

    void InitDeviceBeforeD3D();
    void InitDeviceAfterD3D();
    void Shutdown();
    float GetScreenDiagonalInInches() const { return m_screenSize; }
    void SetScreenDiagonalInInches(float size) { m_screenSize = size; }

    void CreateResources();
    void ReleaseResources();

    bool IsStereoEnabled() const { return m_device != STEREO_DEVICE_NONE && m_mode != STEREO_MODE_NO_STEREO; }

    void PrepareStereo(EStereoMode mode, EStereoOutput output);

    EStereoDevice GetStereoDevice() const { return m_device; }
    EStereoMode GetStereoMode() const { return m_mode; }
    EStereoOutput GetStereoOutput() const { return m_output; }

    CTexture* GetLeftEye() { return m_pLeftTex == NULL ? CTexture::s_ptexStereoL : m_pLeftTex; }
    CTexture* GetRightEye() { return m_pRightTex == NULL ? CTexture::s_ptexStereoR : m_pRightTex; }

    void SetEyeTextures(CTexture* leftTex, CTexture* rightTex);

    void Update();
    void ProcessScene(int sceneFlags, const SRenderingPassInfo& passInfo);
    void ReleaseBuffers();
    void OnResolutionChanged() override;
    void CalculateBackbufferResolution(int eyeWidth, int eyeHeight, int& backbufferWidth, int& backbufferHeight);

    void CopyToStereo(int channel);
    void DisplayStereo();

    void BeginRenderingMRT(bool disableClear);
    void EndRenderingMRT(bool bResolve = true);

    void ResolveStereoBuffers();

    void BeginRenderingTo(EStereoEye eye);
    void EndRenderingTo(EStereoEye eye);

    void TakeScreenshot(const char path[]);

    float GetNearGeoShift() { return m_zeroParallaxPlaneDist; }
    float GetNearGeoScale() { return m_nearGeoScale; }
    float GetGammaAdjustment() { return m_gammaAdjustment; }

public:
    // IStereoRenderer Interface
    virtual EStereoDevice GetDevice();
    virtual EStereoDeviceState GetDeviceState();
    virtual void GetInfo(EStereoDevice* device, EStereoMode* mode, EStereoOutput* output, EStereoDeviceState* state) const;

    virtual bool GetStereoEnabled();
    virtual float GetStereoStrength();
    virtual float GetMaxSeparationScene(bool half = true);
    virtual float GetZeroParallaxPlaneDist();
    virtual void GetNVControlValues(bool& stereoEnabled, float& stereoStrength);

    virtual void OnHmdDeviceChanged();
    Status GetStatus() const override { return m_renderStatus; }
    virtual bool IsRenderingToHMD() override;

private:
    enum DriverType
    {
        DRIVER_UNKNOWN = 0,
        DRIVER_NV = 1,
        DRIVER_AMD = 2
    };

    CD3D9Renderer& m_renderer;

    EStereoDevice m_device;
    EStereoDeviceState m_deviceState;
    EStereoMode m_mode;
    EStereoOutput m_output;

    DriverType m_driver;

    CTexture* m_pLeftTex;
    CTexture* m_pRightTex;

    void* m_nvStereoHandle;
    float m_nvStereoStrength;
    uint8 m_nvStereoActivated;

    EStereoEye m_curEye;
    IStereoRenderer::Status m_renderStatus;
    CCryNameR m_SourceSizeParamName;

    uint32 m_frontBufWidth, m_frontBufHeight;

    float m_stereoStrength;
    float m_zeroParallaxPlaneDist;
    float m_maxSeparationScene;
    float m_nearGeoScale;
    float m_gammaAdjustment;
    float m_screenSize;

    bool m_resourcesPatched;
    bool m_needClearLeft;
    bool m_needClearRight;

    D3DHMDRenderer* m_hmdRenderer;

private:
    void SelectDefaultDevice();

    void CreateIntermediateBuffers();

    bool EnableStereo();
    void DisableStereo();
    void ChangeOutputFormat();
    void HandleNVControl();
    bool InitializeHmdRenderer();
    void ShutdownHmdRenderer();

    void RenderScene(int sceneFlags, const SRenderingPassInfo& passInfo);

    void SelectShaderTechnique();

    bool IsRenderThread() const;
    void CopyToStereoFromMainThread(int channel);

    void PushRenderTargets();
    void PopRenderTargets(bool bResolve);

    CCamera PrepareCamera(EStereoEye nEye, const CCamera& currentCamera, const SRenderingPassInfo& passInfo);

    bool IsDriver(DriverType driver) { return m_device == STEREO_DEVICE_DRIVER && m_driver == driver; }
};

#endif
