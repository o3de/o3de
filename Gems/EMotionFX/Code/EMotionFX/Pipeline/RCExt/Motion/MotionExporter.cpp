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

#include <AzToolsFramework/Debug/TraceContext.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <SceneAPIExt/Groups/IMotionGroup.h>
#include <RCExt/Motion/MotionExporter.h>
#include <RCExt/ExportContexts.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneEvents = AZ::SceneAPI::Events;

        MotionExporter::MotionExporter()
        {
            BindToCall(&MotionExporter::ProcessContext);
        }

        void MotionExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MotionExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult MotionExporter::ProcessContext(SceneEvents::ExportEventContext& context) const
        {
            const SceneContainers::SceneManifest& manifest = context.GetScene().GetManifest();

            auto valueStorage = manifest.GetValueStorage();
            auto view = SceneContainers::MakeDerivedFilterView<Group::IMotionGroup>(valueStorage);

            SceneEvents::ProcessingResultCombiner result;
            for (const Group::IMotionGroup& motionGroup : view)
            {
                AZ_TraceContext("Animation group", motionGroup.GetName());
                result += SceneEvents::Process<MotionGroupExportContext>(context, motionGroup, AZ::RC::Phase::Construction);
                result += SceneEvents::Process<MotionGroupExportContext>(context, motionGroup, AZ::RC::Phase::Filling);
                result += SceneEvents::Process<MotionGroupExportContext>(context, motionGroup, AZ::RC::Phase::Finalizing);
            }
            return result.GetResult();
        }
    } // namespace Pipeline
} // namespace EMotionFX
