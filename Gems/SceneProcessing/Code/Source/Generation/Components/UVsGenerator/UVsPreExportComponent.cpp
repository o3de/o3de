/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/UVsGenerator/UVsPreExportComponent.h>

#include <AzCore/RTTI/RTTI.h>

#include <Generation/Components/UVsGenerator/UVsGenerateComponent.h> // for the context
#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ::SceneGenerationComponents
{
    //! This is the component responsible for actually hooking into the scene API's processing flow
    //! during the generation step.
    class UVsPreExportComponent : public AZ::SceneAPI::SceneCore::GenerationComponent
    {
    public:
        AZ_COMPONENT(UVsPreExportComponent, s_UVsPreExportComponentTypeId, AZ::SceneAPI::SceneCore::GenerationComponent);
        UVsPreExportComponent();

        static void Reflect(AZ::ReflectContext* context);

        AZ::SceneAPI::Events::ProcessingResult Register(AZ::SceneAPI::Events::GenerateAdditionEventContext& context);
    };

    AZ::ComponentDescriptor* CreateUVsPreExportComponentDescriptor()
    {
        return UVsPreExportComponent::CreateDescriptor();
    }

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
