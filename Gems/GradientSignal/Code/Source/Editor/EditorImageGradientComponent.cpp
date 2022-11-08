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
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyFilePathCtrl.h>
#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorGradientImageCreatorUtils.h>

namespace GradientSignal
{
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(ImageGradientAutoSaveMode);

    void EditorImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ImageGradientAutoSaveModeReflect(*serializeContext);

            serializeContext->Class<EditorImageGradientComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(3)
                ->Field("Previewer", &EditorImageGradientComponent::m_previewer)
                ->Field("CreationSelectionChoice", &EditorImageGradientComponent::m_creationSelectionChoice)
                ->Field("OutputResolution", &EditorImageGradientComponent::m_outputResolution)
                ->Field("OutputFormat", &EditorImageGradientComponent::m_outputFormat)
                ->Field("Configuration", &EditorImageGradientComponent::m_configuration)
                ->Field("AutoSaveMode", &EditorImageGradientComponent::m_autoSaveMode)
                ->Field("ComponentMode", &EditorImageGradientComponent::m_componentModeDelegate)
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

                    // Either show the "Create" options or the "use image" options based on how this is set.
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorImageGradientComponent::m_creationSelectionChoice,
                        "Source Type", "Select whether to create a new image or use an existing image.")
                        ->EnumAttribute(ImageCreationOrSelection::UseExistingImage, "Use Existing Image")
                        ->EnumAttribute(ImageCreationOrSelection::CreateNewImage, "Create New Image")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorImageGradientComponent::InComponentMode)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::RefreshCreationSelectionChoice)

                    // Auto-save option when editing an image.
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_autoSaveMode, "Save Mode",
                        "When editing an image, this selects whether to manually prompt for the save location, auto-save on every edit, "
                        "or auto-save with incrementing file names on every edit.")
                        ->EnumAttribute(ImageGradientAutoSaveMode::SaveAs, "Save As...")
                        ->EnumAttribute(ImageGradientAutoSaveMode::AutoSave, "Auto Save")
                        ->EnumAttribute(ImageGradientAutoSaveMode::AutoSaveWithIncrementalNames, "Auto Save With Incrementing Names")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetAutoSaveVisibility)
                        // There's no need to ChangeNotify when this property changes, it doesn't affect the behavior of the comopnent,
                        // it's only queried at the point that an edit is completed.

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

        // Make sure our image asset settings are synced in the runtime component's configuration.
        RefreshImageAssetStatus();

        EnableComponentMode();
    }

    void EditorImageGradientComponent::Deactivate()
    {
        DisableComponentMode();

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

    void EditorImageGradientComponent::RefreshComponentModeStatus()
    {
        const bool paintModeVisible = (GetPaintModeVisibility() != AZ::Edit::PropertyVisibility::Hide);

        if (paintModeVisible)
        {
            EnableComponentMode();
        }
        else
        {
            DisableComponentMode();
        }
    }

    void EditorImageGradientComponent::EnableComponentMode()
    {
        if (m_componentModeDelegate.AddedToComponentMode())
        {
            return;
        }

        auto entityComponentIdPair = AZ::EntityComponentIdPair(GetEntityId(), GetId());
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorImageGradientComponent, EditorImageGradientComponentMode>(
            entityComponentIdPair, nullptr);
    }

    void EditorImageGradientComponent::DisableComponentMode()
    {
        if (!m_componentModeDelegate.AddedToComponentMode())
        {
            return;
        }

        m_componentModeDelegate.Disconnect();
    }


    void EditorImageGradientComponent::OnCompositionChanged()
    {
        m_previewer.RefreshPreview();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();

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

    AZ::Crc32 EditorImageGradientComponent::GetAutoSaveVisibility() const
    {
        return (m_creationSelectionChoice == ImageCreationOrSelection::UseExistingImage) ? AZ::Edit::PropertyVisibility::Show
                                                                                         : AZ::Edit::PropertyVisibility::Hide;
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
        // Only show the image painting button while we're using an image, not while we're creating one.
        return ((GetImageOptionsVisibility() != AZ::Edit::PropertyVisibility::Hide)
                && (m_currentImageAssetStatus == AZ::Data::AssetData::AssetStatus::Ready) && !m_currentImageJobsPending)
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
        return AZ::IO::Path(GetImageSourcePath(m_configuration.m_imageAsset.GetId()));
    }

    void EditorImageGradientComponent::SetOutputImagePath(const AZ::IO::Path& outputImagePath)
    {
        m_component.SetImageAssetSourcePath(outputImagePath.String());
    }

    bool EditorImageGradientComponent::InComponentMode() const
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    AZ::IO::Path EditorImageGradientComponent::GetIncrementingAutoSavePath(const AZ::IO::Path& currentPath) const
    {
        // Given a path for a source texture, this will return a new path with an incremented version number on the end.
        // If the input path doesn't have a version number yet, it will get one added.
        // Ex:
        // 'Assets/Gradients/MyGradient_gsi.tif' -> 'Assets/Gradients/MyGradient_gsi.0000.tif'
        // 'Assets/Gradients/MyGradient_gsi.0005.tif' -> 'Assets/Gradients/MyGradient_gsi.0006.tif'

        // We'll use 4 digits as our minimum version number size. We use this to add leading 0s so that alpha-sorting of the
        // numbers still puts them in the right order. For example, we'll get 08, 09, 10 instead of 0, 1, 10, 2. This does
        // mean that the alpha-sorting will look wrong if we hit 5 digits, but it's a readability tradeoff.
        constexpr int NumVersionDigits = 4;

        // Store a copy of the filename in a string so that we can modify it below.
        AZStd::string baseFileName = currentPath.Stem().Native();

        size_t foundDotChar = baseFileName.find_last_of(AZ_FILESYSTEM_EXTENSION_SEPARATOR);
        uint32_t versionNumber = 0;

        // If the base name ends with '.####' (4 or more digits), then we'll treat that as our auto version number.
        // We'll read in the existing number, strip it, and increment it.
        if (foundDotChar <= (baseFileName.length() - NumVersionDigits - 1))
        {
            bool foundVersionNumber = true;
            uint32_t tempVersionNumber = 0;

            for (size_t digitChar = foundDotChar + 1; digitChar < baseFileName.length(); digitChar++)
            {
                // If any character after the . isn't a digit, then this isn't a valid version number, so leave it alone.
                // (Ex: "image_gsi.o3de")
                if (!isdigit(baseFileName.at(digitChar)))
                {
                    foundVersionNumber = false;
                    break;
                }

                // Convert the version number that we've found one character at a time.
                tempVersionNumber = (tempVersionNumber * 10) + (baseFileName.at(digitChar) - '0');
            }

            // If we found a valid version number, auto-increment it by one and strip off the previous one.
            // We'll re-add the new incremented version number back on at the end.
            if (foundVersionNumber)
            {
                versionNumber = tempVersionNumber + 1;
                baseFileName = baseFileName.substr(0, foundDotChar);
            }
        }

        // Create a new string of the form <filename>.####
        // For example, "entity1_gsi.tif" should become "entity1_gsi.0000.tif"
        AZStd::string newFilename = AZStd::string::format(
            AZ_STRING_FORMAT "%c%0*d" AZ_STRING_FORMAT,
            AZ_STRING_ARG(baseFileName),
            AZ_FILESYSTEM_EXTENSION_SEPARATOR,
            NumVersionDigits, versionNumber,
            AZ_STRING_ARG(currentPath.Extension().Native()));

        AZ::IO::Path newPath = currentPath;
        newPath.ReplaceFilename(AZ::IO::Path(newFilename));
        return newPath;
    }


    bool EditorImageGradientComponent::GetSaveLocation(
        AZ::IO::Path& fullPath, AZStd::string& relativePath, ImageGradientAutoSaveMode autoSaveMode)
    {
        QString absoluteSaveFilePath = QString(fullPath.Native().c_str());
        bool promptForSaveName = false;

        switch (autoSaveMode)
        {
        case ImageGradientAutoSaveMode::SaveAs:
            promptForSaveName = true;
            break;
        case ImageGradientAutoSaveMode::AutoSave:
            // If the user has never been prompted for a save location during this Editor run, make sure they're prompted at least once.
            // If they have been prompted, then skip the prompt and just overwrite the existing location.
            promptForSaveName = !m_promptedForSaveLocation;
            break;
        case ImageGradientAutoSaveMode::AutoSaveWithIncrementalNames:
            fullPath = GetIncrementingAutoSavePath(fullPath);
            absoluteSaveFilePath = QString(fullPath.Native().c_str());

            // Only prompt if our auto-generated name matches an existing file.
            promptForSaveName = AZ::IO::SystemFile::Exists(fullPath.Native().c_str());
            break;
        }

        if (promptForSaveName)
        {
            // Prompt the user for the file name and path.
            const QString fileFilter = ImageCreatorUtils::GetSupportedImagesFilter().c_str();
            absoluteSaveFilePath = AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As...", absoluteSaveFilePath, fileFilter);
        }

        // User canceled the save dialog, so exit out.
        if (absoluteSaveFilePath.isEmpty())
        {
            return false;
        }

        // If we prompted for a save name and didn't cancel out with an empty path, track that we've prompted the user so that we don't
        // do it again for autosave.
        m_promptedForSaveLocation = m_promptedForSaveLocation || promptForSaveName;

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

        fullPathIO = AZ::IO::Path(GetImageSourcePath({}));

        // Creating an image should always prompt the user for the save location.
        const auto saveMode = ImageGradientAutoSaveMode::SaveAs;

        if (!GetSaveLocation(fullPathIO, relativePath, saveMode))
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

    AZStd::string EditorImageGradientComponent::GetImageSourcePath(const AZ::Data::AssetId& imageAssetId) const
    {
        if (imageAssetId.IsValid())
        {
            AZStd::string sourcePath;
            bool sourceFileFound = false;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                sourceFileFound,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID,
                imageAssetId.m_guid,
                assetInfo,
                watchFolder);

            if (sourceFileFound)
            {
                bool success = AzFramework::StringFunc::Path::ConstructFull(
                    watchFolder.c_str(), assetInfo.m_relativePath.c_str(), sourcePath, true);

                if (success)
                {
                    return sourcePath;
                }
            }
        }

        // Invalid image asset or failed path creation, try creating a new name.
        return AZStd::string::format(AZ_STRING_FORMAT "_gsi.tif", AZ_STRING_ARG(GetEntity()->GetName()));
    }

    bool EditorImageGradientComponent::SaveImage()
    {
        AZ::IO::Path fullPathIO;
        AZStd::string relativePath;

        // Turn it into an absolute path to give to the "Save file" dialog.
        fullPathIO = AZ::IO::Path(GetImageSourcePath(m_configuration.m_imageAsset.GetId()));

        if (!GetSaveLocation(fullPathIO, relativePath, m_autoSaveMode))
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
            AZ::Data::AssetLoadBehavior::PreLoad);

        // Set the asset hint to the source path so that we can display something reasonably correct in the component while waiting
        // for the product asset to get created.
        createdAsset.SetHint(relativePath);

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
