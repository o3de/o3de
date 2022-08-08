/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyFilePathCtrl.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <GradientSignal/Editor/EditorGradientPainterComponent.h>

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING

namespace GradientSignal
{
    AZStd::string GradientPainterConfig::GetSupportedImagesFilter()
    {
        // Build filter for supported streaming image formats that will be used on the
        // native file dialog when creating/picking an output file for the painted image.
        // ImageProcessingAtom::s_SupportedImageExtensions actually has more formats
        // that will produce streaming image assets, but not all of them support
        // all of the bit depths we care about (8/16/32), so we've reduced the list
        // to the image formats that do.
        return "Images (*.png *.tif *.tiff *.tga *.exr)";
    }

    AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> GradientPainterConfig::SupportedOutputFormatOptions()
    {
        AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> options;

        options.push_back(AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R8, "R8 (8-bit)"));
        options.push_back(AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R16, "R16 (16-bit)"));
        options.push_back(AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R32, "R32 (32-bit)"));

        return options;
    }

    void GradientPainterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientPainterConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("InputBounds", &GradientPainterConfig::m_inputBounds)
                ->Field("OutputResolution", &GradientPainterConfig::m_outputResolution)
                ->Field("OutputFormat", &GradientPainterConfig::m_outputFormat)
                ->Field("OutputImagePath", &GradientPainterConfig::m_outputImagePath)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientPainterConfig>("Gradient Painter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientPainterConfig::m_inputBounds, "Input Bounds",
                        "Input bounds for where to sample the data.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientPainterConfig::m_outputResolution, "Resolution",
                        "Output resolution of the baked image.")
                    ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                    ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &GradientPainterConfig::m_outputFormat, "Output Format",
                        "Output format of the baked image.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &GradientPainterConfig::SupportedOutputFormatOptions)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientPainterConfig::m_outputImagePath, "Output Path",
                        "Output path to bake the image to.")
                    ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, GradientPainterConfig::GetSupportedImagesFilter())
                    ->Attribute(AZ::Edit::Attributes::DefaultAsset, "baked_output_gsi")
                    ;
            }
        }
    }

    void EditorGradientPainterComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientPainterConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGradientPainterComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Configuration", &EditorGradientPainterComponent::m_configuration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorGradientPainterComponent>(
                        EditorGradientPainterComponent::s_componentName, EditorGradientPainterComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorGradientPainterComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->UIElement(AZ_CRC_CE("GradientPreviewer"), "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &EditorGradientPainterComponent::GetGradientEntityId)
                    ->EndGroup()

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGradientPainterComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientPainterComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "PaintImage", "Paint into an image asset")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Paint")
                    //->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientPainterComponent::BakeImage)
                    ;
            }
        }
    }

    void EditorGradientPainterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientPainterService"));
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void EditorGradientPainterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientPainterService"));
        services.push_back(AZ_CRC_CE("GradientBakerService"));
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void EditorGradientPainterComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        m_gradientEntityId = GetEntityId();

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        GradientPreviewContextRequestBus::Handler::BusConnect(GetEntityId());

        // Setup the dependency monitor and listen for gradient requests
        SetupDependencyMonitor();

        GradientImageCreatorRequestBus::Handler::BusConnect(GetEntityId());

        UpdatePreviewSettings();
    }

    void EditorGradientPainterComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        GradientImageCreatorRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        // If the preview shouldn't be active, use an invalid entityId
        m_gradientEntityId = AZ::EntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientPreviewContextRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorGradientPainterComponent::OnCompositionChanged()
    {
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGradientPainterComponent::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration
        // change)
        GradientSignal::GradientPreviewRequestBus::Event(m_gradientEntityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

    AzToolsFramework::EntityIdList EditorGradientPainterComponent::CancelPreviewRendering() const
    {
        AzToolsFramework::EntityIdList entityIds;
        AZ::EBusAggregateResults<AZ::EntityId> canceledPreviews;
        GradientSignal::GradientPreviewRequestBus::BroadcastResult(
            canceledPreviews, &GradientSignal::GradientPreviewRequestBus::Events::CancelRefresh);

        // Gather up the EntityIds for any previews that were in progress when we canceled them
        for (auto entityId : canceledPreviews.values)
        {
            if (entityId.IsValid())
            {
                entityIds.push_back(entityId);
            }
        }

        return entityIds;
    }

    void EditorGradientPainterComponent::SetupDependencyMonitor()
    {
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());

        // Connect to GradientRequestBus after the gradient sampler and dependency monitor is configured
        // before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    AZ::EntityId EditorGradientPainterComponent::GetPreviewEntity() const
    {
        // Our preview entity will always be ourself since we want to preview
        // exactly what's going to be in the baked image.
        return GetEntityId();
    }

    AZ::Aabb EditorGradientPainterComponent::GetPreviewBounds() const
    {
        AZ::Aabb bounds = AZ::Aabb::CreateNull();

        if (m_configuration.m_inputBounds.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                bounds, m_configuration.m_inputBounds, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        return bounds;
    }

    AZ::EntityId EditorGradientPainterComponent::GetGradientEntityId() const
    {
        return m_gradientEntityId;
    }

    float EditorGradientPainterComponent::GetValue([[maybe_unused]] const GradientSampleParams& sampleParams) const
    {
        return 0.0f;
    }

    void EditorGradientPainterComponent::GetValues(
        [[maybe_unused]] AZStd::span<const AZ::Vector3> positions, [[maybe_unused]] AZStd::span<float> outValues) const
    {
    }

    bool EditorGradientPainterComponent::IsEntityInHierarchy([[maybe_unused]] const AZ::EntityId& entityId) const
    {
        return false;
    }

    AZ::EntityId EditorGradientPainterComponent::GetInputBounds() const
    {
        return m_configuration.m_inputBounds;
    }

    void EditorGradientPainterComponent::SetInputBounds(const AZ::EntityId& inputBounds)
    {
        m_configuration.m_inputBounds = inputBounds;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector2 EditorGradientPainterComponent::GetOutputResolution() const
    {
        return m_configuration.m_outputResolution;
    }

    void EditorGradientPainterComponent::SetOutputResolution(const AZ::Vector2& resolution)
    {
        m_configuration.m_outputResolution = resolution;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    OutputFormat EditorGradientPainterComponent::GetOutputFormat() const
    {
        return m_configuration.m_outputFormat;
    }

    void EditorGradientPainterComponent::SetOutputFormat(OutputFormat outputFormat)
    {
        m_configuration.m_outputFormat = outputFormat;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::IO::Path EditorGradientPainterComponent::GetOutputImagePath() const
    {
        return m_configuration.m_outputImagePath;
    }

    void EditorGradientPainterComponent::SetOutputImagePath(const AZ::IO::Path& outputImagePath)
    {
        m_configuration.m_outputImagePath = outputImagePath;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void EditorGradientPainterComponent::OnSelected()
    {
        UpdatePreviewSettings();
    }

    void EditorGradientPainterComponent::OnDeselected()
    {
        UpdatePreviewSettings();
    }

    void EditorGradientPainterComponent::OnConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = CancelPreviewRendering();

        // Re-setup the dependency monitor when the configuration changes because the gradient sampler
        // could've changed
        SetupDependencyMonitor();

        // Refresh any of the previews that we canceled that were still in progress so they can be completed
        for (auto entityId : entityIds)
        {
            GradientSignal::GradientPreviewRequestBus::Event(entityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
        }

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call UpdatePreviewSettings explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
} // namespace GradientSignal
