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

#include "SVOGI_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>


#include "SVOGISystemComponent.h"

//Needed for gEnv
#include <ISystem.h>
#include "SvoBrick.h"


namespace SVOGI
{

    //Free functions to be called by command callbacks.
    void ResetData([[maybe_unused]] IConsoleCmdArgs* args)
    {
        SVOGIRequestBus::Broadcast(&SVOGIRequests::ResetData);
    }

    void ResetGpuData([[maybe_unused]] IConsoleCmdArgs* args)
    {
        SVOGIRequestBus::Broadcast(&SVOGIRequests::ResetGpuData);
    }

    void ToggleShowVoxels([[maybe_unused]] IConsoleCmdArgs* args)
    {
        SVOGIRequestBus::Broadcast(&SVOGIRequests::ToggleShowVoxels);
    }

    //Free functions to be called by cvar callbacks.
    void ResetData([[maybe_unused]] ICVar* args)
    {
        SVOGIRequestBus::Broadcast(&SVOGIRequests::ResetData);
    }

    void ResetGpuData([[maybe_unused]] ICVar* args)
    {
        SVOGIRequestBus::Broadcast(&SVOGIRequests::ResetGpuData);
    }

    void SVOGISystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SVOGISystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SVOGISystemComponent>("LegacySVOGI", "Provides a Legacy interface to be used by CryEngine Code")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Triggers an update of voxel data.
    void SVOGISystemComponent::UpdateVoxelData()
    {
        static bool reload = true;
        if (!gEnv->pConsole->GetCVar("e_GI")->GetIVal())
        {
            reload = true;
            return;
        }
        AZ_TRACE_METHOD();

        if (reload && (!gEnv->p3DEngine->LevelLoadingInProgress() || gEnv->IsEditor()))
        {
            m_svoEnv->ReconstructTree();
            reload = false;
        }

        if (gEnv->pConsole->GetCVar("e_GI")->GetIVal())
        {
            if (m_svoEnv)
            {
                m_svoEnv->m_voxelLodCutoff = gEnv->pConsole->GetCVar("e_svoVoxelLodCutoff")->GetFVal();
                //Process nodes that were marked for update.
                m_svoEnv->ProcessVoxels();
                //Trigger clean up of out of date voxels if room is needed.
                m_svoEnv->EvictVoxels();
                //Refresh Update Queue
                m_svoEnv->UpdateVoxels();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Triggers an update of voxel data to GPU.
    void SVOGISystemComponent::UpdateRenderData()
    {
        if (gEnv->pConsole->GetCVar("e_GI")->GetIVal())
        {
            if (m_svoEnv)
            {
                m_svoEnv->CollectLights();
                m_svoEnv->EvictGpuData();
                m_svoEnv->UploadVoxels(m_svoShowVoxels);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Called at framestart
    void SVOGISystemComponent::OnFrameStart(const SRenderingPassInfo& passInfo)
    {
        if (m_svoEnv)
        {
            m_svoEnv->SetCamera(passInfo.GetCamera());
        }

        SvoEnvironment::m_currentPassFrameId = passInfo.GetMainFrameID();
    }

    //////////////////////////////////////////////////////////////////////////
    // Gets the textures bound for GI plus lighting data.
    void SVOGISystemComponent::GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D)
    {
        m_svoEnv->GetSvoStaticTextures(svoInfo, pLightsTI_S, pLightsTI_D);
    }

    //////////////////////////////////////////////////////////////////////////
    // Generates a list of bricks that need to be updated in compute shaders.
    void SVOGISystemComponent::GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, bool getDynamic)
    {
        if (m_svoEnv)
        {
            m_svoEnv->GetSvoBricksForUpdate(arrNodeInfo, getDynamic);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Causes the GI system to free all voxel data.
    void SVOGISystemComponent::ReleaseData()
    {
        //This is a product of how the legacy sytem worked.  It just bulk freed the entire system.
        //We bring up a new one right after since it was usually on level load.
        //Deactivate will nuke any remaning env.

        if (m_renderMutex)
        {
            m_renderMutex->lock();
        }

        //Free the svo renderer.
        if (gEnv->pRenderer->GetISvoRenderer())
        {
            gEnv->pRenderer->GetISvoRenderer()->Release();
        }
        delete m_svoEnv;

        //Recreate the svo renderer.
        gEnv->pRenderer->GetISvoRenderer();
        m_svoEnv = aznew SvoEnvironment();

        if (m_renderMutex)
        {
            m_renderMutex->unlock();
        }

    }

    //////////////////////////////////////////////////////////////////////////
    // Register and unregister a mutex to protect assets during rendering.
    void SVOGISystemComponent::RegisterMutex(AZStd::mutex* mutex)
    {
        if (m_renderMutex)
        {
            m_renderMutex->lock();
        }
        AZStd::mutex* temp = m_renderMutex;
        m_renderMutex = mutex;
        if (temp)
        {
            temp->unlock();
        }
    }

    void SVOGISystemComponent::UnregisterMutex()
    {
        if (m_renderMutex)
        {
            m_renderMutex->lock();
        }
        AZStd::mutex* temp = m_renderMutex;
        m_renderMutex = nullptr;
        if (temp)
        {
            temp->unlock();
        }
    }


    void SVOGISystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SVOGILegacyService"));
        provided.push_back(AZ_CRC("SVOGIService"));
    }

    void SVOGISystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SVOGILegacyService"));
        incompatible.push_back(AZ_CRC("SVOGIService"));
    }

    void SVOGISystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void SVOGISystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SVOGISystemComponent::Init()
    {
        m_svoEnv = nullptr;
        m_renderMutex = nullptr;
        m_svoShowVoxels = false;
    }

    void SVOGISystemComponent::Activate()
    {
        CrySystemEventBus::Handler::BusConnect();
        SVOGILegacyRequestBus::Handler::BusConnect();
        LmbrCentral::GiRegistrationBus::Handler::BusConnect();
        SVOGIRequestBus::Handler::BusConnect();
    }

    void SVOGISystemComponent::Deactivate()
    {
        SVOGIRequestBus::Handler::BusDisconnect();
        LmbrCentral::GiRegistrationBus::Handler::BusDisconnect();
        SVOGILegacyRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    void SVOGISystemComponent::UpsertToGi(AZ::EntityId entityId, AZ::Transform transform, AZ::Aabb worldAabb,
        AZ::Data::Asset <LmbrCentral::MeshAsset > meshAsset, _smart_ptr<IMaterial> material)
    {
        if (m_svoEnv)
        {
            m_svoEnv->UpsertMesh(entityId, transform, worldAabb, meshAsset, material);
        }
    }

    void SVOGISystemComponent::RemoveFromGi(AZ::EntityId entityId)
    {
        if (m_svoEnv)
        {
            m_svoEnv->RemoveMesh(entityId);
        }
    }

    void SVOGISystemComponent::OnCrySystemPreInitialize([[maybe_unused]] ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
    }

    void SVOGISystemComponent::OnCrySystemInitialized([[maybe_unused]] ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#endif
        RegisterCVars();
        gEnv->pRenderer->GetISvoRenderer();
        m_svoEnv = aznew SvoEnvironment();
    }

    void SVOGISystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        UnregisterCVars();
#if !defined(AZ_MONOLITHIC_BUILD)
        gEnv = nullptr;
#endif
        if (gEnv && gEnv->pRenderer && gEnv->pRenderer->GetISvoRenderer())
        {
            gEnv->pRenderer->GetISvoRenderer()->Release();
        }
        delete m_svoEnv;
        m_svoEnv = nullptr;
    }

    void SVOGISystemComponent::ResetData()
    {
        if (m_renderMutex)
        {
            m_renderMutex->lock();
        }

        if (m_svoEnv)
        {
            m_svoEnv->ReconstructTree();
        }

        if (m_renderMutex)
        {
            m_renderMutex->unlock();
        }
    }

    void SVOGISystemComponent::ResetGpuData()
    {
        if (m_renderMutex)
        {
            m_renderMutex->lock();
        }
        if (m_svoEnv && m_svoEnv->m_svoRoot)
        {
            m_svoEnv->m_brickUpdateQueue.clear();
            m_svoEnv->m_svoRoot->EvictGpuData(0, true);
            m_svoEnv->DeallocateTexturePools();
            m_svoEnv->AllocateTexturePools();
        }
        if (m_renderMutex)
        {
            m_renderMutex->unlock();
        }
    }

    void SVOGISystemComponent::ToggleShowVoxels()
    {
        m_svoShowVoxels = m_svoShowVoxels ? false : true;
    }

    void SVOGISystemComponent::RegisterCVars()
    {
        // UI parameters
        REGISTER_CVAR2_CB("e_svoTI_Active", &m_svoTI_Active, 0, VF_NULL,
            "Activates voxel GI for the level (experimental feature)", &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_IntegrationMode", &m_svoTI_IntegrationMode, 0, VF_EXPERIMENTAL,
            "GI computations may be used in several ways:\n"
            "0 = AO + Sun bounce\n"
            "      Large scale ambient occlusion (static) modulates (or replaces) default ambient lighting\n"
            "      Single light bounce (fully real-time) is supported for sun and (with limitations) for projectors \n"
            "      This mode takes less memory (only opacity is voxelized) and works acceptable on consoles\n"
            "1 = Diffuse GI mode (experimental)\n"
            "      GI completely replaces default diffuse ambient lighting\n"
            "      Two indirect light bounces are supported for sun and semi-static lights (use '_TI' in light name)\n"
            "      Single fully dynamic light bounce is supported for projectors (use '_TI_DYN' in light name)\n"
            "      Default ambient specular is modulated by intensity of diffuse GI\n"
            "2 = Full GI mode (very experimental)\n"
            "      Both ambient diffuse and ambient specular lighting is computed by voxel cone tracing\n"
            "      This mode works fine only on good modern PC",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_InjectionMultiplier", &m_svoTI_InjectionMultiplier, 0, VF_NULL,
            "Modulates light injection (controls the intensity of bounce light)",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_NumberOfBounces", &m_svoTI_NumberOfBounces, 0, VF_EXPERIMENTAL,
            "Maximum number of indirect bounces (from 0 to 2)\nFirst indirect bounce is completely dynamic\nThe rest of the bounces are cached in SVO and mostly static\nModifing this cvar will trigger cpu to gpu data uploads.",
            &SVOGI::ResetData);
        REGISTER_CVAR2("e_svoTI_Saturation", &m_svoTI_Saturation, 0, VF_NULL,
            "Controls color saturation of propagated light");
        REGISTER_CVAR2_CB("e_svoTI_DiffuseConeWidth", &m_svoTI_DiffuseConeWidth, 0, VF_EXPERIMENTAL,
            "Controls wideness of diffuse cones\nWider cones work faster but may cause over-occlusion and more light leaking",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_ConeMaxLength", &m_svoTI_ConeMaxLength, 0, VF_NULL,
            "Maximum length of the tracing rays (in meters)\nShorter rays work faster",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_AmbientOffsetRed", &m_svoTI_AmbientOffsetRed, 1, VF_NULL,
            "Ambient offset color for use with GI (Red Channel)",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_AmbientOffsetGreen", &m_svoTI_AmbientOffsetGreen, 1, VF_NULL,
            "Ambient offset color for use with GI (Green Channel)",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_AmbientOffsetBlue", &m_svoTI_AmbientOffsetBlue, 1, VF_NULL,
            "Ambient offset color for use with GI (Blue Channel)",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoTI_AmbientOffsetBias", &m_svoTI_AmbientOffsetBias, .1, VF_NULL,
            "Controls the amount of ambiant offset bias in the scene",
            &SVOGI::ResetData);
        REGISTER_CVAR2("e_svoTI_SSAOAmount", &m_svoTI_SSAOAmount, 0, VF_EXPERIMENTAL,
            "Allows to scale down SSAO (SSDO) amount and radius when GI is active");
        REGISTER_CVAR2("e_svoTI_UseLightProbes", &m_svoTI_UseLightProbes, 0, VF_NULL,
            "If enabled - environment probes lighting is multiplied with GI\nIf disabled - diffuse contribution of environment probes is ignored\nIn modes 1-2 it enables usage of global env probe for sky light instead of TOD fog color");
        REGISTER_CVAR2_CB("e_svoMinNodeSize", &m_svoMinNodeSize, 4, VF_EXPERIMENTAL,
            "Smallest SVO node allowed to create during level voxelization\nSmaller values helps getting more detailed lighting but may work slower and use more memory in pool\n",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoMaxNodeSize", &m_svoMaxNodeSize, 32, VF_NULL,
            "Maximum SVO node size for voxelization (bigger nodes stays empty)",
            &SVOGI::ResetData);
        REGISTER_CVAR2_CB("e_svoMaxBricksOnCPU", &m_svoMaxBricksOnCPU, 1024 * 8, VF_NULL,
            "Maximum number of voxel bricks allowed to cache on CPU side",
            &SVOGI::ResetData);
        REGISTER_CVAR2("e_svoMaxBrickUpdates", &m_svoMaxBrickUpdates, 48, VF_NULL,
            "Limits the number of bricks uploaded from CPU to GPU per frame");
        REGISTER_CVAR2("e_svoMaxVoxelUpdatesPerJob", &m_svoMaxVoxelUpdatesPerJob, 24, VF_NULL,
            "Controls amount of voxels allowed to refresh every frame");
        REGISTER_CVAR2("e_svoTemporalFilteringBase", &m_svoTemporalFilteringBase, .25f, VF_NULL,
            "Controls amount of temporal smoothing\n0 = less noise and aliasing, 1 = less ghosting");
        REGISTER_CVAR2("e_svoTemporalFilteringMinDistance", &m_svoTemporalFilteringMinDistance, .5f, VF_NULL,
            "Prevent previous frame re-projection at very near range, mostly for 1p weapon and hands");
        REGISTER_CVAR2("e_svoVoxelLodCutoff", &m_svoVoxelLodCutoff, 2.f, VF_NULL,
            "Cutoff for voxel lod ratio. Ratio is camera distance to voxel size");
        REGISTER_COMMAND("svoReset", &SVOGI::ResetData, 0, "This function forces a reset of the GI data. This is useful for dealing with legacy data.");
        REGISTER_COMMAND("svoToggleShowVoxels", &SVOGI::ToggleShowVoxels, 0, "This function toggles displaying voxels on and off.");
    }

    void SVOGISystemComponent::UnregisterCVars()
    {
        UNREGISTER_CVAR("e_svoMaxBricksOnCPU");
        UNREGISTER_CVAR("e_svoMaxBrickUpdates");
        UNREGISTER_CVAR("e_svoMaxStreamRequest");
        UNREGISTER_CVAR("e_svoMinNodeSize");
        UNREGISTER_CVAR("e_svoMaxNodeSize");
        UNREGISTER_CVAR("e_svoMaxAreaSize");
        UNREGISTER_CVAR("e_svoMaxVoxelUpdatesPerJob");
        UNREGISTER_CVAR("e_svoTemporalFilteringBase");
        UNREGISTER_CVAR("e_svoTemporalFilteringMinDistance");
        UNREGISTER_CVAR("e_svoVoxelLodCutoff");

        UNREGISTER_CVAR("e_svoTI_IntegrationMode");
        UNREGISTER_CVAR("e_svoTI_InjectionMultiplier");
        UNREGISTER_CVAR("e_svoTI_NumberOfBounces");
        UNREGISTER_CVAR("e_svoTI_Saturation");
        UNREGISTER_CVAR("e_svoTI_DiffuseConeWidth");
        UNREGISTER_CVAR("e_svoTI_ConeMaxLength");
        UNREGISTER_CVAR("e_svoTI_SpecularAmplifier");
        UNREGISTER_CVAR("e_svoTI_SSAOAmount");
        UNREGISTER_CVAR("e_svoTI_AmbientOffsetRed");
        UNREGISTER_CVAR("e_svoTI_AmbientOffsetGreen");
        UNREGISTER_CVAR("e_svoTI_AmbientOffsetBlue");
        UNREGISTER_CVAR("e_svoTI_AmbientOffsetBias");
    }
}

