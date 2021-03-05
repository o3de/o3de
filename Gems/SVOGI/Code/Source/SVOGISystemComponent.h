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

#include <AzCore/Component/Component.h>

#include <LmbrCentral/Rendering/GiRegistrationBus.h>
#include <SVOGI/SVOGIBus.h>
#include <I3DEngine.h>

#include <SvoTree.h>

#include <CrySystemBus.h>

//This interface exists entirely for linking with the current CrySystems.  
//It is mostly intended to be a thin wrapper the newer system that we can easily remove once it
//is no longer needed. 
namespace SVOGI
{
    class SVOGISystemComponent
        : public AZ::Component
        , private CrySystemEventBus::Handler
        , protected SVOGIRequestBus::Handler
        , protected SVOGILegacyRequestBus::Handler
        , protected LmbrCentral::GiRegistrationBus::Handler
    {
    public:
        AZ_COMPONENT(SVOGISystemComponent, "{800B70D1-04D0-4E77-A603-D527CE1D3E03}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // SVOGILegacyRequestBus interface implementation

        //////////////////////////////////////////////////////////////////////////
        // Triggers an update of voxel data. 
        void UpdateVoxelData() override;

        //////////////////////////////////////////////////////////////////////////
        // Triggers an update of voxel data to GPU.
        void UpdateRenderData() override;

        //////////////////////////////////////////////////////////////////////////
        // Called at framestart
        void OnFrameStart(const SRenderingPassInfo& passInfo) override;

        //////////////////////////////////////////////////////////////////////////
        // Gets the textures bound for GI plus lighting data.
        void GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D) override;

        //////////////////////////////////////////////////////////////////////////
        // Generates a list of bricks that need to be updated in compute shaders.
        void GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, bool getDynamic) override;

        //////////////////////////////////////////////////////////////////////////
        // Causes the GI system to free all voxel data. 
        void ReleaseData() override;

        //////////////////////////////////////////////////////////////////////////
        // Register and unregister a mutex to protect assets during rendering.
        void RegisterMutex(AZStd::mutex* mutex) override;
        void UnregisterMutex() override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LmbrCentral::SVOGIRegistration interface implementation
        void UpsertToGi(AZ::EntityId entityId, AZ::Transform transform, AZ::Aabb worldAabb,
            AZ::Data::Asset <LmbrCentral::MeshAsset > meshAsset, _smart_ptr<IMaterial> material) override;
        void RemoveFromGi(AZ::EntityId entityId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SVOGIRequests interface implementation
        void ResetData() override;
        void ResetGpuData() override;
        void ToggleShowVoxels() override;
        ////////////////////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemPreInitialize(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;
        ////////////////////////////////////////////////////////////////////////////

    private:
        //Root entry point for SVOGI work.
        SvoEnvironment* m_svoEnv;

        //SVOGI CVARS
        void    RegisterCVars();
        void    UnregisterCVars();
        int     m_svoMaxBricksOnCPU;
        int     m_svoMaxBrickUpdates;
        int     m_svoMaxVoxelUpdatesPerJob;
        float   m_svoMinNodeSize;
        float   m_svoMaxNodeSize;
        float   m_svoTemporalFilteringBase;
        float   m_svoTemporalFilteringMinDistance;
        float   m_svoVoxelLodCutoff;

        int     m_svoTI_Active;
        int     m_svoTI_IntegrationMode;
        int     m_svoTI_NumberOfBounces;
        int     m_svoTI_UseLightProbes;
        float   m_svoTI_SSAOAmount;
        float   m_svoTI_Saturation;
        float   m_svoTI_DiffuseConeWidth;
        float   m_svoTI_ConeMaxLength;
        float   m_svoTI_InjectionMultiplier;
        float   m_svoTI_AmbientOffsetRed;
        float   m_svoTI_AmbientOffsetGreen;
        float   m_svoTI_AmbientOffsetBlue;
        float   m_svoTI_AmbientOffsetBias;

        bool    m_svoShowVoxels;

        AZStd::mutex* m_renderMutex;
    };
}
