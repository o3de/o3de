/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorImageGradientComponent.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyFilePathCtrl.h>
#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorGradientImageCreatorUtils.h>

namespace GradientSignal
{
    void EditorImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorImageGradientComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("Previewer", &EditorImageGradientComponent::m_previewer)
                ->Field("CreationSelectionChoice", &EditorImageGradientComponent::m_creationSelectionChoice)
                ->Field("OutputResolution", &EditorImageGradientComponent::m_outputResolution)
                ->Field("OutputFormat", &EditorImageGradientComponent::m_outputFormat)
                ->Field("OutputImagePath", &EditorImageGradientComponent::m_outputImagePath)
                ->Field("Configuration", &EditorImageGradientComponent::m_configuration)
                ->Field("ComponentMode", &EditorImageGradientComponent::m_componentModeDelegate)
                // For now, we need to serialize the paint brush settings so that they can be displayed via the EditContext
                // while in the component mode. This can eventually be removed if we add support for paint brush settting
                // modifications in-viewport.
                ->Field("PaintBrush", &EditorImageGradientComponent::m_paintBrush)
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
                        ->EnumAttribute(ChannelToUse::Red, "Red (default)")
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
                        ->EnumAttribute(CustomScaleType::None, "None (default)")
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

                    // Either show the "Create" options or the "use image" options based on how this is set.
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorImageGradientComponent::m_creationSelectionChoice,
                        "Source Type", "Select whether to create a new image or use an existing image.")
                        ->EnumAttribute(ImageCreationOrSelection::UseExistingImage, "Use Existing Image")
                        ->EnumAttribute(ImageCreationOrSelection::CreateNewImage, "Create New Image")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorImageGradientComponent::InComponentMode)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::RefreshCreationSelectionChoice)

                    // Controls for creating a new image
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_outputResolution,
                        "Resolution", "Output resolution of the saved image.")
                        ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 8192.0f)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageCreationVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorImageGradientComponent::m_outputFormat,
                        "Output Format", "Output format of the saved image.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &ImageCreatorUtils::SupportedOutputFormatOptions)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageCreationVisibility)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Create Image")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::CreateImage)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Create")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageCreationVisibility)

                    // Configuration for the Image Gradient control itself
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorImageGradientComponent::GetImageOptionsReadOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::ConfigurationChanged)

                    // Paint controls for editing the image
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_componentModeDelegate,
                        "Paint Image", "Paint into an image asset")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Paint")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetPaintModeVisibility)

                    // For now, we need to display the paint brush settings here to make them editable while we're in component mode.
                    // If we ever provide in-viewport editing for the properties, this can be removed.
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_paintBrush,
                        "Paint Brush", "Paint Brush Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::InComponentMode)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::PaintBrushSettingsChanged)
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
        EditorImageGradientRequestBus::Handler::BusConnect(GetEntityId());

        m_previewer.Activate(GetEntityId());
        GradientImageCreatorRequestBus::Handler::BusConnect(GetEntityId());

        // Make sure our image asset and creation/selection visibility settings are synced in the runtime component's configuration.
        RefreshImageAssetStatus();
        RefreshCreationSelectionChoice();

        auto entityComponentIdPair = AZ::EntityComponentIdPair(GetEntityId(), GetId());
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorImageGradientComponent, EditorImageGradientComponentMode>(
            entityComponentIdPair, nullptr);

        // The PaintBrushNotificationBus is needed so that we can keep our viewable / editable paint brush settings in sync
        // with the ones on the brush manipulator itself.
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

    }

    void EditorImageGradientComponent::Deactivate()
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();

        m_componentModeDelegate.Disconnect();

        m_currentImageAssetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded;
        m_currentImageJobsPending = false;

        GradientImageCreatorRequestBus::Handler::BusDisconnect();
        m_previewer.Deactivate();

        EditorImageGradientRequestBus::Handler::BusDisconnect();
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

    bool EditorImageGradientComponent::ImageHasPendingJobs(const AZ::Data::AssetId& assetId)
    {
        // If it's not a valid asset, it doesn't have any pending jobs.
        if (!assetId.IsValid())
        {
            return false;
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
        AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
            jobOutcome,
            &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID,
            assetId,
            false,
            false);

        if (jobOutcome.IsSuccess())
        {
            // If there are any jobs that are pending, we'll set our current status based on that instead of
            // on the asset loading status.
            auto jobInfo = jobOutcome.GetValue();
            for (auto& job : jobInfo)
            {
                switch (job.m_status)
                {
                case AzToolsFramework::AssetSystem::JobStatus::Queued:
                case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                    return true;
                }
            }
        }

        return false;
    }


    bool EditorImageGradientComponent::RefreshImageAssetStatus()
    {
        bool jobsPending = ImageHasPendingJobs(m_configuration.m_imageAsset.GetId());
        bool statusChanged =
            (m_currentImageAssetStatus != m_configuration.m_imageAsset.GetStatus()) ||
            (m_currentImageJobsPending != jobsPending);

        m_currentImageAssetStatus = m_configuration.m_imageAsset.GetStatus();
        m_currentImageJobsPending = jobsPending;

        // If there's a valid image selected, check to see if it has any current AP jobs running.
        if (jobsPending)
        {
            m_configuration.SetImageAssetPropertyName("Image Asset (processing)");
            return statusChanged;
        }

        // No pending asset processing jobs, so just use the current load status of the asset.
        switch (m_currentImageAssetStatus)
        {
        case AZ::Data::AssetData::AssetStatus::NotLoaded:
        case AZ::Data::AssetData::AssetStatus::Error:
            m_configuration.SetImageAssetPropertyName("Image Asset (not loaded)");
            break;
        case AZ::Data::AssetData::AssetStatus::Queued:
        case AZ::Data::AssetData::AssetStatus::StreamReady:
        case AZ::Data::AssetData::AssetStatus::Loading:
        case AZ::Data::AssetData::AssetStatus::LoadedPreReady:
            m_configuration.SetImageAssetPropertyName("Image Asset (loading)");
            break;
        case AZ::Data::AssetData::AssetStatus::ReadyPreNotify:
        case AZ::Data::AssetData::AssetStatus::Ready:
        default:
            m_configuration.SetImageAssetPropertyName("Image Asset");
            break;
        }

        return statusChanged;
    }

    void EditorImageGradientComponent::OnCompositionChanged()
    {
        m_previewer.RefreshPreview();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();

        // Make sure the creation/selection visibility flags have been refreshed correctly.
        RefreshCreationSelectionChoice();

        if (RefreshImageAssetStatus() && GetImageOptionsVisibility())
        {
            // If the asset status changed and the image asset property is visible, refresh the entire tree so
            // that the label change is picked up.
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
        }
        else
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
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

    AZ::u32 EditorImageGradientComponent::RefreshCreationSelectionChoice()
    {
        // We need to refresh the entire tree because this selection changes the visibility of other properties.
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorImageGradientComponent::GetImageOptionsVisibility() const
    {
        return (m_creationSelectionChoice == ImageCreationOrSelection::UseExistingImage)
            ? AZ::Edit::PropertyVisibility::ShowChildrenOnly
            : AZ::Edit::PropertyVisibility::Hide;
    }

    bool EditorImageGradientComponent::GetImageCreationVisibility() const
    {
        // Only show the image creation options if we don't have an existing image asset selected.
        return (m_creationSelectionChoice == ImageCreationOrSelection::CreateNewImage);
    }

    AZ::Crc32 EditorImageGradientComponent::GetPaintModeVisibility() const
    {
        // temporary setting to disable this feature unless the registry key is set.
        if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get())
        {
            constexpr AZStd::string_view ImageGradientPaintFeature = "/O3DE/Preferences/ImageGradient/PaintFeature";
            bool hasPaintFeature = false;
            settingsRegistry->Get(hasPaintFeature, ImageGradientPaintFeature);
            if (!hasPaintFeature)
            {
                return AZ::Edit::PropertyVisibility::Hide;
            }
        }

        // Only show the image painting button while we're using an image, not while we're creating one.
        return ((GetImageOptionsVisibility() != AZ::Edit::PropertyVisibility::Hide)
                && m_configuration.m_imageAsset.IsReady()
                && !ImageHasPendingJobs(m_configuration.m_imageAsset.GetId()))
            ? AZ::Edit::PropertyVisibility::ShowChildrenOnly
            : AZ::Edit::PropertyVisibility::Hide;

    }

    bool EditorImageGradientComponent::GetImageOptionsReadOnly() const
    {
        return ((GetImageOptionsVisibility() == AZ::Edit::PropertyVisibility::Hide) || m_component.ModificationBufferIsActive());
    }

    AZ::Vector2 EditorImageGradientComponent::GetOutputResolution() const
    {
        return m_outputResolution;
    }

    void EditorImageGradientComponent::SetOutputResolution(const AZ::Vector2& resolution)
    {
        m_outputResolution = resolution;
    }

    OutputFormat EditorImageGradientComponent::GetOutputFormat() const
    {
        return m_outputFormat;
    }

    void EditorImageGradientComponent::SetOutputFormat(OutputFormat outputFormat)
    {
        m_outputFormat = outputFormat;
    }

    AZ::IO::Path EditorImageGradientComponent::GetOutputImagePath() const
    {
        return m_outputImagePath;
    }

    void EditorImageGradientComponent::SetOutputImagePath(const AZ::IO::Path& outputImagePath)
    {
        m_outputImagePath = outputImagePath;
    }

    bool EditorImageGradientComponent::InComponentMode() const
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    void EditorImageGradientComponent::OnIntensityChanged(float intensity)
    {
        // Any time the paint brush manipulator changes a brush setting, keep our displayed settings in sync with it.
        m_paintBrush.m_intensity = intensity;
    }

    void EditorImageGradientComponent::OnOpacityChanged(float opacity)
    {
        // Any time the paint brush manipulator changes a brush setting, keep our displayed settings in sync with it.
        m_paintBrush.m_opacity = opacity;
    }

    void EditorImageGradientComponent::OnRadiusChanged(float radius)
    {
        // Any time the paint brush manipulator changes a brush setting, keep our displayed settings in sync with it.
        m_paintBrush.m_radius = radius;
    }

    AZ::u32 EditorImageGradientComponent::PaintBrushSettingsChanged()
    {
        // Any time a paint brush setting is edited via the component UX, broadcast it out so that the paint brush manipulator
        // stays in sync with it.

        auto entityComponentIdPair = AZ::EntityComponentIdPair(GetEntityId(), GetId());

        AzToolsFramework::PaintBrushRequestBus::Event(
            entityComponentIdPair, &AzToolsFramework::PaintBrushRequestBus::Events::SetRadius, m_paintBrush.m_radius);

        AzToolsFramework::PaintBrushRequestBus::Event(
            entityComponentIdPair, &AzToolsFramework::PaintBrushRequestBus::Events::SetIntensity, m_paintBrush.m_intensity);

        AzToolsFramework::PaintBrushRequestBus::Event(
            entityComponentIdPair, &AzToolsFramework::PaintBrushRequestBus::Events::SetOpacity, m_paintBrush.m_opacity);

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }


    bool EditorImageGradientComponent::GetSaveLocation(AZ::IO::Path& fullPath, AZStd::string& relativePath)
    {
        // Create a default path to save our image to if we haven't previously set the image path to anything yet.
        if (m_outputImagePath.empty())
        {
            m_outputImagePath = AZStd::string::format("%.*s_gsi.tif", AZ_STRING_ARG(GetEntity()->GetName()));
        }

        // Turn it into an absolute path to give to the "Save file" dialog.
        fullPath = AzToolsFramework::GetAbsolutePathFromRelativePath(m_outputImagePath);

        // Prompt the user for the file name and path.
        const QString fileFilter = ImageCreatorUtils::GetSupportedImagesFilter().c_str();
        const QString absoluteSaveFilePath =
            AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As...", QString(fullPath.Native().c_str()), fileFilter);

        // User canceled the save dialog, so exit out.
        if (absoluteSaveFilePath.isEmpty())
        {
            return false;
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCStr = absoluteSaveFilePathUtf8.constData();
        fullPath.Assign(absoluteSaveFilePathCStr);
        fullPath = fullPath.LexicallyNormal();

        // Turn the absolute path selected in the "Save file" dialog back into a relative path.
        // It's a way to verify that our path exists within the project asset search hierarchy,
        // and it will get used as an asset hint until the asset is fully processed.
        AZStd::string rootFilePath;
        bool relativePathFound;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            relativePathFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
            absoluteSaveFilePathCStr,
            relativePath,
            rootFilePath);

        if (!relativePathFound)
        {
            AZ_Error(
                "EditorImageGradientComponent",
                false,
                "Selected path exists outside of the asset processing directories: %s",
                absoluteSaveFilePathCStr);
            return false;
        }

        return true;
    }

    void EditorImageGradientComponent::StartImageModification()
    {
        // While we're editing, we need to set all the configuration properties to read-only and refresh them.
        // Otherwise, the property changes could conflict with the current painted modifications.
        m_configuration.m_imageModificationActive = true;
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponent::EndImageModification()
    {
        // We're done editing, so set all the configuration properties back to writeable and refresh them.
        m_configuration.m_imageModificationActive = false;
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponent::CreateImage()
    {
        AZ::IO::Path fullPathIO;
        AZStd::string relativePath;
        if (!GetSaveLocation(fullPathIO, relativePath))
        {
            return;
        }

        // Get the actual resolution of our image.
        const int imageResolutionX = aznumeric_cast<int>(m_outputResolution.GetX());
        const int imageResolutionY = aznumeric_cast<int>(m_outputResolution.GetY());

        // The TGA and EXR formats aren't recognized with only single channel data,
        // so need to use RGBA format for them
        int channels = 1;
        if (fullPathIO.Extension() == ".tga" || fullPathIO.Extension() == ".exr")
        {
            channels = 4;
        }

        auto pixelBuffer = ImageCreatorUtils::CreateDefaultImageBuffer(imageResolutionX, imageResolutionY, channels, m_outputFormat);

        SaveImageInternal(
            fullPathIO, relativePath,
            imageResolutionX, imageResolutionY, channels, m_outputFormat, pixelBuffer);
    }

    bool EditorImageGradientComponent::SaveImage()
    {
        AZ::IO::Path fullPathIO;
        AZStd::string relativePath;
        if (!GetSaveLocation(fullPathIO, relativePath))
        {
            return false;
        }

        // The TGA and EXR formats aren't recognized with only single channel data,
        // so need to use RGBA format for them
        int channels = 1;
        if (fullPathIO.Extension() == ".tga" || fullPathIO.Extension() == ".exr")
        {
            AZ_Assert(false, "4-channel TGA / EXR isn't currently supported in this method.");
            return false;
        }

        // Get the resolution of our modified image.
        const int imageResolutionX = aznumeric_cast<int>(m_component.GetImageWidth());
        const int imageResolutionY = aznumeric_cast<int>(m_component.GetImageHeight());

        // Get the image modification buffer
        auto pixelBuffer = m_component.GetImageModificationBuffer();

        return SaveImageInternal(
            fullPathIO, relativePath, imageResolutionX, imageResolutionY, channels, OutputFormat::R32,
            AZStd::span<const uint8_t>(reinterpret_cast<uint8_t*>(pixelBuffer->data()), pixelBuffer->size() * sizeof(float)));
    }

    bool EditorImageGradientComponent::SaveImageInternal(
        AZ::IO::Path& fullPath, AZStd::string& relativePath,
        int imageResolutionX, int imageResolutionY, int channels, OutputFormat format, AZStd::span<const uint8_t> pixelBuffer)
    {
        // Try to write out the image
        constexpr bool showProgressDialog = true;
        if (!ImageCreatorUtils::WriteImage(
                fullPath.c_str(),
                imageResolutionX, imageResolutionY, channels, format, pixelBuffer,
                showProgressDialog))
        {
            AZ_Error("EditorImageGradientComponent", false, "Failed to save image: %s", fullPath.c_str());
            return false;
        }

        // Try to find the source information for the new image in the Asset System.
        bool sourceInfoFound = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            sourceInfoFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            fullPath.c_str(),
            sourceInfo,
            watchFolder);

        // If this triggers, the flow for handling newly-created images needs to be examined further.
        // It's possible that we need to wait for some sort of asset processing event before we can get
        // the source asset ID.
        AZ_Warning("EditorImageGradientComponent", sourceInfoFound, "Could not find source info for %s", fullPath.c_str());

        // Using the source asset ID, get or create an asset referencing using the expected product asset ID.
        // If we're overwriting an existing source asset, this will already exist, but if we're creating a new file,
        // the product asset won't exist yet.
        auto createdAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(
            AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, AZ::RPI::StreamingImageAsset::GetImageAssetSubId()),
            azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
            AZ::Data::AssetLoadBehavior::QueueLoad);

        // Set the asset hint to the source path so that we can display something reasonably correct in the component while waiting
        // for the product asset to get created.
        createdAsset.SetHint(relativePath);

        // Set our output image path to whatever was selected so that we default to the same file name and path next time.
        m_outputImagePath = fullPath.c_str();

        // Set the active image to the created one.
        m_component.SetImageAsset(createdAsset);

        // Switch our creation/selection choice to using an existing image.
        m_creationSelectionChoice = ImageCreationOrSelection::UseExistingImage;

        // Resync the configurations and refresh the display to hide the "Create" button
        // We need to use "Refresh_EntireTree" because "Refresh_AttributesAndValues" isn't enough to refresh the visibility
        // settings.
        OnCompositionChanged();
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);

        return true;
    }


}
