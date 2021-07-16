/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
