/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <Source/Integration/Assets/ActorAsset.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>
#include <SceneAPIExt/Rules/SimulatedObjectSetupRule.h>

#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <RCExt/Actor/ActorGroupExporter.h>
#include <RCExt/ExportContexts.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;

        ActorGroupExporter::ActorGroupExporter()
        {
            BindToCall(&ActorGroupExporter::ProcessContext);
        }

        Actor* ActorGroupExporter::GetActor() const
        {
            return m_actor.get();
        }

        void ActorGroupExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // Increasing the version number of the actor group exporter will make sure all actor products will be force re-generated.
                serializeContext->Class<ActorGroupExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(4);
            }
        }

        SceneEvents::ProcessingResult ActorGroupExporter::ProcessContext(ActorGroupExportContext& context)
        {
            if (context.m_phase == AZ::RC::Phase::Construction)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }
            if (context.m_phase == AZ::RC::Phase::Finalizing)
            {
                return SaveActor(context);
            }

            m_actor = AZStd::make_shared<Actor>(context.m_group.GetName().c_str());
            if (!m_actor)
            {
                return SceneEvents::ProcessingResult::Failure;
            }

            SceneEvents::ProcessingResultCombiner result;

            const Group::IActorGroup& actorGroup = context.m_group;
            ActorBuilderContext actorBuilderContext(context.m_scene, context.m_outputDirectory, actorGroup, m_actor.get(), m_actorMaterialReferences, AZ::RC::Phase::Construction);

            result += SceneEvents::Process(actorBuilderContext);
            result += SceneEvents::Process<ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
            result += SceneEvents::Process<ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);

            // Check if there is meta data and apply it to the actor.
            AZStd::string metaDataString;
            if (Rule::MetaDataRule::LoadMetaData(actorGroup, metaDataString))
            {
                // Create a temporary actor asset as the commands use the actor manager to find the corresponding actor object
                // and our actor can only be found as part of a registered actor asset.
                const AZ::Data::AssetId actorAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
                AZ::Data::Asset<Integration::ActorAsset> actorAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::ActorAsset>(actorAssetId);
                actorAsset.GetAs<Integration::ActorAsset>()->SetData(m_actor);
                GetEMotionFX().GetActorManager()->RegisterActor(actorAsset);

                if (!CommandSystem::MetaData::ApplyMetaDataOnActor(m_actor.get(), metaDataString))
                {
                    AZ_Error("EMotionFX", false, "Applying meta data to actor '%s' failed.", m_actor->GetName());
                }

                GetEMotionFX().GetActorManager()->UnregisterActor(actorAsset->GetId());
            }

            AZStd::shared_ptr<EMotionFX::PhysicsSetup> physicsSetup;
            if (EMotionFX::Pipeline::Rule::LoadFromGroup<EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule, AZStd::shared_ptr<EMotionFX::PhysicsSetup>>(actorGroup, physicsSetup))
            {
                m_actor->SetPhysicsSetup(physicsSetup);
            }

            AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup> simulatedObjectSetup;
            if (EMotionFX::Pipeline::Rule::LoadFromGroup<EMotionFX::Pipeline::Rule::SimulatedObjectSetupRule>(actorGroup, simulatedObjectSetup))
            {
                m_actor->SetSimulatedObjectSetup(simulatedObjectSetup);
            }

            return result.GetResult();
        }

        AZ::SceneAPI::Events::ProcessingResult ActorGroupExporter::SaveActor(ActorGroupExportContext& context)
        {
            AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(), context.m_outputDirectory, "");
            if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
            {
                return SceneEvents::ProcessingResult::Failure;
            }

            ExporterLib::SaveActor(filename, m_actor.get(), MCore::Endian::ENDIAN_LITTLE, GetMeshAssetId(context));

            static AZ::Data::AssetType emotionFXActorAssetType("{F67CC648-EA51-464C-9F5D-4A9CE41A7F86}"); // from ActorAsset.h in EMotionFX Gem
            AZ::SceneAPI::Events::ExportProduct& product = context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), emotionFXActorAssetType,
                AZStd::nullopt, AZStd::nullopt);

            for (AZStd::string& materialPathReference : m_actorMaterialReferences)
            {
                product.m_legacyPathDependencies.emplace_back(AZStd::move(materialPathReference));
            }

            // Mesh asset, skin meta asset and morph target meta asset are sub assets for actor asset.
            // In here we set them as the dependency of the actor asset. That make sure those assets get automatically loaded before actor asset.
            // Default to the first product until we are able to establish a link between mesh and actor (ATOM-13590).
            const AZ::Data::AssetType assetDependencyList[] = {
                azrtti_typeid<AZ::RPI::ModelAsset>(),
                azrtti_typeid<AZ::RPI::SkinMetaAsset>(),
                azrtti_typeid<AZ::RPI::MorphTargetMetaAsset>()
            };

            for (const AZ::Data::AssetType& assetDependency : assetDependencyList)
            {
                AZStd::optional<AZ::SceneAPI::Events::ExportProduct> result = GetFirstProductByType(context, assetDependency);
                if (result != AZStd::nullopt)
                {
                    AZ::SceneAPI::Events::ExportProduct exportProduct = result.value();
                    exportProduct.m_dependencyFlags = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad);
                    product.m_productDependencies.emplace_back(exportProduct);
                }
            }

            return SceneEvents::ProcessingResult::Success;
        }

        AZStd::optional<AZ::Data::AssetId> ActorGroupExporter::GetMeshAssetId(const ActorGroupExportContext& context) const
        {
            AZ::Data::AssetType atomModelAssetType = azrtti_typeid<AZ::RPI::ModelAsset>();
            const AZStd::vector<AZ::SceneAPI::Events::ExportProduct>& products = context.m_products.GetProducts();

            // Gather the asset ids from the exported mesh groups (Atom model assets).
            AZStd::vector<AZ::SceneAPI::Events::ExportProduct> atomModelAssets;
            for (const AZ::SceneAPI::Events::ExportProduct& product : products)
            {
                if (product.m_assetType == atomModelAssetType)
                {
                    AZ_Assert(product.m_id == context.m_scene.GetSourceGuid(), "Source asset uuid differs from the Atom model product uuid.");
                    atomModelAssets.push_back(product);
                }
            }

            // Default to the first mesh group until we get a way to choose it via the scene settings (ATOM-13590).
            AZ_Error("EMotionFX", atomModelAssets.size() <= 1, "Ambigious mesh for actor asset. More than one mesh group found. Defaulting to the first one.");
            if (!atomModelAssets.empty())
            {
                const AZ::SceneAPI::Events::ExportProduct& meshProduct = atomModelAssets[0];
                return AZ::Data::AssetId(meshProduct.m_id, meshProduct.m_subId.value());
            }

            return AZStd::nullopt;
        }

        AZStd::optional<AZ::SceneAPI::Events::ExportProduct> ActorGroupExporter::GetFirstProductByType(
            const ActorGroupExportContext& context, AZ::Data::AssetType type)
        {
            const AZStd::vector<AZ::SceneAPI::Events::ExportProduct>& products = context.m_products.GetProducts();
            for (const AZ::SceneAPI::Events::ExportProduct& product : products)
            {
                if (product.m_assetType == type)
                {
                    return product;
                }
            }

            return AZStd::nullopt;
        }
    } // namespace Pipeline
} // namespace EMotionFX
