/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <RCExt/Actor/ActorExporter.h>
#include <RCExt/ExportContexts.h>

#include <AzToolsFramework/Debug/TraceContext.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneEvents = AZ::SceneAPI::Events;

        ActorExporter::ActorExporter()
        {
            BindToCall(&ActorExporter::ProcessContext);
        }

        void ActorExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ActorExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        uint8_t ActorExporter::GetPriority() const
        {
            // Set the priority within the SceneAPI exporter processes and process actors after other exporters using the SceneAPI.
            return CallProcessor::LatestProcessing;
        }

        SceneEvents::ProcessingResult ActorExporter::ProcessContext(SceneEvents::ExportEventContext& context) const
        {
            const SceneContainers::SceneManifest& manifest = context.GetScene().GetManifest();

            auto valueStorage = manifest.GetValueStorage();
            auto view = SceneContainers::MakeDerivedFilterView<Group::IActorGroup>(valueStorage);

            SceneEvents::ProcessingResultCombiner result;
            for (const Group::IActorGroup& actorGroup : view)
            {
                AZ_TraceContext("Actor group", actorGroup.GetName());
                result += SceneEvents::Process<ActorGroupExportContext>(context, actorGroup, AZ::RC::Phase::Construction);
                result += SceneEvents::Process<ActorGroupExportContext>(context, actorGroup, AZ::RC::Phase::Filling);
                result += SceneEvents::Process<ActorGroupExportContext>(context, actorGroup, AZ::RC::Phase::Finalizing);
            }
            return result.GetResult();
        }
    } // namespace Pipeline
} // namespace EMotionFX
