/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignalEditorModule.h>
#include <Editor/EditorSurfaceAltitudeGradientComponent.h>
#include <Editor/EditorSmoothStepGradientComponent.h>
#include <Editor/EditorSurfaceSlopeGradientComponent.h>
#include <Editor/EditorMixedGradientComponent.h>
#include <Editor/EditorImageGradientComponent.h>
#include <Editor/EditorConstantGradientComponent.h>
#include <Editor/EditorThresholdGradientComponent.h>
#include <Editor/EditorLevelsGradientComponent.h>
#include <Editor/EditorReferenceGradientComponent.h>
#include <Editor/EditorInvertGradientComponent.h>
#include <Editor/EditorDitherGradientComponent.h>
#include <Editor/EditorPosterizeGradientComponent.h>
#include <Editor/EditorShapeAreaFalloffGradientComponent.h>
#include <Editor/EditorPerlinGradientComponent.h>
#include <Editor/EditorRandomGradientComponent.h>
#include <Editor/EditorGradientTransformComponent.h>
#include <Editor/EditorStreamingImageAssetCtrl.h>
#include <Editor/EditorSurfaceMaskGradientComponent.h>
#include <Editor/EditorGradientSurfaceDataComponent.h>
#include <GradientSignal/Editor/GradientPreviewer.h>
#include <GradientSignal/Editor/EditorGradientBakerComponent.h>
#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <GradientSignal/Editor/PaintableImageAssetHelper.h>
#include <UI/GradientPreviewDataWidget.h>

namespace GradientSignal
{
    GradientSignalEditorModule::GradientSignalEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            GradientSignalEditorSystemComponent::CreateDescriptor(),

            EditorGradientBakerComponent::CreateDescriptor(),
            EditorSurfaceAltitudeGradientComponent::CreateDescriptor(),
            EditorSmoothStepGradientComponent::CreateDescriptor(),
            EditorSurfaceSlopeGradientComponent::CreateDescriptor(),
            EditorMixedGradientComponent::CreateDescriptor(),
            EditorImageGradientComponent::CreateDescriptor(),
            EditorConstantGradientComponent::CreateDescriptor(),
            EditorThresholdGradientComponent::CreateDescriptor(),
            EditorLevelsGradientComponent::CreateDescriptor(),
            EditorReferenceGradientComponent::CreateDescriptor(),
            EditorInvertGradientComponent::CreateDescriptor(),
            EditorDitherGradientComponent::CreateDescriptor(),
            EditorPosterizeGradientComponent::CreateDescriptor(),
            EditorShapeAreaFalloffGradientComponent::CreateDescriptor(),
            EditorPerlinGradientComponent::CreateDescriptor(),
            EditorRandomGradientComponent::CreateDescriptor(),
            EditorGradientTransformComponent::CreateDescriptor(),
            EditorSurfaceMaskGradientComponent::CreateDescriptor(),
            EditorGradientSurfaceDataComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList GradientSignalEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = GradientSignalModule::GetRequiredSystemComponents();

        requiredComponents.push_back(azrtti_typeid<GradientSignalEditorSystemComponent>());

        return requiredComponents;
    }

    void GradientSignalEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientPreviewer::Reflect(context);
        ImageCreatorUtils::PaintableImageAssetHelper<EditorImageGradientComponent, EditorImageGradientComponentMode>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientSignalEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GradientSignalEditorSystemComponent>("GradientSignalEditorSystemComponent", "Handles registration of the gradient preview data widget handler")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void GradientSignalEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GradientSignalEditorService"));
    }

    void GradientSignalEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GradientSignalEditorService"));
    }

    void GradientSignalEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PropertyManagerService"));
    }

    void GradientSignalEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
    {
    }

    void GradientSignalEditorSystemComponent::OnActionContextModeBindingHook()
    {
        EditorImageGradientComponentMode::BindActionsToModes();
    }

    void GradientSignalEditorSystemComponent::Activate()
    {
        GradientPreviewDataWidgetHandler::Register();
        StreamingImagePropertyHandler::Register();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void GradientSignalEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        GradientPreviewDataWidgetHandler::Unregister();
        // We don't need to unregister the StreamingImagePropertyHandler
        // because its set to auto-delete (default)
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), GradientSignal::GradientSignalEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_GradientSignal_Editor, GradientSignal::GradientSignalEditorModule)
#endif
