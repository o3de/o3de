/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorImageGradientComponent.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorUtils.h>

namespace GradientSignal
{
    // Implements EditorImageGradientComponent RTTI functions
    AZ_RTTI_NO_TYPE_INFO_IMPL(EditorImageGradientComponent, AzToolsFramework::Components::EditorComponentBase);

    void EditorImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorImageGradientComponentMode::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorImageGradientComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(4)
                ->Field("Previewer", &EditorImageGradientComponent::m_previewer)
                ->Field("Configuration", &EditorImageGradientComponent::m_configuration)
                ->Field("PaintableImageAssetHelper", &EditorImageGradientComponent::m_paintableImageAssetHelper)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ImageGradientConfig>("Image Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_imageAsset,
                        "Image Asset", "Image asset whose values will be mapped as gradient output.")
                        ->Attribute(AZ::Edit::Attributes::Handler, AZ_CRC_CE("GradientSignalStreamingImageAsset"))
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ImageGradientConfig::GetImageAssetPropertyName)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::IsImageAssetReadOnly)
                        // Refresh the attributes because some fields will switch between read-only and writeable when
                        // the image asset is changed.
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_samplingType,
                        "Sampling Type", "Sampling type to use for the image data.")
                        ->EnumAttribute(SamplingType::Point, "Point")
                        ->EnumAttribute(SamplingType::Bilinear, "Bilinear")
                        ->EnumAttribute(SamplingType::Bicubic, "Bicubic")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Vector2, &ImageGradientConfig::m_tiling,
                        "Tiling", "Number of times to tile horizontally/vertically.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1024.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_channelToUse,
                        "Channel To Use", "The channel to use from the image.")
                        ->EnumAttribute(ChannelToUse::Red, "Red")
                        ->EnumAttribute(ChannelToUse::Green, "Green")
                        ->EnumAttribute(ChannelToUse::Blue, "Blue")
                        ->EnumAttribute(ChannelToUse::Alpha, "Alpha")
                        ->EnumAttribute(ChannelToUse::Terrarium, "Terrarium")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &ImageGradientConfig::m_mipIndex, "Mip Index", "Mip index to sample from.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::RHI::Limits::Image::MipCountMax)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_customScaleType,
                        "Custom Scale", "Choose a type of scaling to be applied to the image data.")
                        ->EnumAttribute(CustomScaleType::None, "None")
                        ->EnumAttribute(CustomScaleType::Auto, "Auto")
                        ->EnumAttribute(CustomScaleType::Manual, "Manual")
                        // Refresh the entire tree on scaling changes, because it will show/hide the scale ranges for Manual scaling.
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_scaleRangeMin,
                        "Range Minimum", "The minimum range each value from the image data is scaled against.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetManualScaleVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_scaleRangeMax,
                        "Range Maximum", "The maximum range each value from the image data is scaled against.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetManualScaleVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)
                    ;


                editContext
                    ->Class<EditorImageGradientComponent>(
                        EditorImageGradientComponent::s_componentName, EditorImageGradientComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, EditorImageGradientComponent::s_icon)
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, EditorImageGradientComponent::s_viewportIcon)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, EditorImageGradientComponent::s_helpUrl)
                        ->Attribute(AZ::Edit::Attributes::Category, EditorImageGradientComponent::s_categoryName)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_previewer, "Previewer", "Gradient Previewer")

                    // Configuration for the Image Gradient control itself
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorImageGradientComponent::GetImageOptionsReadOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::ConfigurationChanged)

                    // Paint controls for editing the image
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_paintableImageAssetHelper,
                        "Paint Image", "Paint into an image asset")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Paint")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    // The following methods pass through to the runtime component so that the Editor component shares the same requirements.

    void EditorImageGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        ImageGradientComponent::GetRequiredServices(services);
    }

    void EditorImageGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        ImageGradientComponent::GetIncompatibleServices(services);
    }

    void EditorImageGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        ImageGradientComponent::GetProvidedServices(services);
    }

    void EditorImageGradientComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        ImageGradientComponent::GetDependentServices(services);
    }


    void EditorImageGradientComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // When building the game entity, use the copy of the runtime configuration on the Editor component to create
        // a new runtime component that's configured correctly.
        gameEntity->AddComponent(aznew ImageGradientComponent(m_configuration));
    }

    void EditorImageGradientComponent::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();

        // Initialize the copy of the runtime component.
        m_runtimeComponentActive = false;
        m_component.ReadInConfig(&m_configuration);
        m_component.Init();
    }

    void EditorImageGradientComponent::Activate()
    {
        // This block of code is aligned with EditorWrappedComponentBase
        {
            AzToolsFramework::Components::EditorComponentBase::Activate();

            // Use the visibility bus to control whether or not the runtime gradient is active and processing in the Editor.
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                m_visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

            // Synchronize the runtime component with the Editor component.
            m_component.ReadInConfig(&m_configuration);
            m_component.SetEntity(GetEntity());

            if (m_visible)
            {
                m_component.Activate();
                m_runtimeComponentActive = true;
            }
        }

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::PaintBrushNotificationBus::Handler::BusConnect({ GetEntityId(), GetId() });

        m_previewer.Activate(GetEntityId());

        // Initialize the paintable image asset helper.
        m_paintableImageAssetHelper.Activate(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            OutputFormat::R32,
            "Image Asset",
            [this]()
            {
                // Get a default image filename and path that either uses the source asset filename (if the source asset exists)
                // or creates a new name by taking the entity name and adding "_gsi.tif".
                return AZ::IO::Path(ImageCreatorUtils::GetDefaultImageSourcePath(
                    m_configuration.m_imageAsset.GetId(), GetEntity()->GetName() + AZStd::string("_gsi.tif")));
            },
            [this](AZ::Data::Asset<AZ::Data::AssetData> createdAsset)
            {
                // Set the active image to the created one.
                m_component.SetImageAsset(createdAsset);

                OnCompositionChanged();
            });

        AZStd::string assetLabel =
            m_paintableImageAssetHelper.Refresh(m_configuration.m_imageAsset);

        m_configuration.SetImageAssetPropertyName(assetLabel);
    }

    void EditorImageGradientComponent::Deactivate()
    {
        m_paintableImageAssetHelper.Deactivate();

        m_previewer.Deactivate();

        AzFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // This block of code is aligned with EditorWrappedComponentBase
        {
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();

            m_runtimeComponentActive = false;
            m_component.Deactivate();
            // remove the entity association, in case the parent component is being removed, otherwise the component will be reactivated
            m_component.SetEntity(nullptr);
        }
    }

    void EditorImageGradientComponent::OnEntityVisibilityChanged(bool visibility)
    {
        if (m_visible != visibility)
        {
            m_visible = visibility;
            ConfigurationChanged();
        }
    }

    void EditorImageGradientComponent::OnCompositionRegionChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        // If only a region of the image gradient changed, then we only need to refresh the preview.
        m_previewer.RefreshPreview();
    }

    void EditorImageGradientComponent::OnCompositionChanged()
    {
        // Keep track of what our previous label was, so that we know to refresh if it changes.
        // We need to grab this before calling WriteOutConfig() because that will overwrite the label with
        // the empty label that's stored with the runtime component.
        auto previousImageAssetPropertyName = m_configuration.GetImageAssetPropertyName();

        m_previewer.RefreshPreview();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();

        AZStd::string assetLabel = m_paintableImageAssetHelper.Refresh(m_configuration.m_imageAsset);

        m_configuration.SetImageAssetPropertyName(assetLabel);

        bool imageNameChanged = m_configuration.GetImageAssetPropertyName() != previousImageAssetPropertyName;

         InvalidatePropertyDisplay(imageNameChanged ? AzToolsFramework::Refresh_EntireTree : AzToolsFramework::Refresh_AttributesAndValues);

    }

    AZ::u32 EditorImageGradientComponent::ConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = m_previewer.CancelPreviewRendering();

        // This block of code aligns with EditorWrappedComponentBase
        {
            if (m_runtimeComponentActive)
            {
                m_runtimeComponentActive = false;
                m_component.Deactivate();
            }

            m_component.ReadInConfig(&m_configuration);

            if (m_visible && !m_runtimeComponentActive)
            {
                m_component.Activate();
                m_runtimeComponentActive = true;
            }
        }

        // Refresh any of the previews that we canceled that were still in progress so they can be completed
        m_previewer.RefreshPreviews(entityIds);

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    bool EditorImageGradientComponent::GetImageOptionsReadOnly() const
    {
        // you cannot change any configuration option if the image is modified in memory but not saved.
        // note that this will apply to all child options, too.
        return (m_component.ModificationBufferIsActive());
    }

    void EditorImageGradientComponent::OnPaintModeBegin()
    {
        m_configuration.m_numImageModificationsActive++;

        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnPaintModeBegin);

        // While we're editing, we need to set all the configuration properties to read-only and refresh them.
        // Otherwise, the property changes could conflict with the current painted modifications.
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);

    }

    void EditorImageGradientComponent::OnPaintModeEnd()
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnPaintModeEnd);

        m_configuration.m_numImageModificationsActive--;

        // It's possible that we're leaving component mode as the result of an "undo" action.
        // If that's the case, don't prompt the user to save the changes.
        if (!AzToolsFramework::UndoRedoOperationInProgress() && m_component.ImageIsModified())
        {
            SavePaintedData(); // this function may execute a modal call.  Delay property invalidation until afterwards.
        }
        else
        {
             m_component.ClearImageModificationBuffer(); // unless we do this, all properties stay read-only
        }

        // We're done editing, so set all the configuration properties back to writeable and refresh them.
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponent::OnBrushStrokeBegin(const AZ::Color& color)
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() },
            &AzFramework::PaintBrushNotificationBus::Events::OnBrushStrokeBegin,
            color);
    }

    void EditorImageGradientComponent::OnBrushStrokeEnd()
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnBrushStrokeEnd);
    }

    void EditorImageGradientComponent::OnPaint(
        const AZ::Color& color, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnPaint,
            color, dirtyArea, valueLookupFn, blendFn);
    }

    void EditorImageGradientComponent::OnSmooth(
        const AZ::Color& color,
        const AZ::Aabb& dirtyArea,
        ValueLookupFn& valueLookupFn,
        AZStd::span<const AZ::Vector3> valuePointOffsets,
        SmoothFn& smoothFn)
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnSmooth,
            color, dirtyArea, valueLookupFn, valuePointOffsets, smoothFn);
    }

    AZ::Color EditorImageGradientComponent::OnGetColor(const AZ::Vector3& brushCenter) const
    {
        AZ::Color result;

        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::EventResult(result,
            { m_component.GetEntityId(), m_component.GetId() },
            &AzFramework::PaintBrushNotificationBus::Events::OnGetColor,
            brushCenter);

        return result;
    }

    bool EditorImageGradientComponent::SavePaintedData()
    {
        // Get the resolution of our modified image.
        const int imageResolutionX = aznumeric_cast<int>(m_component.GetImageWidth());
        const int imageResolutionY = aznumeric_cast<int>(m_component.GetImageHeight());

        // Get the image modification buffer
        auto pixelBuffer = m_component.GetImageModificationBuffer();

        const OutputFormat format = OutputFormat::R32;

        auto createdAsset = m_paintableImageAssetHelper.SaveImage(
            imageResolutionX, imageResolutionY, format,
            AZStd::span<const uint8_t>(reinterpret_cast<uint8_t*>(pixelBuffer->data()), pixelBuffer->size() * sizeof(float)));

        if (createdAsset)
        {
            // Set the active image to the created one.
            m_component.SetImageAsset(createdAsset.value());
            m_component.ClearImageModificationBuffer(); // we no longer have modified changes that are unsaved.

            OnCompositionChanged();
        }

        return createdAsset.has_value();
    }


}
