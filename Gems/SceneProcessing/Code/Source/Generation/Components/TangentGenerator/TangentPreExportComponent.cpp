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
        return SceneEvents::ProcessingResult::Success;
    }
} // namespace AZ::SceneGenerationComponents
