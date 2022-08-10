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
#include <Editor/EditorGradientPainterComponent.h>
#include <Editor/EditorGradientPainterComponentMode.h>

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING

namespace GradientSignal
{
    void GradientPainterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientPainterConfig, AZ::ComponentConfig>()
                ->Version(2)
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
                        AZ::Edit::UIHandlers::Default, &GradientPainterConfig::m_outputResolution, "Resolution",
                        "Output resolution of the saved image.")
                    ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                    ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 8192.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &GradientPainterConfig::m_outputFormat, "Output Format",
                        "Output format of the saved image.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &GradientImageCreatorRequests::SupportedOutputFormatOptions)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientPainterConfig::m_outputImagePath, "Output Path",
                        "Output path to save the image to.")
                    ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, GradientImageCreatorRequests::GetSupportedImagesFilter())
                    ->Attribute(AZ::Edit::Attributes::DefaultAsset, "gradientimage_gsi")
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
                ->Field("Previewer", &EditorGradientPainterComponent::m_previewer)
                ->Field("Configuration", &EditorGradientPainterComponent::m_configuration)
                ->Field("ComponentMode", &EditorGradientPainterComponent::m_componentModeDelegate)
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

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGradientPainterComponent::m_previewer, "Preview", "")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGradientPainterComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientPainterComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorGradientPainterComponent::InComponentMode)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorGradientPainterComponent::m_componentModeDelegate,
                        "Paint Image",
                        "Paint into an image asset")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorGradientPainterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientImageCreatorService"));
        services.push_back(AZ_CRC_CE("GradientPainterService"));
    }

    void EditorGradientPainterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientImageCreatorService"));
        services.push_back(AZ_CRC_CE("GradientPainterService"));

        // Don't put this on any entities with another gradient - the gradient previews won't work correctly because this component
        // and the other gradient will both respond to all preview requests, since the requests are made based on an Entity ID,
        // not a Component ID.
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void EditorGradientPainterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("ShapeService"));
    }

    void EditorGradientPainterComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        // Setup the dependency monitor and listen for gradient requests
        SetupDependencyMonitor();

        GradientImageCreatorRequestBus::Handler::BusConnect(GetEntityId());
        GradientPainterRequestBus::Handler::BusConnect(GetEntityId());

        ResizePixelBuffer(AZ::Vector2(0.0f), m_configuration.m_outputResolution);

        m_previewer.SetPreviewSettingsVisible(false);
        m_previewer.SetPreviewEntity(GetEntityId());
        m_previewer.Activate(GetEntityId());

        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorGradientPainterComponent, EditorGradientPainterComponentMode>(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
    }

    void EditorGradientPainterComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_componentModeDelegate.Disconnect();

        m_previewer.Deactivate();

        GradientPainterRequestBus::Handler::BusDisconnect();
        GradientImageCreatorRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        // Free the pixel buffer while the component isn't active.
        ClearPixelBuffer();

        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorGradientPainterComponent::ClearPixelBuffer()
    {
        ResizePixelBuffer(m_pixelBufferResolution, AZ::Vector2(0.0f));
    }

    void EditorGradientPainterComponent::ResizePixelBuffer([[maybe_unused]] const AZ::Vector2& oldSize, const AZ::Vector2& newSize)
    {
        m_pixelBuffer.resize(aznumeric_cast<size_t>(newSize.GetX() * newSize.GetY()));
        m_pixelBufferResolution = newSize;
    }

    void EditorGradientPainterComponent::OnCompositionChanged()
    {
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGradientPainterComponent::OnResolutionChanged()
    {
        ResizePixelBuffer(m_configuration.m_outputResolution, m_configuration.m_outputResolution);
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

    bool EditorGradientPainterComponent::InComponentMode()
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    float EditorGradientPainterComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float outValue = 0.0f;
        GetValues(AZStd::span<const AZ::Vector3>(&sampleParams.m_position, 1), AZStd::span<float>(&outValue, 1));

        return outValue;
    }

    void EditorGradientPainterComponent::GetValues(
        AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            bounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        AZ::Vector3 boundsToPixelIndex = AZ::Vector3(m_configuration.m_outputResolution) / bounds.GetExtents();

        for (size_t index = 0; index < positions.size(); index++)
        {
            // Convert input position to pixel lookup.
            const auto& position = positions[index];
            if (bounds.Contains(position))
            {
                AZ::Vector3 pixelIndex = (position - bounds.GetMin()) * boundsToPixelIndex;
                size_t pixelIndexInt =
                    aznumeric_cast<size_t>((pixelIndex.GetY() * m_configuration.m_outputResolution.GetX()) + pixelIndex.GetX());

                outValues[index] = m_pixelBuffer[pixelIndexInt];
            }
            else
            {
                outValues[index] = 0.0f;
            }
        }
    }

    AZStd::vector<float>* EditorGradientPainterComponent::GetPixelBuffer()
    {
        return &m_pixelBuffer;
    }

    void EditorGradientPainterComponent::RefreshPreview()
    {
        m_previewer.RefreshPreview();
    }

    bool EditorGradientPainterComponent::IsEntityInHierarchy([[maybe_unused]] const AZ::EntityId& entityId) const
    {
        return false;
    }

    AZ::Vector2 EditorGradientPainterComponent::GetOutputResolution() const
    {
        return m_configuration.m_outputResolution;
    }

    void EditorGradientPainterComponent::SetOutputResolution(const AZ::Vector2& resolution)
    {
        ResizePixelBuffer(m_configuration.m_outputResolution, resolution);
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

    void EditorGradientPainterComponent::OnConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = m_previewer.CancelPreviewRendering();

        // Re-setup the dependency monitor when the configuration changes because the gradient sampler
        // could've changed
        SetupDependencyMonitor();

        if (m_configuration.m_outputResolution != m_pixelBufferResolution)
        {
            ResizePixelBuffer(m_pixelBufferResolution, m_configuration.m_outputResolution);
        }

        // Refresh any of the previews that we canceled that were still in progress so they can be completed
        m_previewer.RefreshPreviews(entityIds);

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void EditorGradientPainterComponent::SaveImage()
    {
        // Get the actual resolution of our image.
        const int imageResolutionX = aznumeric_cast<int>(m_configuration.m_outputResolution.GetX());
        const int imageResolutionY = aznumeric_cast<int>(m_configuration.m_outputResolution.GetY());

        // Get the absolute path for our stored relative path
        AZ::IO::Path fullPathIO = AzToolsFramework::GetAbsolutePathFromRelativePath(m_configuration.m_outputImagePath);

        // Delete the output image (if it exists) before we start baking so that in case
        // the Editor shuts down mid-bake we don't leave the output image in a bad state.
        if (AZ::IO::SystemFile::Exists(fullPathIO.c_str()))
        {
            AZ::IO::SystemFile::Delete(fullPathIO.c_str());
        }


        // The TGA and EXR formats aren't recognized with only single channel data,
        // so need to use RGBA format for them
        int channels = 1;
        if (fullPathIO.Extension() == ".tga" || fullPathIO.Extension() == ".exr")
        {
            channels = 4;
        }

        int bytesPerPixel = 0;
        OIIO::TypeDesc pixelFormat = OIIO::TypeDesc::UINT8;
        switch (m_configuration.m_outputFormat)
        {
        case OutputFormat::R8:
            bytesPerPixel = 1;
            pixelFormat = OIIO::TypeDesc::UINT8;
            break;
        case OutputFormat::R16:
            bytesPerPixel = 2;
            pixelFormat = OIIO::TypeDesc::UINT16;
            break;
        case OutputFormat::R32:
            bytesPerPixel = 4;
            pixelFormat = OIIO::TypeDesc::FLOAT;
            break;
        default:
            AZ_Assert(false, "Unsupported output image format (%d)", m_configuration.m_outputFormat);
            return;
        }
        const size_t imageSize = imageResolutionX * imageResolutionY * channels * bytesPerPixel;
        AZStd::vector<AZ::u8> pixels(imageSize, 0);

        AZ::IO::Path absolutePath = fullPathIO.LexicallyNormal();
        std::unique_ptr<OIIO::ImageOutput> outputImage = OIIO::ImageOutput::create(absolutePath.c_str());
        if (!outputImage)
        {
            AZ_Error("GradientBaker", false, "Failed to write out gradient baked image to path: %s", absolutePath.c_str());
            return;
        }

        OIIO::ImageSpec spec(imageResolutionX, imageResolutionY, channels, pixelFormat);
        outputImage->open(absolutePath.c_str(), spec);

        for (size_t pixel = 0; pixel < (imageResolutionX * imageResolutionY); pixel++)
        {
            // Write out the sample value for the pixel based on output format
            size_t index = (pixel * channels);
            float sample = m_pixelBuffer[pixel];
            switch (m_configuration.m_outputFormat)
            {
            case OutputFormat::R8:
                {
                    AZ::u8 value = static_cast<AZ::u8>(sample * std::numeric_limits<AZ::u8>::max());
                    pixels[index] = value; // R

                    if (channels == 4)
                    {
                        pixels[index + 1] = value; // G
                        pixels[index + 2] = value; // B
                        pixels[index + 3] = std::numeric_limits<AZ::u8>::max(); // A
                    }
                    break;
                }
            case OutputFormat::R16:
                {
                    auto actualMem = reinterpret_cast<AZ::u16*>(pixels.data());
                    AZ::u16 value = static_cast<AZ::u16>(sample * std::numeric_limits<AZ::u16>::max());
                    actualMem[index] = value; // R

                    if (channels == 4)
                    {
                        actualMem[index + 1] = value; // G
                        actualMem[index + 2] = value; // B
                        actualMem[index + 3] = std::numeric_limits<AZ::u16>::max(); // A
                    }
                    break;
                }
            case OutputFormat::R32:
                {
                    auto actualMem = reinterpret_cast<float*>(pixels.data());
                    actualMem[index] = sample; // R

                    if (channels == 4)
                    {
                        actualMem[index + 1] = sample; // G
                        actualMem[index + 2] = sample; // B
                        actualMem[index + 3] = 1.0f; // A
                    }
                    break;
                }
            }
        }

        // Don't try to write out the image if the job was canceled
        bool result = outputImage->write_image(pixelFormat, pixels.data());
        if (!result)
        {
            AZ_Error("GradientBaker", result, "Failed to write out gradient baked image to path: %s", absolutePath.c_str());
        }

        outputImage->close();
    }


} // namespace GradientSignal
