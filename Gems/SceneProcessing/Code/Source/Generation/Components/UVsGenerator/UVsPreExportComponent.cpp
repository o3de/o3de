/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/UVsGenerator/UVsPreExportComponent.h>
#include <Generation/Components/UVsGenerator/UVsGenerateComponent.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ::SceneGenerationComponents
{
    namespace SceneEvents = AZ::SceneAPI::Events;

    UVsPreExportComponent::UVsPreExportComponent()
    {
        BindToCall(&UVsPreExportComponent::Register);
    }

    void UVsPreExportComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<UVsPreExportComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(1);
        }
    }

    AZ::SceneAPI::Events::ProcessingResult UVsPreExportComponent::Register(AZ::SceneAPI::Events::GenerateAdditionEventContext& context)
    {
        SceneEvents::ProcessingResultCombiner result;
        UVsGenerateContext uvsGenerateContext(context.GetScene());
        result += SceneEvents::Process<UVsGenerateContext>(uvsGenerateContext);
        return result.GetResult();
    }
} // namespace AZ::SceneGenerationComponents
