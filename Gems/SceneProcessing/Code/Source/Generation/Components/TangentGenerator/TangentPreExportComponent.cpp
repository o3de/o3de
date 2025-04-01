/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/TangentGenerator/TangentPreExportComponent.h>
#include <Generation/Components/TangentGenerator/TangentGenerateComponent.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ::SceneGenerationComponents
{
    namespace SceneEvents = AZ::SceneAPI::Events;

    TangentPreExportComponent::TangentPreExportComponent()
    {
        BindToCall(&TangentPreExportComponent::Register);
    }

    uint8_t TangentPreExportComponent::GetPriority() const
    {
        return AZ::SceneAPI::Events::CallProcessor::ProcessingPriority::LateProcessing;
    }


    void TangentPreExportComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TangentPreExportComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(1);
        }
    }

    AZ::SceneAPI::Events::ProcessingResult TangentPreExportComponent::Register(AZ::SceneAPI::Events::GenerateAdditionEventContext& context)
    {
        SceneEvents::ProcessingResultCombiner result;
        TangentGenerateContext tangentGenerateContext(context.GetScene());
        result += SceneEvents::Process<TangentGenerateContext>(tangentGenerateContext);
        return result.GetResult();
    }
} // namespace AZ::SceneGenerationComponents
