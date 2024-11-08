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
#include <GradientSignal/Editor/EditorGradientBakerComponent.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorUtils.h>

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

        int bytesPerChannel = ImageCreatorUtils::GetBytesPerChannel(m_configuration.m_outputFormat);

        const size_t imageSize = imageResolutionX * imageResolutionY * channels * bytesPerChannel;
        AZStd::vector<AZ::u8> pixels(imageSize, 0);

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

        const AZ::Vector3 positionOffset = inputBoundsStart + scaledTexelOffset;

        // Generate a set of input positions that are inside the bounds along with
        // their corresponding x,y indices
        AZStd::vector<AZ::Vector3> inputPositions;
        inputPositions.reserve(imageResolutionX * imageResolutionY);
        AZStd::vector<AZStd::pair<int, int>> indices;
        indices.reserve(imageResolutionX * imageResolutionY);

        // All the input position gathering logic occurs in this lambda passed to the
        // ShapeComponentRequestsBus so that we only need one bus call
        LmbrCentral::ShapeComponentRequestsBus::Event(
            m_boundsEntityId,
            [this, positionOffset, pixelToBoundsScale, imageResolutionX, imageResolutionY, &inputPositions, &indices](LmbrCentral::ShapeComponentRequestsBus::Events* shape)
            {
                for (int y = 0; !m_shouldCancel && (y < imageResolutionY); ++y)
                {
                    for (int x = 0; x < imageResolutionX; ++x)
                    {
                        // Invert world y to match axis.  (We use "imageBoundsY- 1" to invert because our loop doesn't go all the way to
                        // imageBoundsY)
                        AZ::Vector3 uvw(static_cast<float>(x), static_cast<float>((imageResolutionY - 1) - y), 0.0f);

                        AZ::Vector3 position = positionOffset + (uvw * pixelToBoundsScale);

                        if (!shape->IsPointInside(position))
                        {
                            continue;
                        }

                        // Keep track of this input position + the x,y indices
                        inputPositions.push_back(position);
                        indices.push_back(AZStd::make_pair(x, y));
                    }
                }
            });

        // Retrieve all the gradient values for the input positions
        const size_t numPositions = inputPositions.size();
        AZStd::vector<float> outputValues(numPositions);
        m_configuration.m_gradientSampler.GetValues(inputPositions, outputValues);

        // Write out all the gradient values to our output image
        for (int i = 0; !m_shouldCancel && (i < numPositions); ++i)
        {
            const float& sample = outputValues[i];
            const auto& [x, y] = indices[i];

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

        // Don't try to write out the image if the job was canceled
        if (!m_shouldCancel)
        {
            constexpr bool showProgressDialog = false;
            bool result = ImageCreatorUtils::WriteImage(
                m_outputImageAbsolutePath.c_str(),
                imageResolutionX, imageResolutionY, channels, m_configuration.m_outputFormat, pixels,
                showProgressDialog);
            if (!result)
            {
                AZ_Error(
                    "GradientBaker", result, "Failed to write out gradient baked image to path: %s", m_outputImageAbsolutePath.c_str());
            }
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
        Wait();
    }

    void BakeImageJob::Wait()
    {
        // Jobs don't inherently have a way to block on cancellation / completion, so we need to implement it
        // ourselves.

        // If we've already started the job, block on a condition variable that gets notified at
        // the end of the Process() function.
        AZStd::unique_lock<decltype(m_bakeImageMutex)> lock(m_bakeImageMutex);
        if (!m_isFinished)
        {
            m_finishedNotify.wait(lock, [this] { return m_isFinished == true; });
        }

        // Regardless of whether or not we were running, we need to reset the internal Job class status
        // and clear our cancel flag.
        Reset(true);
        m_shouldCancel = false;
    }

    bool BakeImageJob::IsFinished() const
    {
        return m_isFinished.load();
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
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &ImageCreatorUtils::SupportedOutputFormatOptions)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_outputImagePath, "Output Path",
                        "Output path to bake the image to.")
                    ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, ImageCreatorUtils::GetSupportedImagesFilter())
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
                ->Version(1)
                ->Field("Previewer", &EditorGradientBakerComponent::m_previewer)
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

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGradientBakerComponent::m_previewer, "Previewer", "")

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

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GradientImageCreatorRequestBus>("GradientImageCreatorRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gradient")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "gradient")
                ->Event("GetOutputResolution", &GradientImageCreatorRequests::GetOutputResolution)
                ->Event("SetOutputResolution", &GradientImageCreatorRequests::SetOutputResolution)
                ->VirtualProperty("OutputResolution", "GetOutputResolution", "SetOutputResolution")
                ->Event("GetOutputFormat", &GradientImageCreatorRequests::GetOutputFormat)
                ->Event("SetOutputFormat", &GradientImageCreatorRequests::SetOutputFormat)
                ->VirtualProperty("OutputFormat", "GetOutputFormat", "SetOutputFormat")
                ->Event("GetOutputImagePath", &GradientImageCreatorRequests::GetOutputImagePath)
                ->Event("SetOutputImagePath", &GradientImageCreatorRequests::SetOutputImagePath)
                ->VirtualProperty("OutputImagePath", "GetOutputImagePath", "SetOutputImagePath");


            behaviorContext->Class<EditorGradientBakerComponent>()
                ->RequestBus("GradientImageCreatorRequestBus")
                ->RequestBus("GradientBakerRequestBus")
                ;

            behaviorContext->EBus<GradientBakerRequestBus>("GradientBakerRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gradient")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "gradient")
                ->Event("BakeImage", &GradientBakerRequests::BakeImage)
                ->Event("GetInputBounds", &GradientBakerRequests::GetInputBounds)
                ->Event("SetInputBounds", &GradientBakerRequests::SetInputBounds)
                ->VirtualProperty("InputBounds", "GetInputBounds", "SetInputBounds");
        }
    }

    void EditorGradientBakerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientImageCreatorService"));
        services.push_back(AZ_CRC_CE("GradientBakerService"));
    }

    void EditorGradientBakerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientImageCreatorService"));
        services.push_back(AZ_CRC_CE("GradientBakerService"));
    }

    void EditorGradientBakerComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        m_configuration.m_gradientSampler.m_ownerEntityId = GetEntityId();

        // Validation needs to happen after the ownerEntity is set in case the validation needs that data
        if (!m_configuration.m_gradientSampler.ValidateGradientEntityId())
        {
            SetDirty();
        }

        // Setup the dependency monitor and listen for gradient requests
        SetupDependencyMonitor();

        GradientBakerRequestBus::Handler::BusConnect(GetEntityId());
        GradientImageCreatorRequestBus::Handler::BusConnect(GetEntityId());

        m_previewer.SetPreviewSettingsVisible(false);
        m_previewer.SetPreviewEntity(m_configuration.m_inputBounds);
        m_previewer.Activate(GetEntityId());

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

        GradientImageCreatorRequestBus::Handler::BusDisconnect();
        GradientBakerRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        m_previewer.Deactivate();

        // If we had a bake job running, delete it before deactivating
        // This delete will cancel the job and block waiting for it to complete
        AZ::TickBus::Handler::BusDisconnect();
        if (m_bakeImageJob)
        {
            delete m_bakeImageJob;
            m_bakeImageJob = nullptr;
        }

        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorGradientBakerComponent::OnCompositionChanged()
    {
        m_previewer.SetPreviewEntity(m_configuration.m_inputBounds);
        m_previewer.RefreshPreview();

        InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
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

        m_bakeImageJob = aznew BakeImageJob(m_configuration, fullPathIO, m_previewer.GetPreviewBounds(), m_configuration.m_inputBounds);
        m_bakeImageJob->Start();

        // Force a refresh now so the bake button gets disabled
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
    }

    bool EditorGradientBakerComponent::IsBakeDisabled() const
    {
        return m_configuration.m_outputImagePath.empty() || !m_configuration.m_gradientSampler.m_gradientId.IsValid() ||
            !m_configuration.m_inputBounds.IsValid() || m_bakeImageJob;
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

                GradientSignal::ImageGradientRequestBus::Event(
                    GetEntityId(), &GradientSignal::ImageGradientRequests::SetImageAssetSourcePath, m_configuration.m_outputImagePath.c_str());

                undo.MarkEntityDirty(GetEntityId());
            }

            // Refresh once the job has completed so the Bake button can be re-enabled
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
        }
        else if (!m_bakeImageJob)
        {
            // If we didn't have a bake job already going, start one now
            // This is to handle the case where the bake is initiated when
            // activating the component and the output image doesn't exist
            StartBakeImageJob();
        }
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

    AZ::EntityId EditorGradientBakerComponent::GetInputBounds() const
    {
        return m_configuration.m_inputBounds;
    }

    void EditorGradientBakerComponent::SetInputBounds(const AZ::EntityId& inputBounds)
    {
        m_configuration.m_inputBounds = inputBounds;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector2 EditorGradientBakerComponent::GetOutputResolution() const
    {
        return m_configuration.m_outputResolution;
    }

    void EditorGradientBakerComponent::SetOutputResolution(const AZ::Vector2& resolution)
    {
        m_configuration.m_outputResolution = resolution;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    OutputFormat EditorGradientBakerComponent::GetOutputFormat() const
    {
        return m_configuration.m_outputFormat;
    }

    void EditorGradientBakerComponent::SetOutputFormat(OutputFormat outputFormat)
    {
        m_configuration.m_outputFormat = outputFormat;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::IO::Path EditorGradientBakerComponent::GetOutputImagePath() const
    {
        return m_configuration.m_outputImagePath;
    }

    void EditorGradientBakerComponent::SetOutputImagePath(const AZ::IO::Path& outputImagePath)
    {
        m_configuration.m_outputImagePath = outputImagePath;

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void EditorGradientBakerComponent::OnConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = m_previewer.CancelPreviewRendering();

        // Re-setup the dependency monitor when the configuration changes because the gradient sampler
        // could've changed
        SetupDependencyMonitor();

        // Refresh any of the previews that we canceled that were still in progress so they can be completed
        m_previewer.RefreshPreviews(entityIds);

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
} // namespace GradientSignal
