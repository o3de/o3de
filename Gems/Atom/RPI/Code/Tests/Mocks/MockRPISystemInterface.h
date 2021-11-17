/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gmock/gmock.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

namespace AZ::RPI
{
    class MockRPISystemInterface
        : public RPISystemInterface
    {
    public:
        AZ_RTTI(MockRPISystemInterface, "{76FE11F7-6454-4964-90F2-B5D1579FC1E0}");
        
        MOCK_METHOD0(InitializeSystemAssets, void());
        MOCK_CONST_METHOD0(IsInitialized, bool());
        MOCK_METHOD1(RegisterScene, void(ScenePtr scene));
        MOCK_METHOD1(UnregisterScene, void(ScenePtr scene));
        MOCK_CONST_METHOD0(GetDefaultScene, ScenePtr());
        MOCK_CONST_METHOD1(GetScene, Scene*(const SceneId& sceneId));
        MOCK_CONST_METHOD1(GetSceneByName, Scene*(const AZ::Name& name));
        MOCK_METHOD1(GetRenderPipelineForWindow, RenderPipelinePtr(AzFramework::NativeWindowHandle windowHandle));
        MOCK_CONST_METHOD0(GetCommonShaderAssetForSrgs, Data::Asset<ShaderAsset>());
        MOCK_CONST_METHOD0(GetSceneSrgLayout, RHI::Ptr<RHI::ShaderResourceGroupLayout>());
        MOCK_CONST_METHOD0(GetViewSrgLayout, RHI::Ptr<RHI::ShaderResourceGroupLayout>());
        MOCK_METHOD0(SimulationTick, void());
        MOCK_METHOD0(RenderTick, void());
        MOCK_METHOD1(SetSimulationJobPolicy, void(RHI::JobPolicy jobPolicy));
        MOCK_CONST_METHOD0(GetSimulationJobPolicy, RHI::JobPolicy());
        MOCK_METHOD1(SetRenderPrepareJobPolicy, void(RHI::JobPolicy jobPolicy));
        MOCK_CONST_METHOD0(GetRenderPrepareJobPolicy, RHI::JobPolicy());
        MOCK_CONST_METHOD0(GetDescriptor, const RPISystemDescriptor&());
        MOCK_CONST_METHOD0(GetRenderApiName, Name());
        MOCK_CONST_METHOD0(GetCurrentTick, uint64_t());
    };
} // namespace AZ
