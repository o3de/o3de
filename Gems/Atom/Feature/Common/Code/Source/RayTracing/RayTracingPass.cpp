/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/Feature/RayTracing/RayTracingPassData.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

using uint = uint32_t;
using uint4 = uint[4];
#include "../../../Feature/Common/Assets/ShaderLib/Atom/Features/IndirectRendering.azsli"

namespace AZ
{
    namespace Render
    {

        void RayTracingShaderLibs::AddShaderFunction(
            const ShaderFunctionType type, const AZStd::string& entryFunction, ShaderLib* shaderLib)
        {
            switch (type)
            {
            case ShaderFunctionType::RayGen:
                {
                    AZ_Assert(shaderLib->m_rayGen.IsEmpty(), "RayGeneration function is already specified");
                    shaderLib->m_rayGen = AZ::Name{ entryFunction };
                    break;
                }
            case ShaderFunctionType::ProceduralClosestHit:
                {
                    AZ_Assert(shaderLib->m_proceduralClosestHit.IsEmpty(), "ClosestHit function is already specified");
                    shaderLib->m_proceduralClosestHit = AZ::Name{ entryFunction };
                    break;
                }
            case ShaderFunctionType::ClosestHit:
                {
                    AZ_Assert(shaderLib->m_closestHit.IsEmpty(), "ClosestHit function is already specified");
                    shaderLib->m_closestHit = AZ::Name{ entryFunction };
                    break;
                }
            case ShaderFunctionType::AnyHit:
                {
                    AZ_Assert(shaderLib->m_anyHit.IsEmpty(), "AnyHit function is already specified");
                    shaderLib->m_anyHit = AZ::Name{ entryFunction };
                    break;
                }
            case ShaderFunctionType::Intersection:
                {
                    AZ_Assert(shaderLib->m_intersection.IsEmpty(), "Intersection function is already specified");
                    shaderLib->m_intersection = AZ::Name{ entryFunction };
                    break;
                }
            case ShaderFunctionType::Miss:
                {
                    AZ_Assert(shaderLib->m_miss.IsEmpty(), "Miss function is already specified");
                    shaderLib->m_miss = AZ::Name{ entryFunction };
                    break;
                }
            default:
                AZ_Assert(false, "Unknown Raytracing Shader type");
                break;
            }
            m_assignedShaderLibs[static_cast<size_t>(type)].push_back(shaderLib);
        } // namespace Render

        RayTracingShaderLibs::ShaderLib* RayTracingShaderLibs::GetOrCreateShaderLib(
            const AZ::Data::Instance<AZ::RPI::Shader>& shader, [[maybe_unused]] const AZ::Name& supervariantName)
        {
            auto it = m_shaderLibs.find(shader->GetAssetId());
            if (it == m_shaderLibs.end())
            {
                auto shaderLib = AZStd::make_unique<ShaderLib>(ShaderLib{ shader });
                const auto& [new_it, inserted] = m_shaderLibs.emplace(AZStd::make_pair(shader->GetAssetId(), AZStd::move(shaderLib)));
                return new_it->second.get();
            }
            return it->second.get();
        }

        void RayTracingShaderLibs::AddShaderFunction(
            const RayTracingShaderLibs::ShaderFunctionType type,
            const AZStd::string& entryFunction,
            const AZ::Data::Instance<AZ::RPI::Shader>& shader,
            const AZ::Name& supervariantName)
        {
            auto* shaderLib = GetOrCreateShaderLib(shader, supervariantName);
            AddShaderFunction(type, entryFunction, shaderLib);
        }

        RayTracingShaderLibs::ShaderLib* RayTracingShaderLibs::GetOrCreateShaderLib(
            const RPI::AssetReference& assetReference, const AZ::Name& supervariantName)
        {
            auto it = m_shaderLibs.find(assetReference.m_assetId);
            if (it == m_shaderLibs.end())
            {
                auto shaderAsset{ AZ::RPI::FindShaderAsset(assetReference.m_assetId, assetReference.m_filePath) };
                AZ_Assert(shaderAsset.IsReady(), "Failed to load shader %s", assetReference.m_filePath.c_str());
                auto shader{ AZ::RPI::Shader::FindOrCreate(shaderAsset, supervariantName) };
                AZ_Assert(shader, "Failed to load shader %s", assetReference.m_filePath.c_str());
                auto shaderLib = AZStd::make_unique<ShaderLib>(ShaderLib{ shader });
                const auto& [new_it, inserted] = m_shaderLibs.emplace(AZStd::make_pair(shader->GetAssetId(), AZStd::move(shaderLib)));
                return new_it->second.get();
            }
            return it->second.get();
        }

        void RayTracingShaderLibs::AddShaderFunction(
            const RayTracingShaderLibs::ShaderFunctionType type,
            const AZStd::string& entryFunction,
            const RPI::AssetReference& assetReference,
            const AZ::Name& supervariantName)
        {
            auto* shaderLib = GetOrCreateShaderLib(assetReference, supervariantName);
            AddShaderFunction(type, entryFunction, shaderLib);
        }

        bool RayTracingPass::ValidateShaderLibs(const AZStd::span<const RayTracingShaderLibs* const> shaderLibsCollection) const
        {
            auto GetLayoutId = [&](const RayTracingShaderLibs::ShaderLib* shaderLib, const uint32_t slot)
            {
                auto layout = shaderLib->m_shader->FindShaderResourceGroupLayout(slot);
                if (layout)
                {
                    return layout->GetUniqueId();
                }
                // shader doesn't use this slot
                return AZStd::string{ "" };
            };

            bool valid = true;
            const RayTracingShaderLibs::ShaderLib* rayGenShaderLib{ nullptr };
            for (auto* shaderLibs : shaderLibsCollection)
            {
                rayGenShaderLib = shaderLibs->GetRayGenShaderLib();
                if (rayGenShaderLib)
                {
                    break;
                }
            }

            if (!rayGenShaderLib)
            {
                AZ_Error("RayTracingPass", false, "None of the loaded shaderLib contains a RayGeneration shader");
                return false;
            }

            // note: this assumes the Bindless SRG is the last srg
            AZStd::array<AZStd::string, RPI::SrgBindingSlot::Bindless + 1> layoutIds;
            for (uint32_t slot = 0; slot <= RPI::SrgBindingSlot::Bindless; ++slot)
            {
                // collect the SRG layout from the first shader library
                layoutIds[slot] = GetLayoutId(rayGenShaderLib, slot);
            }

            AZStd::unordered_set<AZ::Name> shaderNames;

            for (auto shaderLibs : shaderLibsCollection)
            {
                // make sure the names of all shaders are unique, since the HitGroups reference the shaders only with the name
                for (auto& [_, shaderLib] : shaderLibs->GetShaderLibraries())
                {
                    if (!shaderLib->m_anyHit.IsEmpty())
                    {
                        const auto& [__, inserted] = shaderNames.insert(shaderLib->m_anyHit);
                        if (inserted == false)
                        {
                            valid = false;
                            AZ_Error(
                                "RayTracingPass",
                                false,
                                "AnyHit shader name %s is not unique across all hit groups",
                                shaderLib->m_anyHit.GetCStr());
                        }
                    }
                    if (!shaderLib->m_closestHit.IsEmpty())
                    {
                        const auto& [__, inserted] = shaderNames.insert(shaderLib->m_closestHit);
                        if (inserted == false)
                        {
                            valid = false;
                            AZ_Error(
                                "RayTracingPass",
                                false,
                                "ClosestHit shader name %s is not unique across all hit groups",
                                shaderLib->m_closestHit.GetCStr());
                        }
                    }
                    if (!shaderLib->m_intersection.IsEmpty())
                    {
                        const auto& [__, inserted] = shaderNames.insert(shaderLib->m_intersection);
                        if (inserted == false)
                        {
                            valid = false;
                            AZ_Error(
                                "RayTracingPass",
                                false,
                                "Intersection shader name %s is not unique across all hit groups",
                                shaderLib->m_intersection.GetCStr());
                        }
                    }
                    if (!shaderLib->m_proceduralClosestHit.IsEmpty())
                    {
                        const auto& [__, inserted] = shaderNames.insert(shaderLib->m_proceduralClosestHit);
                        if (inserted == false)
                        {
                            valid = false;
                            AZ_Error(
                                "RayTracingPass",
                                false,
                                "ProceduralClosestHit shader name %s is not unique across all hit groups",
                                shaderLib->m_proceduralClosestHit.GetCStr());
                        }
                    }

                    for (uint32_t slot = 0; slot <= RPI::SrgBindingSlot::Bindless; ++slot)
                    {
                        // - The shader we're testing has an empty layout but the RayGenerationShader doesnt: that is okay, since the shader
                        // won't access whatever is actually bound in that slot anyway.
                        // - The slot is empty in the RayGeneration-shader but the current shader expects an SRG: That is not okay, since
                        // it could clash with other shaders binding something different in that slot, and we also won't bind anything
                        // there.
                        auto layoutId = GetLayoutId(shaderLib.get(), slot);
                        if (!layoutId.empty() && layoutId != layoutIds[slot])
                        {
                            valid = false;
                            AZ_Error(
                                "RayTracingPass",
                                false,
                                "Srg Layouts in binding slot %d of Raytracing shaders %s [%s] and %s [%s] don't match",
                                slot,
                                rayGenShaderLib->m_shader->GetAsset().GetHint().c_str(),
                                layoutIds[slot].c_str(),
                                shaderLib->m_shader->GetAsset().GetHint().c_str(),
                                layoutId.c_str());
                        }
                    }
                }
            }
            return valid;
        }

        const RayTracingShaderLibs::AssignedShaderLibraries& RayTracingShaderLibs::GetAssignedShaderLibraries() const
        {
            return m_assignedShaderLibs;
        }
        const RayTracingShaderLibs::UniqueShaderLibraries& RayTracingShaderLibs::GetShaderLibraries() const
        {
            return m_shaderLibs;
        }

        const RayTracingShaderLibs::ShaderLib* RayTracingShaderLibs::GetRayGenShaderLib() const
        {
            auto& rayGenShaderLibs{ m_assignedShaderLibs[static_cast<size_t>(ShaderFunctionType::RayGen)] };
            if (rayGenShaderLibs.empty())
            {
                return nullptr;
            }
            return rayGenShaderLibs.span().front();
        }

        const RPI::Shader* RayTracingShaderLibs::GetShaderForSrgs() const
        {
            // we assume we always have a RayGeneration Shader
            return GetRayGenShaderLib()->m_shader.get();
        }

        void RayTracingShaderLibs::RegisterShaderLibraries(RHI::RayTracingPipelineStateDescriptor& descriptor)
        {
            for (auto& [_, shaderLib] : m_shaderLibs)
            {
                descriptor.ShaderLibrary(shaderLib->m_pipelineStateDescriptor);
                if (!shaderLib->m_rayGen.IsEmpty())
                {
                    descriptor.RayGenerationShaderName(shaderLib->m_rayGen);
                }
                if (!shaderLib->m_closestHit.IsEmpty())
                {
                    descriptor.ClosestHitShaderName(shaderLib->m_closestHit);
                }
                if (!shaderLib->m_proceduralClosestHit.IsEmpty())
                {
                    // the procedural closest-hit - shader is a normal ClosestHit - shader for the GPU
                    descriptor.ClosestHitShaderName(shaderLib->m_proceduralClosestHit);
                }
                if (!shaderLib->m_anyHit.IsEmpty())
                {
                    descriptor.AnyHitShaderName(shaderLib->m_anyHit);
                }
                if (!shaderLib->m_intersection.IsEmpty())
                {
                    descriptor.IntersectionShaderName(shaderLib->m_intersection);
                }
                if (!shaderLib->m_miss.IsEmpty())
                {
                    descriptor.MissShaderName(shaderLib->m_miss);
                }
            }
        }

        void RayTracingShaderLibs::Reset()
        {
            for (auto& libs : m_assignedShaderLibs)
            {
                libs.clear();
            };
            m_shaderLibs.clear();
        }

        void RayTracingHitGroups::SetRayGenerationShader(RayTracingShaderLibs::ShaderLib* shaderLib)
        {
            AZ_Assert(!shaderLib->m_rayGen.IsEmpty(), "ShaderLib has no RayGen Shader");
            m_rayGenShader = shaderLib->m_rayGen;
        }

        void RayTracingHitGroups::SetMissShader(RayTracingShaderLibs::ShaderLib* shaderLib)
        {
            AZ_Assert(!shaderLib->m_miss.IsEmpty(), "ShaderLib has no Miss Shader");
            m_missShader = shaderLib->m_miss;
        }

        void RayTracingHitGroups::AddHitGroup(const AZ::Name& name, const HitGroupShaderLibs& shaderLibs, bool IsProceduralHitGroup)
        {
            HitGroup hitGroup{};

            hitGroup.m_name = name;

            int hitShaderCount = 0;
            if (shaderLibs.m_closestHit)
            {
                if (IsProceduralHitGroup)
                {
                    AZ_Assert(!shaderLibs.m_closestHit->m_proceduralClosestHit.IsEmpty(), "ShaderLib has no ProceduralClosestHit Shader");
                    hitShaderCount++;
                    hitGroup.m_closestHit = shaderLibs.m_closestHit->m_proceduralClosestHit;
                }
                else
                {
                    AZ_Assert(!shaderLibs.m_closestHit->m_closestHit.IsEmpty(), "ShaderLib has no ClosestHit Shader");
                    hitShaderCount++;
                    hitGroup.m_closestHit = shaderLibs.m_closestHit->m_closestHit;
                }
            }
            if (shaderLibs.m_anyHit)
            {
                AZ_Assert(!shaderLibs.m_anyHit->m_anyHit.IsEmpty(), "ShaderLib has no AnyHit Shader");
                hitShaderCount++;
                hitGroup.m_anyHit = shaderLibs.m_anyHit->m_anyHit;
            }
            if (shaderLibs.m_intersection)
            {
                AZ_Assert(!shaderLibs.m_intersection->m_intersection.IsEmpty(), "ShaderLib has no intersection Shader");
                hitGroup.m_intersection = shaderLibs.m_intersection->m_intersection;
            }
            AZ_Assert(hitShaderCount > 0, "Hit group needs at least one hit shader");
            m_hitGroups.push_back(hitGroup);
        }

        void RayTracingHitGroups::RegisterHitGroups(RHI::RayTracingPipelineStateDescriptor& descriptor)
        {
            for (auto& hitGroup : m_hitGroups)
            {
                descriptor.HitGroup(hitGroup.m_name);

                if (!hitGroup.m_closestHit.IsEmpty())
                {
                    descriptor.ClosestHitShaderName(hitGroup.m_closestHit);
                }
                if (!hitGroup.m_anyHit.IsEmpty())
                {
                    descriptor.AnyHitShaderName(hitGroup.m_anyHit);
                }
                if (!hitGroup.m_intersection.IsEmpty())
                {
                    descriptor.IntersectionShaderName(hitGroup.m_intersection);
                }
            }
        }

        const AZStd::vector<RayTracingHitGroups::HitGroup>& RayTracingHitGroups::GetHitGroups() const
        {
            return m_hitGroups;
        }

        void RayTracingHitGroups::Reset()
        {
            m_rayGenShader = AZ::Name{ "" };
            m_missShader = AZ::Name{ "" };
            m_hitGroups.clear();
        }

        AZStd::shared_ptr<RHI::RayTracingShaderTableDescriptor> RayTracingHitGroups::CreateRayTracingShaderTableDescriptor(
            RHI::Ptr<RHI::RayTracingPipelineState>& rayTracingPipelineState) const
        {
            AZStd::shared_ptr<RHI::RayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<RHI::RayTracingShaderTableDescriptor>();

            descriptor->Build(AZ::Name("RayTracingShaderTable"), rayTracingPipelineState)
                ->RayGenerationRecord(m_rayGenShader)
                ->MissRecord(m_missShader);

            // choose the Hit-groups
            for (auto& hitGroup : m_hitGroups)
            {
                descriptor->HitGroupRecord(hitGroup.m_name);
            }
            return descriptor;
        }

        RPI::Ptr<RayTracingPass> RayTracingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RayTracingPass> pass = aznew RayTracingPass(descriptor);
            return pass;
        }

        RayTracingPass::RayTracingPass(const RPI::PassDescriptor& descriptor)
            : RenderPass(descriptor)
            , m_passDescriptor(descriptor)
            , m_dispatchRaysItem(RHI::RHISystemInterface::Get()->GetRayTracingSupport())
        {
            m_flags.m_canBecomeASubpass = false;
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices)
            {
                // raytracing is not supported on this platform
                SetEnabled(false);
                return;
            }

            m_passData = RPI::PassUtils::GetPassData<RayTracingPassData>(m_passDescriptor);
            if (m_passData == nullptr)
            {
                AZ_Error("PassSystem", false, "RayTracingPass [%s]: Invalid RayTracingPassData", GetPathName().GetCStr());
                return;
            }

            m_indirectDispatch = m_passData->m_indirectDispatch;
            m_indirectDispatchBufferSlotName = m_passData->m_indirectDispatchBufferSlotName;

            m_fullscreenDispatch = m_passData->m_fullscreenDispatch;
            m_fullscreenSizeSourceSlotName = m_passData->m_fullscreenSizeSourceSlotName;

            AZ_Assert(
                !(m_indirectDispatch && m_fullscreenDispatch),
                "[RaytracingPass '%s']: Only one of the dispatch options (indirect, fullscreen) can be active",
                GetPathName().GetCStr());

            m_defaultShaderAttachmentStage = RHI::ScopeAttachmentStage::RayTracingShader;

            // store the max ray length
            m_maxRayLength = m_passData->m_maxRayLength;

            LoadShaderLibs(m_passData);
            if (!ValidateShaderLibs(AZStd::array{ &m_meshShaders, &m_proceduralShaders }))
            {
                AZ_Error(
                    "PassSystem", false, "RayTracingPass [%s]: Failed to validate all raytracing shader libs", GetPathName().GetCStr());
                return;
            }
            PrepareSrgs();
        }

        RayTracingPass::~RayTracingPass()
        {
            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        void RayTracingPass::LoadProceduralShaderLibs(RayTracingFeatureProcessorInterface* rtfp)
        {
            m_proceduralShaders.Reset();
            const auto& proceduralGeometryTypes = rtfp->GetProceduralGeometryTypes();
            for (auto it = proceduralGeometryTypes.cbegin(); it != proceduralGeometryTypes.cend(); ++it)
            {
                m_proceduralShaders.AddShaderFunction(
                    RayTracingShaderLibs::ShaderFunctionType::Intersection,
                    it->m_intersectionShaderName.GetStringView(),
                    it->m_intersectionShader);
            }
        }

        bool ShaderEntryFunctionsSpecified(const RayTracingPassData* passData)
        {
            bool result = true;
            result &=
                (passData->m_rayGenerationShaderAssetReference.m_assetId.IsValid() ||
                 !passData->m_rayGenerationShaderAssetReference.m_filePath.empty());
            result &=
                (passData->m_missShaderAssetReference.m_assetId.IsValid() ||
                 !passData->m_closestHitProceduralShaderAssetReference.m_filePath.empty());
            result &=
                (passData->m_closestHitShaderAssetReference.m_assetId.IsValid() ||
                 !passData->m_closestHitShaderAssetReference.m_filePath.empty());
            // the procedural hit shader is optional
            return result;
        }

        void RayTracingPass::LoadShaderLibs(const RayTracingPassData* passData)
        {
            m_meshShaders.Reset();

            // make sure the Procedural Shader Libs are also reloaded
            m_proceduralGeometryTypeRevision = std::numeric_limits<uint32_t>::max();

            if (!ShaderEntryFunctionsSpecified(m_passData))
            {
                AZ_Error(
                    "PassSystem",
                    false,
                    "RayTracingPass [%s]: Either Raytracing Shader Functions or a DrawListTag needs to be specified",
                    GetPathName().GetCStr());
                return;
            }

            using Type = RayTracingShaderLibs::ShaderFunctionType;

            m_meshShaders.AddShaderFunction(
                Type::RayGen, passData->m_rayGenerationShaderName, passData->m_rayGenerationShaderAssetReference);
            m_meshShaders.AddShaderFunction(Type::ClosestHit, passData->m_closestHitShaderName, passData->m_closestHitShaderAssetReference);
            m_meshShaders.AddShaderFunction(Type::Miss, passData->m_missShaderName, passData->m_missShaderAssetReference);

            // The procedural hit shader is a normal ClosestHit shader, but it works together with
            // the procedural intersection shaders that are registered with the RayTracingFeatureProcessor.
            if (!passData->m_closestHitProceduralShaderName.empty())
            {
                m_meshShaders.AddShaderFunction(
                    RayTracingShaderLibs::ShaderFunctionType::ProceduralClosestHit,
                    passData->m_closestHitProceduralShaderName,
                    passData->m_closestHitProceduralShaderAssetReference);
            }
        }

        void RayTracingPass::PrepareHitGroups()
        {
            // We don't offer anything to specify the hit-groups directly, so we make the following assumptions about how they should be
            // created from the ShaderLibs:
            // - The ClosestHit shader-functions and AnyHit shader-functions are in separate lists, but end up in a Hit-Group based on the
            // index:
            //     With e.g. 2 ClosestHit and 3 AnyHit shaders we get 3 HitGroups, where the third hit-group doesn't have a ClosestHit
            //     shader.
            // - Each Intersection - shader is assigned in a (procedural) hit-group that is added after the existing Hit groups. Also each
            // procedural hit-group uses the same ClosestHit - shader specified in the 'ProceduralClosestHit' - passdata - field.

            m_hitGroups.Reset();
            using Type = RayTracingShaderLibs::ShaderFunctionType;

            const auto& shaderLibs = m_meshShaders.GetAssignedShaderLibraries();

            // RayGeneration shader
            m_hitGroups.SetRayGenerationShader(shaderLibs[static_cast<size_t>(Type::RayGen)].span().front());
            // MissShader
            m_hitGroups.SetMissShader(shaderLibs[static_cast<size_t>(Type::Miss)].span().front());

            // Hit-Groups for normal meshes: Either anyHit, closestHit or both, but they can't have an intersection shader
            const auto hitShaderCount =
                AZStd::max(shaderLibs[static_cast<size_t>(Type::AnyHit)].size(), shaderLibs[static_cast<size_t>(Type::ClosestHit)].size());
            for (int index = 0; index < hitShaderCount; ++index)
            {
                auto name = AZ::Name{ AZStd::string::format("HitGroup_%d", index) };

                HitGroupShaderLibs hitGroupShaderLibs;
                if (shaderLibs[static_cast<size_t>(Type::ClosestHit)].size() > index)
                {
                    hitGroupShaderLibs.m_closestHit = shaderLibs[static_cast<size_t>(Type::ClosestHit)][index];
                }
                if (shaderLibs[static_cast<size_t>(Type::AnyHit)].size() > index)
                {
                    hitGroupShaderLibs.m_anyHit = shaderLibs[static_cast<size_t>(Type::AnyHit)][index];
                }
                // no Intersection shader
                m_hitGroups.AddHitGroup(name, hitGroupShaderLibs);
            }

            // Hit-Groups for procedural meshes: We use the same closesthit for each procedural intersection shader
            const auto& proceduralShaderLibs = m_proceduralShaders.GetAssignedShaderLibraries();
            for (int index = 0; index < proceduralShaderLibs[static_cast<size_t>(Type::Intersection)].size(); index++)
            {
                auto name = AZ::Name{ AZStd::string::format("ProceduralHitGroup_%d", index) };
                HitGroupShaderLibs hitGroupShaderLibs;

                if (!shaderLibs[static_cast<size_t>(Type::ProceduralClosestHit)].empty())
                {
                    hitGroupShaderLibs.m_closestHit = shaderLibs[static_cast<size_t>(Type::ProceduralClosestHit)].span().front();
                }

                // support for procedural anyhit shaders would go here

                hitGroupShaderLibs.m_intersection = proceduralShaderLibs[static_cast<size_t>(Type::Intersection)][index];

                m_hitGroups.AddHitGroup(name, hitGroupShaderLibs, true);
            }
        }

        void RayTracingPass::PrepareSrgs()
        {
            auto* shader{ m_meshShaders.GetShaderForSrgs() };
            if (!shader)
            {
                AZ_Error(
                    "PassSystem",
                    shader != nullptr,
                    "RayTracingPass [%s] Failed to find any shader to determine the PassSrg layout",
                    GetPathName().GetCStr());
                return;
            }
            // create global srg
            const auto& globalSrgLayout = shader->FindShaderResourceGroupLayout(RayTracingGlobalSrgBindingSlot);
            AZ_Error(
                "PassSystem",
                globalSrgLayout != nullptr,
                "RayTracingPass [%s] Failed to find RayTracingGlobalSrg layout",
                GetPathName().GetCStr());

            m_shaderResourceGroup =
                RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), globalSrgLayout->GetName());
            AZ_Assert(m_shaderResourceGroup, "RayTracingPass [%s]: Failed to create RayTracingGlobalSrg", GetPathName().GetCStr());
            RPI::PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());

            // check to see if the shader requires the View, Scene, or RayTracingMaterial Srgs
            const auto& viewSrgLayout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::View);
            m_requiresViewSrg = (viewSrgLayout != nullptr);

            const auto& sceneSrgLayout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Scene);
            m_requiresSceneSrg = (sceneSrgLayout != nullptr);

            const auto& rayTracingMaterialSrgLayout = shader->FindShaderResourceGroupLayout(RayTracingMaterialSrgBindingSlot);
            m_requiresRayTracingMaterialSrg = (rayTracingMaterialSrgLayout != nullptr);

            const auto& rayTracingSceneSrgLayout = shader->FindShaderResourceGroupLayout(RayTracingSceneSrgBindingSlot);
            m_requiresRayTracingSceneSrg = (rayTracingSceneSrgLayout != nullptr);
        }

        void RayTracingPass::CreatePipelineState()
        {
            m_rayTracingShaderTable.reset();
            m_maxRayLengthInputIndex.Reset();

            if (!ValidateShaderLibs(AZStd::array{ &m_meshShaders, &m_proceduralShaders }))
            {
                AZ_Error(
                    "PassSystem", false, "RayTracingPass [%s]: Failed to validate all raytracing shader libs", GetPathName().GetCStr());
                return;
            }

            // by now we should have all hit shaders, the main and the procedural ones, and we can prepare the hit groups
            PrepareHitGroups();

            auto* rayGenShaderLib{ m_meshShaders.GetRayGenShaderLib() };
            auto& rayGenShader{ rayGenShaderLib->m_shader };
            auto& rayGenPipelineStateDesc{ rayGenShaderLib->m_pipelineStateDescriptor };

            m_globalPipelineState = rayGenShader->AcquirePipelineState(rayGenPipelineStateDesc);
            AZ_Assert(m_globalPipelineState, "Failed to acquire ray tracing global pipeline state");

            // build the ray tracing pipeline state descriptor
            RHI::RayTracingPipelineStateDescriptor descriptor;
            descriptor.Build()
                ->PipelineState(m_globalPipelineState.get())
                ->MaxPayloadSize(m_passData->m_maxPayloadSize)
                ->MaxAttributeSize(m_passData->m_maxAttributeSize)
                ->MaxRecursionDepth(m_passData->m_maxRecursionDepth);

            m_meshShaders.RegisterShaderLibraries(descriptor);
            m_proceduralShaders.RegisterShaderLibraries(descriptor);

            m_hitGroups.RegisterHitGroups(descriptor);

            // create the ray tracing pipeline state object
            m_rayTracingPipelineState = aznew RHI::RayTracingPipelineState;
            m_rayTracingPipelineState->Init(RHI::RHISystemInterface::Get()->GetRayTracingSupport(), descriptor);

            // register the ray tracing and global pipeline state object with the dispatch-item
            m_dispatchRaysItem.SetRayTracingPipelineState(m_rayTracingPipelineState.get());
            m_dispatchRaysItem.SetPipelineState(m_globalPipelineState.get());

            RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();

            for (auto& [assetId, _] : m_meshShaders.GetShaderLibraries())
            {
                RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(assetId);
            }
            // reloading of the procedural Shaders happens via the proceduralGeometryTypeRevision
        }

        bool RayTracingPass::IsEnabled() const
        {
            if (!RenderPass::IsEnabled())
            {
                return false;
            }

            if (m_pipeline == nullptr)
            {
                return false;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (!rayTracingFeatureProcessor)
            {
                return false;
            }

            return true;
        }

        void RayTracingPass::BuildInternal()
        {
            if (m_indirectDispatch)
            {
                if (!m_indirectDispatchRaysBufferSignature)
                {
                    AZ::RHI::IndirectBufferLayout bufferLayout;
                    bufferLayout.AddIndirectCommand(AZ::RHI::IndirectCommandDescriptor(AZ::RHI::IndirectCommandType::DispatchRays));

                    if (!bufferLayout.Finalize())
                    {
                        AZ_Assert(false, "Fail to finalize Indirect Layout");
                    }

                    m_indirectDispatchRaysBufferSignature = aznew AZ::RHI::IndirectBufferSignature();
                    AZ::RHI::IndirectBufferSignatureDescriptor signatureDescriptor{};
                    signatureDescriptor.m_layout = bufferLayout;
                    [[maybe_unused]] auto result = m_indirectDispatchRaysBufferSignature->Init(
                        AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), signatureDescriptor);

                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Fail to initialize Indirect Buffer Signature");
                }

                m_indirectDispatchRaysBufferBinding = nullptr;
                if (!m_indirectDispatchBufferSlotName.IsEmpty())
                {
                    m_indirectDispatchRaysBufferBinding = FindAttachmentBinding(m_indirectDispatchBufferSlotName);
                    AZ_Assert(
                        m_indirectDispatchRaysBufferBinding,
                        "[RaytracingPass '%s']: Indirect dispatch buffer slot %s not found.",
                        GetPathName().GetCStr(),
                        m_indirectDispatchBufferSlotName.GetCStr());
                    if (m_indirectDispatchRaysBufferBinding)
                    {
                        AZ_Assert(
                            m_indirectDispatchRaysBufferBinding->m_scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Indirect,
                            "[RaytracingPass '%s']: Indirect dispatch buffer slot %s needs ScopeAttachmentUsage::Indirect.",
                            GetPathName().GetCStr(),
                            m_indirectDispatchBufferSlotName.GetCStr())
                    }
                }
                else
                {
                    for (auto& binding : m_attachmentBindings)
                    {
                        if (binding.m_scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Indirect)
                        {
                            m_indirectDispatchRaysBufferBinding = &binding;
                            break;
                        }
                    }
                    AZ_Assert(
                        m_indirectDispatchRaysBufferBinding,
                        "[RaytracingPass '%s']: No valid indirect dispatch buffer slot found.",
                        GetPathName().GetCStr());
                }

                if (!m_dispatchRaysIndirectBuffer)
                {
                    m_dispatchRaysIndirectBuffer =
                        aznew AZ::RHI::DispatchRaysIndirectBuffer{ AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() };
                    m_dispatchRaysIndirectBuffer->Init(
                        AZ::RPI::BufferSystemInterface::Get()->GetCommonBufferPool(AZ::RPI::CommonBufferPoolType::Indirect).get());
                }
            }
            else if (m_fullscreenDispatch)
            {
                m_fullscreenSizeSourceBinding = nullptr;
                if (!m_fullscreenSizeSourceSlotName.IsEmpty())
                {
                    m_fullscreenSizeSourceBinding = FindAttachmentBinding(m_fullscreenSizeSourceSlotName);
                    AZ_Assert(
                        m_fullscreenSizeSourceBinding,
                        "[RaytracingPass '%s']: Fullscreen size source slot %s not found.",
                        GetPathName().GetCStr(),
                        m_fullscreenSizeSourceSlotName.GetCStr());
                }
                else
                {
                    if (GetOutputCount() > 0)
                    {
                        m_fullscreenSizeSourceBinding = &GetOutputBinding(0);
                    }
                    else if (!m_fullscreenSizeSourceBinding && GetInputOutputCount() > 0)
                    {
                        m_fullscreenSizeSourceBinding = &GetInputOutputBinding(0);
                    }
                    AZ_Assert(
                        m_fullscreenSizeSourceBinding,
                        "[RaytracingPass '%s']: No valid Output or InputOutput slot as a fullscreen size source found.",
                        GetPathName().GetCStr());
                }
            }
            // Load the procedural shader libs from the featureprocessor as soon as we are able, since we need it for the Pipeline State
            RPI::Scene* scene = m_pipeline->GetScene();
            if (scene)
            {
                RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
                if (rayTracingFeatureProcessor)
                {
                    uint32_t proceduralGeometryTypeRevision = rayTracingFeatureProcessor->GetProceduralGeometryTypeRevision();
                    if (m_proceduralGeometryTypeRevision != proceduralGeometryTypeRevision)
                    {
                        LoadProceduralShaderLibs(rayTracingFeatureProcessor);
                        CreatePipelineState();
                        m_proceduralGeometryTypeRevision = proceduralGeometryTypeRevision;
                    }
                }
            }
        }

        void RayTracingPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (!rayTracingFeatureProcessor)
            {
                return;
            }

            RPI::RenderPass::FrameBeginInternal(params);
        }

        void RayTracingPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
            frameGraph.SetEstimatedItemCount(1);

            // TLAS
            {
                const RHI::Ptr<RHI::Buffer>& rayTracingTlasBuffer = rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer();
                if (rayTracingTlasBuffer)
                {
                    AZ::RHI::AttachmentId tlasAttachmentId = rayTracingFeatureProcessor->GetTlasAttachmentId();
                    if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(tlasAttachmentId) == false)
                    {
                        [[maybe_unused]] RHI::ResultCode result =
                            frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, rayTracingTlasBuffer);
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);
                    }

                    uint32_t tlasBufferByteCount =
                        aznumeric_cast<uint32_t>(rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer()->GetDescriptor().m_byteCount);
                    RHI::BufferViewDescriptor tlasBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = tlasAttachmentId;
                    desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::RayTracingShader);
                }
            }
        }

        void RayTracingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");

            if (m_indirectDispatch)
            {
                if (m_indirectDispatchRaysBufferBinding)
                {
                    auto& attachment{ m_indirectDispatchRaysBufferBinding->GetAttachment() };
                    AZ_Assert(
                        attachment,
                        "[RayTracingPass '%s']: Indirect dispatch buffer slot %s has no attachment.",
                        GetPathName().GetCStr(),
                        m_indirectDispatchRaysBufferBinding->m_name.GetCStr());

                    if (attachment)
                    {
                        auto* indirectDispatchBuffer{ context.GetBuffer(attachment->GetAttachmentId()) };
                        m_indirectDispatchRaysBufferView = AZ::RHI::IndirectBufferView{ *indirectDispatchBuffer,
                                                                                        *m_indirectDispatchRaysBufferSignature,
                                                                                        0,
                                                                                        sizeof(DispatchRaysIndirectCommand),
                                                                                        sizeof(DispatchRaysIndirectCommand) };

                        RHI::DispatchRaysIndirect dispatchRaysArgs(
                            1, m_indirectDispatchRaysBufferView, 0, m_dispatchRaysIndirectBuffer.get());

                        m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
                    }
                }
            }
            else if (m_fullscreenDispatch)
            {
                auto& attachment = m_fullscreenSizeSourceBinding->GetAttachment();
                AZ_Assert(
                    attachment,
                    "[RaytracingPass '%s']: Slot %s has no attachment for fullscreen size source.",
                    GetPathName().GetCStr(),
                    m_fullscreenSizeSourceBinding->m_name.GetCStr());
                AZ::RHI::DispatchRaysDirect dispatchRaysArgs;
                if (attachment)
                {
                    AZ_Assert(
                        attachment->GetAttachmentType() == AZ::RHI::AttachmentType::Image,
                        "[RaytracingPass '%s']: Slot %s must be an image for fullscreen size source.",
                        GetPathName().GetCStr(),
                        m_fullscreenSizeSourceBinding->m_name.GetCStr());

                    auto imageDescriptor = context.GetImageDescriptor(attachment->GetAttachmentId());
                    dispatchRaysArgs.m_width = imageDescriptor.m_size.m_width;
                    dispatchRaysArgs.m_height = imageDescriptor.m_size.m_height;
                    dispatchRaysArgs.m_depth = imageDescriptor.m_size.m_depth;
                }
                m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
            }
            else
            {
                AZ::RHI::DispatchRaysDirect dispatchRaysArgs{ m_passData->m_threadCountX,
                                                              m_passData->m_threadCountY,
                                                              m_passData->m_threadCountZ };
                m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
            }

            uint32_t proceduralGeometryTypeRevision = rayTracingFeatureProcessor->GetProceduralGeometryTypeRevision();
            if (m_proceduralGeometryTypeRevision != proceduralGeometryTypeRevision)
            {
                LoadProceduralShaderLibs(rayTracingFeatureProcessor);

                CreatePipelineState();

                RPI::SceneNotificationBus::Event(
                    GetScene()->GetId(),
                    &RPI::SceneNotification::OnRenderPipelineChanged,
                    GetRenderPipeline(),
                    RPI::SceneNotification::RenderPipelineChangeType::PassChanged);
                m_proceduralGeometryTypeRevision = proceduralGeometryTypeRevision;
            }

            if (!m_rayTracingShaderTable || m_rayTracingShaderTableRevision != rayTracingFeatureProcessor->GetRevision())
            {
                // scene changed, need to rebuild the shader table
                m_rayTracingShaderTableRevision = rayTracingFeatureProcessor->GetRevision();
                m_rayTracingShaderTable = aznew AZ::RHI::RayTracingShaderTable();
                m_rayTracingShaderTable->Init(
                    AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), rayTracingFeatureProcessor->GetBufferPools());

                auto shaderTableDescriptor = m_hitGroups.CreateRayTracingShaderTableDescriptor(m_rayTracingPipelineState);

                // We need to provide a corresponding hit group for each hit group the tlas expects.
                // auto TLasHitGroupCount = rayTracingFeatureProcessor->GetCurrentHitGroupCount();
                // TODO: currently this is estimated as one single hit group for meshes, and one hit group per procedural geometry type
                auto TLasHitGroupCount = rayTracingFeatureProcessor->GetProceduralGeometryTypes().size() + 1;

                AZ_Assert(
                    m_hitGroups.GetHitGroups().size() == TLasHitGroupCount,
                    "Not every hit-group in the Raytracing Scene has a corresponding hit-shader");

                m_rayTracingShaderTable->Build(shaderTableDescriptor);

                // register the shader-table with the dispatch item
                m_dispatchRaysItem.SetRayTracingPipelineState(m_rayTracingPipelineState.get());
                m_dispatchRaysItem.SetRayTracingShaderTable(m_rayTracingShaderTable.get());
            }

            // Collect and register the Srgs (RayTracingGlobal, RayTracingScene, ViewSrg, SceneSrg and RayTracingMaterialSrg)
            // The more consistent way would be to call BindSrg() of the RenderPass, and then call
            // SetSrgsForDispatchRays() in BuildCommandListInternal, but that function doesn't exist.
            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderResourceGroup->SetConstant(m_maxRayLengthInputIndex, m_maxRayLength);
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
                m_rayTracingSRGsToBind.push_back(m_shaderResourceGroup->GetRHIShaderResourceGroup());
            }

            if (m_requiresRayTracingSceneSrg)
            {
                m_rayTracingSRGsToBind.push_back(rayTracingFeatureProcessor->GetRayTracingSceneSrg()->GetRHIShaderResourceGroup());
            }

            if (m_requiresViewSrg)
            {
                RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
                if (view)
                {
                    m_rayTracingSRGsToBind.push_back(view->GetShaderResourceGroup()->GetRHIShaderResourceGroup());
                }
            }

            if (m_requiresSceneSrg)
            {
                m_rayTracingSRGsToBind.push_back(scene->GetShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            if (m_requiresRayTracingMaterialSrg)
            {
                m_rayTracingSRGsToBind.push_back(rayTracingFeatureProcessor->GetRayTracingMaterialSrg()->GetRHIShaderResourceGroup());
            }
        }

        void RayTracingPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingPass requires the RayTracingFeatureProcessor");
            AZ_Assert(
                RHI::CheckBit(rayTracingFeatureProcessor->GetDeviceMask(), context.GetDeviceIndex()),
                "RayTracingPass cannot run on a device without a RayTracingAccelerationStructurePass");

            if (!rayTracingFeatureProcessor || !rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() ||
                !rayTracingFeatureProcessor->HasGeometry() || !m_rayTracingShaderTable)
            {
                return;
            }

            if (m_dispatchRaysShaderTableRevision != m_rayTracingShaderTableRevision)
            {
                m_dispatchRaysShaderTableRevision = m_rayTracingShaderTableRevision;
                if (m_dispatchRaysIndirectBuffer)
                {
                    m_dispatchRaysIndirectBuffer->Build(m_rayTracingShaderTable.get());
                }
            }

            // TODO: change this to BindSrgsForDispatchRays() as soon as it exists
            // IMPORTANT: The data in shaderResourceGroups must be sorted by (entry)->GetBindingSlot() (FrequencyId value in SRG source
            // file from SrgSemantics.azsli) in order for them to be correctly assigned by Vulkan
            AZStd::sort(
                m_rayTracingSRGsToBind.begin(),
                m_rayTracingSRGsToBind.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    return lhs->GetBindingSlot() < rhs->GetBindingSlot();
                });
            m_dispatchRaysItem.SetShaderResourceGroups(m_rayTracingSRGsToBind.data(), static_cast<uint32_t>(m_rayTracingSRGsToBind.size()));

            // submit the DispatchRays item
            context.GetCommandList()->Submit(m_dispatchRaysItem.GetDeviceDispatchRaysItem(context.GetDeviceIndex()));
        }

        void RayTracingPass::FrameEndInternal()
        {
            m_rayTracingSRGsToBind.clear();
        }

        void RayTracingPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader& shader)
        {
            LoadShaderLibs(m_passData);
            PrepareSrgs();
            CreatePipelineState();
        }

        void RayTracingPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            LoadShaderLibs(m_passData);
            PrepareSrgs();
            CreatePipelineState();
        }

        void RayTracingPass::OnShaderVariantReinitialized(const RPI::ShaderVariant&)
        {
            LoadShaderLibs(m_passData);
            PrepareSrgs();
            CreatePipelineState();
        }
    } // namespace Render
} // namespace AZ
