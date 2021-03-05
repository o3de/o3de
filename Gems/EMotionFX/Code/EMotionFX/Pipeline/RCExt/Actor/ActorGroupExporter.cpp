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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AutoRegisteredActor.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>
#include <SceneAPIExt/Rules/SimulatedObjectSetupRule.h>

#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <RCExt/Actor/ActorGroupExporter.h>
#include <RCExt/ExportContexts.h>

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
                serializeContext->Class<ActorGroupExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
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
                if (!CommandSystem::MetaData::ApplyMetaDataOnActor(m_actor.get(), metaDataString))
                {
                    AZ_Error("EMotionFX", false, "Applying meta data to actor '%s' failed.", m_actor->GetName());
                }
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

            ExporterLib::SaveActor(filename, m_actor.get(), MCore::Endian::ENDIAN_LITTLE);

#ifdef EMOTIONFX_ACTOR_DEBUG
            // Use there line to create a log file and inspect detail debug info
            AZStd::string folderPath;
            AzFramework::StringFunc::Path::GetFolderPath(filename.c_str(), folderPath);
            AZStd::string logFilename = folderPath;
            logFilename += "EMotionFXExporter_Log.txt";
            MCore::GetLogManager().CreateLogFile(logFilename.c_str());
            EMotionFX::GetImporter().SetLogDetails(true);
            filename += ".xac";

            // use this line to load the actor from the saved actor file
            EMotionFX::Actor* testLoadingActor = EMotionFX::GetImporter().LoadActor(AZStd::string(filename.c_str()));
            MCore::Destroy(testLoadingActor);
#endif // EMOTIONFX_ACTOR_DEBUG

            static AZ::Data::AssetType emotionFXActorAssetType("{F67CC648-EA51-464C-9F5D-4A9CE41A7F86}"); // from ActorAsset.h in EMotionFX Gem
            AZ::SceneAPI::Events::ExportProduct& product = context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), emotionFXActorAssetType,
                AZStd::nullopt, AZStd::nullopt);

            for (AZStd::string& materialPathReference : m_actorMaterialReferences)
            {
                product.m_legacyPathDependencies.emplace_back(AZStd::move(materialPathReference));
            }

            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace Pipeline
} // namespace EMotionFX
