/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorGradientBakerComponent.h"

#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyFilePathCtrl.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING

namespace GradientSignal
{
    // Custom AZ::Job so that we can bake the output image asynchronously.
    // We create the AZ::Job with isAutoDelete = false so that we can detect
    // when the job has completed, which means we need to handle its deletion.
    BakeImageJob::BakeImageJob(
        const GradientBakerConfig& configuration,
        const AZ::IO::Path& fullPath,
        AZ::Aabb inputBounds,
        AZ::EntityId boundsEntityId
        )
        : AZ::Job(false, nullptr)
        , m_configuration(configuration)
        , m_outputImageAbsolutePath(fullPath)
        , m_inputBounds(inputBounds)
        , m_boundsEntityId(boundsEntityId)
    {
    }

    BakeImageJob::~BakeImageJob()
    {
        // Make sure we don't have anything running on another thread before destroying
        // the job instance itself.
        CancelAndWait();
    }

    void BakeImageJob::Process()
    {
        // Get the actual resolution of our image.  Note that this might be non-square, depending on how the window is sized.
        const int imageResolutionX = aznumeric_cast<int>(m_configuration.m_outputResolution.GetX());
        const int imageResolutionY = aznumeric_cast<int>(m_configuration.m_outputResolution.GetY());

        // The TGA and EXR formats aren't recognized with only single channel data,
        // so need to use RGBA format for them
        int channels = 1;
        if (m_outputImageAbsolutePath.Extension() == ".tga" || m_outputImageAbsolutePath.Extension() == ".exr")
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

        AZ::IO::Path absolutePath = m_outputImageAbsolutePath.LexicallyNormal();
        std::unique_ptr<OIIO::ImageOutput> outputImage = OIIO::ImageOutput::create(absolutePath.c_str());
        if (!outputImage)
        {
            AZ_Error("GradientBaker", false, "Failed to write out gradient baked image to path: %s",
                absolutePath.c_str());
            return;
        }

        OIIO::ImageSpec spec(imageResolutionX, imageResolutionY, channels, pixelFormat);
        outputImage->open(absolutePath.c_str(), spec);

        const AZ::Vector3 inputBoundsCenter = m_inputBounds.GetCenter();
        const AZ::Vector3 inputBoundsExtentsOld = m_inputBounds.GetExtents();
        m_inputBounds =
            AZ::Aabb::CreateCenterRadius(inputBoundsCenter, AZ::GetMax(inputBoundsExtentsOld.GetX(), inputBoundsExtentsOld.GetY()) / 2.0f);

        const AZ::Vector3 inputBoundsStart =
            AZ::Vector3(m_inputBounds.GetMin().GetX(), m_inputBounds.GetMin().GetY(), inputBoundsCenter.GetZ());
        const AZ::Vector3 inputBoundsExtents = m_inputBounds.GetExtents();
        const float inputBoundsExtentsX = inputBoundsExtents.GetX();
        const float inputBoundsExtentsY = inputBoundsExtents.GetY();

        // When sampling the gradient, we can choose to either do it at the corners of each texel area we're sampling, or at the center.
        // They're both correct choices in different ways.  We're currently choosing to do the corners, which makes scaledTexelOffset = 0,
        // but the math is here to make it easy to change later if we ever decide sampling from the center provides a more intuitive
        // image.
        constexpr float texelOffset = 0.0f; // Use 0.5f to sample from the center of the texel.
        const AZ::Vector3 scaledTexelOffset(
            texelOffset * inputBoundsExtentsX / static_cast<float>(imageResolutionX),
            texelOffset * inputBoundsExtentsY / static_cast<float>(imageResolutionY), 0.0f);

        // Scale from our image size space (ex: 256 pixels) to our bounds space (ex: 16 meters)
        const AZ::Vector3 pixelToBoundsScale(
            inputBoundsExtentsX / static_cast<float>(imageResolutionX), inputBoundsExtentsY / static_cast<float>(imageResolutionY), 0.0f);

        for (int y = 0; !m_shouldCancel && (y < imageResolutionY); ++y)
        {
            for (int x = 0; !m_shouldCancel && (x < imageResolutionX); ++x)
            {
                // Invert world y to match axis.  (We use "imageBoundsY- 1" to invert because our loop doesn't go all the way to
                // imageBoundsY)
                AZ::Vector3 uvw(static_cast<float>(x), static_cast<float>((imageResolutionY - 1) - y), 0.0f);

                GradientSampleParams sampleParams;
                sampleParams.m_position = inputBoundsStart + (uvw * pixelToBoundsScale) + scaledTexelOffset;

                bool inBounds = true;
                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                    inBounds, m_boundsEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, sampleParams.m_position);

                float sample = inBounds ? m_configuration.m_gradientSampler.GetValue(sampleParams) : 0.0f;

                // Write out the sample value for the pixel based on output format
                int index = ((y * imageResolutionX) + x) * channels;
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
        }

        // Don't try to write out the image if the job was canceled
        if (!m_shouldCancel)
        {
            bool result = outputImage->write_image(pixelFormat, pixels.data());
            if (!result)
            {
                AZ_Error("GradientBaker", result, "Failed to write out gradient baked image to path: %s",
                    absolutePath.c_str());
            }

            outputImage->close();
        }

        // Safely notify that the job has finished
        // The m_finishedNotify is used to notify our blocking wait to cancel the job
        // while it was running
        {
            AZStd::lock_guard<decltype(m_bakeImageMutex)> lock(m_bakeImageMutex);
            m_shouldCancel = false;
            m_isFinished.store(true);
            m_finishedNotify.notify_all();
        }
    }

    void BakeImageJob::CancelAndWait()
    {
        // Set an atomic bool that the Process() loop checks on each iteration to see
        // if it should cancel baking the image
        m_shouldCancel = true;

        // Then we synchronously block until the job has completed
        AZStd::unique_lock<decltype(m_bakeImageMutex)> lock(m_bakeImageMutex);
        if (!m_isFinished)
        {
            m_finishedNotify.wait(lock, [this] { return m_isFinished == true; });
        }

        Reset(true);
        m_shouldCancel = false;
    }

    bool BakeImageJob::IsFinished() const
    {
        return m_isFinished.load();
    }

    AZStd::string GetSupportedImagesFilter()
    {
        // Build filter for supported streaming image formats that will be used on the
        // native file dialog when creating/picking an output file for the baked image.
        // ImageProcessingAtom::s_SupportedImageExtensions actually has more formats
        // that will produce streaming image assets, but not all of them support
        // all of the bit depths we care about (8/16/32), so we've reduced the list
        // to the image formats that do.
        return "Images (*.png *.tif *.tiff *.tga *.exr)";
    }

    AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> SupportedOutputFormatOptions()
    {
        AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> options;

        options.push_back(AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R8, "R8 (8-bit)"));
        options.push_back(AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R16, "R16 (16-bit)"));
        options.push_back(AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R32, "R32 (32-bit)"));

        return options;
    }

    void GradientBakerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientBakerConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("Gradient", &GradientBakerConfig::m_gradientSampler)
                ->Field("InputBounds", &GradientBakerConfig::m_inputBounds)
                ->Field("OutputResolution", &GradientBakerConfig::m_outputResolution)
                ->Field("OutputFormat", &GradientBakerConfig::m_outputFormat)
                ->Field("OutputImagePath", &GradientBakerConfig::m_outputImagePath)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientBakerConfig>("Gradient Baker", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_gradientSampler, "Gradient",
                        "Input gradient to bake the output image from.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_inputBounds, "Input Bounds",
                        "Input bounds for where to sample the data.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_outputResolution, "Resolution",
                        "Output resolution of the baked image.")
                    ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                    ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &GradientBakerConfig::m_outputFormat, "Output Format",
                        "Output format of the baked image.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &SupportedOutputFormatOptions)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_outputImagePath, "Output Path",
                        "Output path to bake the image to.")
                    ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, GetSupportedImagesFilter())
                    ->Attribute(AZ::Edit::Attributes::DefaultAsset, "baked_output_gsi")
                    ;
            }
        }
    }

    void EditorGradientBakerComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientBakerConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGradientBakerComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Configuration", &EditorGradientBakerComponent::m_configuration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorGradientBakerComponent>(
                        EditorGradientBakerComponent::s_componentName, EditorGradientBakerComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorGradientBakerComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->UIElement(AZ_CRC_CE("GradientPreviewer"), "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &EditorGradientBakerComponent::GetGradientEntityId)
                    ->EndGroup()

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGradientBakerComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "BakeImage", "Bakes the inbound gradient signal to an image asset")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Bake image")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::BakeImage)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorGradientBakerComponent::IsBakeDisabled)
                    ;
            }
        }
    }

    void EditorGradientBakerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientBakerService"));
    }

    void EditorGradientBakerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientBakerService"));
    }

    void EditorGradientBakerComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        m_gradientEntityId = GetEntityId();

        SectorDataNotificationBus::Handler::BusConnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        GradientPreviewContextRequestBus::Handler::BusConnect(GetEntityId());

        m_configuration.m_gradientSampler.m_ownerEntityId = GetEntityId();

        // Validation needs to happen after the ownerEntity is set in case the validation needs that data
        if (!m_configuration.m_gradientSampler.ValidateGradientEntityId())
        {
            SetDirty();
        }

        // Setup the dependency monitor and listen for gradient requests
        SetupDependencyMonitor();

        UpdatePreviewSettings();

        // If we have a valid output image path set and the other criteria for baking
        // are met but the image doesn't exist, then bake it when we activate our component.
        if (!IsBakeDisabled())
        {
            AZ::IO::Path fullPathIO = AzToolsFramework::GetAbsolutePathFromRelativePath(m_configuration.m_outputImagePath);
            if (!AZ::IO::SystemFile::Exists(fullPathIO.c_str()))
            {
                // Delay actually starting the bake until the next tick to
                // make sure everything is ready
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }

    void EditorGradientBakerComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        // If the preview shouldn't be active, use an invalid entityId
        m_gradientEntityId = AZ::EntityId();

        // If we had a bake job running, delete it before deactivating
        // This delete will cancel the job and block waiting for it to complete
        AZ::TickBus::Handler::BusDisconnect();
        if (m_bakeImageJob)
        {
            delete m_bakeImageJob;
            m_bakeImageJob = nullptr;
        }

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientPreviewContextRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        SectorDataNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorGradientBakerComponent::OnCompositionChanged()
    {
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGradientBakerComponent::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration
        // change)
        GradientSignal::GradientPreviewRequestBus::Event(m_gradientEntityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

    AzToolsFramework::EntityIdList EditorGradientBakerComponent::CancelPreviewRendering() const
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

    void EditorGradientBakerComponent::SetupDependencyMonitor()
    {
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);

        // Connect to GradientRequestBus after the gradient sampler and dependency monitor is configured
        // before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorGradientBakerComponent::BakeImage()
    {
        if (IsBakeDisabled())
        {
            return;
        }

        AZ::TickBus::Handler::BusConnect();

        StartBakeImageJob();
    }

    void EditorGradientBakerComponent::StartBakeImageJob()
    {
        // Get the absolute path for our stored relative path
        AZ::IO::Path fullPathIO = AzToolsFramework::GetAbsolutePathFromRelativePath(m_configuration.m_outputImagePath);

        // Delete the output image (if it exists) before we start baking so that in case
        // the Editor shuts down mid-bake we don't leave the output image in a bad state.
        if (AZ::IO::SystemFile::Exists(fullPathIO.c_str()))
        {
            AZ::IO::SystemFile::Delete(fullPathIO.c_str());
        }

        m_bakeImageJob = aznew BakeImageJob(m_configuration, fullPathIO, GetPreviewBounds(), GetPreviewEntity());
        m_bakeImageJob->Start();

        // Force a refresh now so the bake button gets disabled
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    bool EditorGradientBakerComponent::IsBakeDisabled() const
    {
        return m_configuration.m_outputImagePath.empty() || !m_configuration.m_gradientSampler.m_gradientId.IsValid() ||
            !m_configuration.m_inputBounds.IsValid() || m_bakeImageJob;
    }

    AZ::EntityId EditorGradientBakerComponent::GetPreviewEntity() const
    {
        // Our preview entity will always be ourself since we want to preview
        // exactly what's going to be in the baked image.
        return GetEntityId();
    }

    AZ::Aabb EditorGradientBakerComponent::GetPreviewBounds() const
    {
        AZ::Aabb bounds = AZ::Aabb::CreateNull();

        if (m_configuration.m_inputBounds.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                bounds, m_configuration.m_inputBounds, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        return bounds;
    }

    void EditorGradientBakerComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_bakeImageJob && m_bakeImageJob->IsFinished())
        {
            delete m_bakeImageJob;
            m_bakeImageJob = nullptr;

            AZ::TickBus::Handler::BusDisconnect();

            // After a successful bake, if the Entity that contains this gradient baker component also
            // has an image gradient component, then update the image gradient's image asset with the
            // output path that we baked to
            if (GradientSignal::ImageGradientRequestBus::HasHandlers(GetEntityId()))
            {
                AzToolsFramework::ScopedUndoBatch undo("Update Image Gradient Asset");

                QString outputImagePath = QString::fromUtf8(
                    m_configuration.m_outputImagePath.c_str(), static_cast<int>(m_configuration.m_outputImagePath.Native().size()));
                outputImagePath += ".streamingimage";

                GradientSignal::ImageGradientRequestBus::Event(
                    GetEntityId(), &GradientSignal::ImageGradientRequests::SetImageAssetPath, outputImagePath.toUtf8().constData());

                undo.MarkEntityDirty(GetEntityId());
            }

            // Refresh once the job has completed so the Bake button can be re-enabled
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
        else if (!m_bakeImageJob)
        {
            // If we didn't have a bake job already going, start one now
            // This is to handle the case where the bake is initiated when
            // activating the component and the output image doesn't exist
            StartBakeImageJob();
        }
    }

    AZ::EntityId EditorGradientBakerComponent::GetGradientEntityId() const
    {
        return m_gradientEntityId;
    }

    float EditorGradientBakerComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        return m_configuration.m_gradientSampler.GetValue(sampleParams);
    }

    void EditorGradientBakerComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        m_configuration.m_gradientSampler.GetValues(positions, outValues);
    }

    bool EditorGradientBakerComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    void EditorGradientBakerComponent::OnSectorDataConfigurationUpdated() const
    {
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void EditorGradientBakerComponent::OnSelected()
    {
        UpdatePreviewSettings();
    }

    void EditorGradientBakerComponent::OnDeselected()
    {
        UpdatePreviewSettings();
    }

    void EditorGradientBakerComponent::OnConfigurationChanged()
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
