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
