/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FrameCaptureSystemComponent.h"

#include <Atom/RHI/RHIUtils.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/ViewportContextManager.h>

#include <Atom/Utils/DdsFile.h>
#include <Atom/Utils/PpmFile.h>
#include <Atom/Utils/PngFile.h>
#include <Atom/Utils/ImageComparison.h>
#include <AzCore/std/parallel/lock.h>

#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Task/TaskGraph.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/Console/Console.h>

#include <tiffio.h>

namespace AZ
{
    namespace Render
    {
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(FrameCaptureResult);

        void FrameCaptureError::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<FrameCaptureError>()
                    ->Version(1)
                    ->Field("ErrorMessage", &FrameCaptureError::m_errorMessage);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<FrameCaptureError>("FrameCaptureError")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "utils")
                    ->Property("ErrorMessage", BehaviorValueProperty(&FrameCaptureError::m_errorMessage))
                        ->Attribute(AZ::Script::Attributes::Alias, "error_message");
            }
        }
        void FrameCaptureTestError::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<FrameCaptureTestError>()
                    ->Version(1)
                    ->Field("ErrorMessage", &FrameCaptureTestError::m_errorMessage);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<FrameCaptureTestError>("FrameCaptureTestError")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "utils")
                    ->Property("ErrorMessage", BehaviorValueProperty(&FrameCaptureTestError::m_errorMessage))
                        ->Attribute(AZ::Script::Attributes::Alias, "error_message");
            }
        }

        AZ_CVAR(unsigned int,
            r_pngCompressionLevel,
            3, // A compression level of 3 seems like the best default in terms of file size and saving speeds
            nullptr,
            ConsoleFunctorFlags::Null,
            "Sets the compression level for saving png screenshots. Valid values are from 0 to 8"
        );

        AZ_CVAR(int,
            r_pngCompressionNumThreads,
            8, // Number of threads to use for the png r<->b channel data swap
            nullptr,
            ConsoleFunctorFlags::Null,
            "Sets the number of threads for saving png screenshots. Valid values are from 1 to 128, although less than or equal the number of hw threads is recommended"
        );


        FrameCaptureOutputResult PngFrameCaptureOutput(
            const AZStd::string& outputFilePath, const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
        {
            AZStd::shared_ptr<AZStd::vector<uint8_t>> buffer = readbackResult.m_dataBuffer;

            RHI::Format format = readbackResult.m_imageDescriptor.m_format;

            // convert bgra to rgba by swapping channels
            const int numChannels = AZ::RHI::GetFormatComponentCount(readbackResult.m_imageDescriptor.m_format);
            if (format == RHI::Format::B8G8R8A8_UNORM)
            {
                format = RHI::Format::R8G8B8A8_UNORM;

                buffer = AZStd::make_shared<AZStd::vector<uint8_t>>(readbackResult.m_dataBuffer->size());
                AZStd::copy(readbackResult.m_dataBuffer->begin(), readbackResult.m_dataBuffer->end(), buffer->begin());
                const int numThreads = r_pngCompressionNumThreads;
                const int numPixelsPerThread = static_cast<int>(buffer->size() / numChannels / numThreads);

                AZ::TaskGraphActiveInterface* taskGraphActiveInterface = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();
                bool taskGraphActive = taskGraphActiveInterface && taskGraphActiveInterface->IsTaskGraphActive();

                if (taskGraphActive)
                {
                    static const AZ::TaskDescriptor pngTaskDescriptor{"PngWriteOutChannelSwap", "Graphics"};
                    AZ::TaskGraph taskGraph{ "FrameCapturePngWriteOut" };
                    for (int i = 0; i < numThreads; ++i)
                    {
                        int startPixel = i * numPixelsPerThread;

                        taskGraph.AddTask(
                            pngTaskDescriptor,
                            [&, startPixel]()
                            {
                                for (int pixelOffset = 0; pixelOffset < numPixelsPerThread; ++pixelOffset)
                                {
                                    if (startPixel * numChannels + numChannels < buffer->size())
                                    {
                                        AZStd::swap(
                                            buffer->data()[(startPixel + pixelOffset) * numChannels],
                                            buffer->data()[(startPixel + pixelOffset) * numChannels + 2]
                                        );
                                    }
                                }
                            });
                    }
                    AZ::TaskGraphEvent taskGraphFinishedEvent{ "FrameCapturePngWriteOutWait" };
                    taskGraph.Submit(&taskGraphFinishedEvent);
                    taskGraphFinishedEvent.Wait();
                }
                else
                {
                    AZ::JobCompletion jobCompletion;
                    for (int i = 0; i < numThreads; ++i)
                    {
                        int startPixel = i * numPixelsPerThread;

                        AZ::Job* job = AZ::CreateJobFunction(
                            [&, startPixel]()
                            {
                                for (int pixelOffset = 0; pixelOffset < numPixelsPerThread; ++pixelOffset)
                                {
                                    if (startPixel * numChannels + numChannels < buffer->size())
                                    {
                                        AZStd::swap(
                                            buffer->data()[(startPixel + pixelOffset) * numChannels],
                                            buffer->data()[(startPixel + pixelOffset) * numChannels + 2]
                                        );
                                    }
                                }
                            }, true, nullptr);

                        job->SetDependent(&jobCompletion);
                        job->Start();
                    }
                    jobCompletion.StartAndWaitForCompletion();
                }
            }

            Utils::PngFile image = Utils::PngFile::Create(readbackResult.m_imageDescriptor.m_size, format, *buffer);

            Utils::PngFile::SaveSettings saveSettings;

            if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("r_pngCompressionLevel", saveSettings.m_compressionLevel);
            }

            // We should probably strip alpha to save space, especially for automated test screenshots. Alpha is left in to maintain
            // prior behavior, changing this is out of scope for the current task. Note, it would have bit of a cascade effect where
            // AtomSampleViewer's ScriptReporter assumes an RGBA image.
            saveSettings.m_stripAlpha = false; 

            if(image && image.Save(outputFilePath.c_str(), saveSettings))
            {
                return FrameCaptureOutputResult{FrameCaptureResult::Success, AZStd::nullopt};
            }

            return FrameCaptureOutputResult{FrameCaptureResult::InternalError, "Unable to save frame capture output to '" + outputFilePath + "'"};
        }

        FrameCaptureOutputResult TiffFrameCaptureOutput(
            const AZStd::string& outputFilePath, const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
        {
            AZStd::shared_ptr<AZStd::vector<uint8_t>> buffer = readbackResult.m_dataBuffer;
            const uint32_t width = readbackResult.m_imageDescriptor.m_size.m_width;
            const uint32_t height = readbackResult.m_imageDescriptor.m_size.m_height;
            const uint32_t numChannels = AZ::RHI::GetFormatComponentCount(readbackResult.m_imageDescriptor.m_format);
            const uint32_t bytesPerChannel = AZ::RHI::GetFormatSize(readbackResult.m_imageDescriptor.m_format) / numChannels;
            const uint32_t bitsPerChannel = bytesPerChannel * 8;

            TIFF* out = TIFFOpen(outputFilePath.c_str(), "w");
            TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
            TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
            TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, numChannels);
            TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bitsPerChannel);
            TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
            TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
            TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);   // interpret each pixel as a float

            size_t pitch = width * numChannels * bytesPerChannel;
            AZ_Assert((pitch * height) == buffer->size(), "Image buffer does not match allocated bytes for tiff saving.")
            unsigned char* raster = (unsigned char*)_TIFFmalloc((tsize_t)(pitch * height));
            memcpy(raster, buffer->data(), pitch * height);
            bool success = true;
            for (uint32_t h = 0; h < height; ++h)
            {
                size_t offset = h * pitch;
                int err = TIFFWriteScanline(out, raster + offset, h, 0);
                if (err < 0)
                {
                    success = false;
                    break;
                }
            }
            _TIFFfree(raster);
            TIFFClose(out);
            return success ? FrameCaptureOutputResult{ FrameCaptureResult::Success, AZStd::nullopt }
                           : FrameCaptureOutputResult{ FrameCaptureResult::InternalError, "Unable to save tif frame capture output to " + outputFilePath };
        }

        class FrameCaptureNotificationBusHandler final
            : public FrameCaptureNotificationBus::MultiHandler // Use multi handler as it has to handle all use cases
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(FrameCaptureNotificationBusHandler, "{68D1D94C-7055-4D32-8E22-BEEEBA0940C4}", AZ::SystemAllocator, OnFrameCaptureFinished);

            void OnFrameCaptureFinished(FrameCaptureResult result, const AZStd::string& info) override
            {
                Call(FN_OnFrameCaptureFinished, result, info);
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    FrameCaptureResultReflect(*serializeContext);
                }

                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    //[GFX_TODO][ATOM-13424] Replace this with a utility in AZ_ENUM_DEFINE_REFLECT_UTILITIES
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::None)>("FrameCaptureResult_None")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::Success)>("FrameCaptureResult_Success")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::FileWriteError)>("FrameCaptureResult_FileWriteError")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::InvalidArgument)>("FrameCaptureResult_InvalidArgument")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::UnsupportedFormat)>("FrameCaptureResult_UnsupportedFormat")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::InternalError)>("FrameCaptureResult_InternalError")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");

                    behaviorContext->EBus<FrameCaptureNotificationBus>("FrameCaptureNotificationBus")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom")
                        ->Handler<FrameCaptureNotificationBusHandler>()
                        ;
                }
            }
        };

        void FrameCaptureSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            FrameCaptureError::Reflect(context);
            FrameCaptureTestError::Reflect(context);
            Utils::ImageDiffResult::Reflect(context);
            FrameCaptureNotificationBusHandler::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FrameCaptureSystemComponent, AZ::Component>()
                    ->Version(1)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<FrameCaptureRequestBus>("FrameCaptureRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Event("CaptureScreenshot", &FrameCaptureRequestBus::Events::CaptureScreenshot)
                    ->Event("CaptureScreenshotWithPreview", &FrameCaptureRequestBus::Events::CaptureScreenshotWithPreview)
                    ->Event("CapturePassAttachment", &FrameCaptureRequestBus::Events::CapturePassAttachment)
                    ;

                behaviorContext->EBus<FrameCaptureTestRequestBus>("FrameCaptureTestRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Event("SetScreenshotFolder", &FrameCaptureTestRequestBus::Events::SetScreenshotFolder)
                    ->Event("SetTestEnvPath", &FrameCaptureTestRequestBus::Events::SetTestEnvPath)
                    ->Event("SetOfficialBaselineImageFolder", &FrameCaptureTestRequestBus::Events::SetOfficialBaselineImageFolder)
                    ->Event("SetLocalBaselineImageFolder", &FrameCaptureTestRequestBus::Events::SetLocalBaselineImageFolder)
                    ->Event("BuildScreenshotFilePath", &FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath)
                    ->Event("BuildOfficialBaselineFilePath", &FrameCaptureTestRequestBus::Events::BuildOfficialBaselineFilePath)
                    ->Event("BuildLocalBaselineFilePath", &FrameCaptureTestRequestBus::Events::BuildLocalBaselineFilePath)
                    ->Event("CompareScreenshots", &FrameCaptureTestRequestBus::Events::CompareScreenshots)
                    ;
            }
        }

        void FrameCaptureSystemComponent::Activate()
        {
            FrameCaptureRequestBus::Handler::BusConnect();
            FrameCaptureTestRequestBus::Handler::BusConnect();
            SystemTickBus::Handler::BusConnect();
        }

        FrameCaptureSystemComponent::CaptureHandle FrameCaptureSystemComponent::InitCapture()
        {
            if (m_idleCaptures.size())
            {
                // Use an existing idle capture state
                CaptureHandle captureHandle = m_idleCaptures.front();
                m_idleCaptures.pop_front();
                if (captureHandle.IsNull())
                {
                    AZ_Assert(false, "FrameCaptureSystemComponent found null capture handle in idle list");
                    return CaptureHandle::Null();
                }
                AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle); // take shared read lock to ensure vector doesn't move while operating on the ptr
                CaptureState* capture = captureHandle.GetCaptureState();
                if (!capture) // failed to get the capture state ptr, abort
                {
                    return CaptureHandle::Null();
                }
                capture->Reset();
                return captureHandle;
            }
            else
            {
                // Create a new CaptureState
                AZStd::lock_guard<AZStd::shared_mutex> lock(m_handleLock); // take exclusive write lock as we may move CaptureState locations in memory
                uint32_t captureIndex = aznumeric_cast<uint32_t>(m_allCaptures.size());
                m_allCaptures.emplace_back(captureIndex);
                return CaptureHandle(this, captureIndex);
            }
        }

        void FrameCaptureSystemComponent::Deactivate()
        {
            FrameCaptureRequestBus::Handler::BusDisconnect();
            FrameCaptureTestRequestBus::Handler::BusDisconnect();
            SystemTickBus::Handler::BusDisconnect();
            m_idleCaptures.clear();
            m_inProgressCaptures.clear();
            m_allCaptures.clear();
        }

        AZStd::string FrameCaptureSystemComponent::ResolvePath(const AZStd::string& filePath)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
            char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
            fileIO->ResolvePath(filePath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);

            return AZStd::string(resolvedPath);
        }

        bool FrameCaptureSystemComponent::CanCapture() const
        {
            return !AZ::RHI::IsNullRHI();
        }

        AZ::Outcome<FrameCaptureSystemComponent::CaptureHandle, FrameCaptureError> FrameCaptureSystemComponent::ScreenshotPreparation(
            const AZStd::string& imagePath,
            AZ::RPI::AttachmentReadback::CallbackFunction callbackFunction)
        {
            FrameCaptureError error;

            if (!CanCapture())
            {
                error.m_errorMessage = "Frame capture not availble.";
                return AZ::Failure(error);
            }

            if (imagePath.empty() && callbackFunction == nullptr)
            {
                error.m_errorMessage = "No callback or image path is set. No result will be generated.";
                return AZ::Failure(error);
            }

            AZ_Warning(
                "FrameCaptureSystemComponent",
                imagePath.empty() || callbackFunction == nullptr,
                "Callback and image path are both set. Image path will be ignored.");

            CaptureHandle captureHandle = InitCapture();
            if (captureHandle.IsNull())
            {
                error.m_errorMessage = "Failed to allocate a capture.";
                return AZ::Failure(error);
            }

            AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
            CaptureState* capture = captureHandle.GetCaptureState();
            if (!capture) // failed to get the capture state ptr, abort
            {
                error.m_errorMessage = "Failed to get the captureState.";
                m_idleCaptures.push_back(captureHandle);
                return AZ::Failure(error);
            }

            if (!capture->m_readback->IsReady())
            {
                error.m_errorMessage = "Failed to capture attachment since the readback is not ready.";
                m_idleCaptures.push_back(captureHandle);
                return AZ::Failure(error);
            }

            capture->m_readback->SetUserIdentifier(captureHandle.GetCaptureStateIndex());
            if (callbackFunction != nullptr)
            {
                capture->m_readback->SetCallback(callbackFunction);
            }
            else
            {
                capture->m_readback->SetCallback(
                    AZStd::bind(&FrameCaptureSystemComponent::CaptureAttachmentCallback, this, AZStd::placeholders::_1));

                AZ_Assert(!imagePath.empty(), "The image path must be provided if the callback is not assigned.");
                capture->m_outputFilePath = ResolvePath(imagePath);
            }

            return AZ::Success(captureHandle);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::CaptureScreenshotForWindow(const AZStd::string& filePath, AzFramework::NativeWindowHandle windowHandle)
        {
            return InternalCaptureScreenshot(filePath, windowHandle);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::CaptureScreenshot(const AZStd::string& filePath)
        {
            FrameCaptureError error;

            AzFramework::NativeWindowHandle windowHandle = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext()->GetWindowHandle();

            return InternalCaptureScreenshot(filePath, windowHandle);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::CaptureScreenshotWithPreview(const AZStd::string& outputFilePath)
        {
            FrameCaptureError error;

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassClass<RPI::ImageAttachmentPreviewPass>();
            AZ::RPI::ImageAttachmentPreviewPass* previewPass = nullptr;
            AZ::RPI::PassSystemInterface::Get()->ForEachPass(
                passFilter,
                [&previewPass](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                {
                    if (pass->GetParent() != nullptr && pass->IsEnabled())
                    {
                        previewPass = azrtti_cast<AZ::RPI::ImageAttachmentPreviewPass*>(pass);
                        return AZ::RPI::PassFilterExecutionFlow::StopVisitingPasses;
                    }
                    return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });

            if (!previewPass)
            {
                error.m_errorMessage = "Failed to find an ImageAttachmentPreviewPass.";
                return AZ::Failure(error);
            }

            auto prepOutcome = ScreenshotPreparation(outputFilePath, nullptr);
            if (!prepOutcome.IsSuccess())
            {
                return AZ::Failure(prepOutcome.TakeError());
            }
            CaptureHandle captureHandle = prepOutcome.GetValue();
            AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
            CaptureState* captureState = captureHandle.GetCaptureState();

            if (!previewPass->ReadbackOutput(captureState->m_readback))
            {
                error.m_errorMessage = "Failed to readback output from the ImageAttachmentPreviewPass";
                m_idleCaptures.push_back(captureHandle);
                return AZ::Failure(error);
            }

            m_inProgressCaptures.push_back(captureHandle);

            FrameCaptureId frameId = captureHandle.GetCaptureStateIndex();
            return AZ::Success(frameId);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::InternalCaptureScreenshot(
            const AZStd::string& imagePath, AzFramework::NativeWindowHandle windowHandle)
        {
            FrameCaptureError error;

            if (!windowHandle)
            {
                error.m_errorMessage = "No valid window for the capture.";
                return AZ::Failure(error); 
            }

            // Find SwapChainPass for the window handle
            RPI::SwapChainPass* pass = AZ::RPI::PassSystemInterface::Get()->FindSwapChainPass(windowHandle);
            if (!pass)
            {
                error.m_errorMessage = "Failed to find SwapChainPass for the window.";
                return AZ::Failure(error);
            }

            auto prepOutcome = ScreenshotPreparation(imagePath, nullptr);
            if (!prepOutcome.IsSuccess())
            {
                return AZ::Failure(prepOutcome.GetError());
            }
            CaptureHandle captureHandle = prepOutcome.GetValue();
            AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
            CaptureState* captureState = captureHandle.GetCaptureState();
            AZ_Assert(captureState, "ScreenshotPreparation should have created a ready capture state "
                "if the capture handle is valid.");

            pass->ReadbackSwapChain(captureState->m_readback);

            m_inProgressCaptures.push_back(captureHandle);

            FrameCaptureId frameId = captureHandle.GetCaptureStateIndex();
            return AZ::Success(frameId);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::InternalCapturePassAttachment(
            const AZStd::string& outputFilePath,
            AZ::RPI::AttachmentReadback::CallbackFunction callbackFunction,
            const AZStd::vector<AZStd::string>& passHierarchy,
            const AZStd::string& slot,
            RPI::PassAttachmentReadbackOption option)
        {
            FrameCaptureError error;

            if (passHierarchy.size() == 0)
            {
                error.m_errorMessage = "Empty data in passHierarchy.";
                return AZ::Failure(error);
            }

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassHierarchy(passHierarchy);
            RPI::Pass* pass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (!pass)
            {
                error.m_errorMessage = AZStd::string::format("Failed to find pass from %s", passHierarchy[0].c_str());
                return AZ::Failure(error);
            }

           auto prepOutcome = ScreenshotPreparation(outputFilePath, callbackFunction);
            if (!prepOutcome.IsSuccess())
            {
                return AZ::Failure(prepOutcome.GetError());
            }
            CaptureHandle captureHandle = prepOutcome.GetValue();
            AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
            CaptureState* captureState = captureHandle.GetCaptureState();
            AZ_Assert(captureState, "ScreenshotPreparation should have created a ready capture state "
                "if the capture handle is valid.");

            if (!pass->ReadbackAttachment(captureState->m_readback, captureHandle.GetCaptureStateIndex(), Name(slot), option))
            {
                error.m_errorMessage = AZStd::string::format(
                    "Failed to readback the attachment bound to pass [%s] slot [%s]", pass->GetName().GetCStr(), slot.c_str());
                m_idleCaptures.push_back(captureHandle);
                return AZ::Failure(error);
            }

            m_inProgressCaptures.push_back(captureHandle);

            FrameCaptureId frameId = captureHandle.GetCaptureStateIndex();
            return AZ::Success(frameId);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::CapturePassAttachment(
            const AZStd::string& imagePath,
            const AZStd::vector<AZStd::string>& passHierarchy,
            const AZStd::string& slot,
            RPI::PassAttachmentReadbackOption option)
        {
            return InternalCapturePassAttachment(
                imagePath,
                nullptr,
                passHierarchy,
                slot,
                option);
        }

        FrameCaptureOutcome FrameCaptureSystemComponent::CapturePassAttachmentWithCallback(
            RPI::AttachmentReadback::CallbackFunction callback,
            const AZStd::vector<AZStd::string>& passHierarchy,
            const AZStd::string& slotName,
            RPI::PassAttachmentReadbackOption option)
        {
            auto captureCallback = [this, callback](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
            {
                CaptureHandle captureHandle(this, readbackResult.m_userIdentifier);

                callback(readbackResult); // call user supplied callback function

                AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
                CaptureState* captureState = captureHandle.GetCaptureState();
                AZ_Assert(captureState && captureState->m_result == FrameCaptureResult::None, "Unexpected value for m_result");
                captureState->m_result = FrameCaptureResult::Success; // just need to mark this capture as complete, callback handles the actual processing
            };

            return InternalCapturePassAttachment("", captureCallback, passHierarchy, slotName, option);
        }

        void FrameCaptureSystemComponent::OnSystemTick()
        {
            // inProgressCaptures is in capture submit order, loop over the captures until we find an unfinished one.
            // This ensures that OnCaptureFinished is signalled in submission order
            while (m_inProgressCaptures.size())
            {
                CaptureHandle captureHandle(m_inProgressCaptures.front());
                if (captureHandle.IsNull())
                {
                    // if we find a null handle, remove it from the list
                    m_inProgressCaptures.pop_front();
                    continue;
                }
                AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
                CaptureState* capture = captureHandle.GetCaptureState();
                if (capture->m_result == FrameCaptureResult::None)
                {
                    break;
                }
                FrameCaptureNotificationBus::Event(captureHandle.GetCaptureStateIndex(), &FrameCaptureNotificationBus::Events::OnFrameCaptureFinished, capture->m_result, capture->m_latestCaptureInfo.c_str());
                m_inProgressCaptures.pop_front();
                m_idleCaptures.push_back(captureHandle);
            }
        }

        void FrameCaptureSystemComponent::CaptureAttachmentCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
        {
            CaptureHandle captureHandle(this, readbackResult.m_userIdentifier);
            AZStd::scoped_lock<CaptureHandle> scope_lock(captureHandle);
            CaptureState* capture = captureHandle.GetCaptureState();
            AZ_Assert(capture && capture->m_result == FrameCaptureResult::None, "Unexpected value for m_result");

            capture->m_latestCaptureInfo = capture->m_outputFilePath;
            if (readbackResult.m_state == AZ::RPI::AttachmentReadback::ReadbackState::Success)
            {
                if (readbackResult.m_attachmentType == AZ::RHI::AttachmentType::Buffer)
                {
                    // write buffer data to the data file
                    AZ::IO::FileIOStream fileStream(capture->m_outputFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
                    if (fileStream.IsOpen())
                    {
                        fileStream.Write(readbackResult.m_dataBuffer->size(), readbackResult.m_dataBuffer->data());
                        capture->m_result = FrameCaptureResult::Success;
                    }
                    else
                    {
                        capture->m_latestCaptureInfo = AZStd::string::format("Failed to open file %s for writing", capture->m_outputFilePath.c_str());
                        capture->m_result = FrameCaptureResult::FileWriteError;
                    }
                }
                else if (readbackResult.m_attachmentType == AZ::RHI::AttachmentType::Image)
                {
                    AZStd::string extension;
                    AzFramework::StringFunc::Path::GetExtension(capture->m_outputFilePath.c_str(), extension, false);
                    AZStd::to_lower(extension.begin(), extension.end());

                    if (extension == "ppm")
                    {
                        if (readbackResult.m_imageDescriptor.m_format == RHI::Format::R8G8B8A8_UNORM ||
                            readbackResult.m_imageDescriptor.m_format == RHI::Format::B8G8R8A8_UNORM)
                        {
                            const auto ppmFrameCapture = PpmFrameCaptureOutput(capture->m_outputFilePath, readbackResult);
                            capture->m_result = ppmFrameCapture.m_result;
                            capture->m_latestCaptureInfo = ppmFrameCapture.m_errorMessage.value_or("");
                        }
                        else
                        {
                            capture->m_latestCaptureInfo = AZStd::string::format(
                                "Can't save image with format %s to a ppm file", RHI::ToString(readbackResult.m_imageDescriptor.m_format));
                            capture->m_result = FrameCaptureResult::UnsupportedFormat;
                        }
                    }
                    else if (extension == "dds")
                    {
                        const auto ddsFrameCapture = DdsFrameCaptureOutput(capture->m_outputFilePath, readbackResult);
                        capture->m_result = ddsFrameCapture.m_result;
                        capture->m_latestCaptureInfo = ddsFrameCapture.m_errorMessage.value_or("");
                    }
                    else if (extension == "tiff" || extension == "tif")
                    {
                        const auto tifFrameCapture = TiffFrameCaptureOutput(capture->m_outputFilePath, readbackResult);
                        capture->m_result = tifFrameCapture.m_result;
                        capture->m_latestCaptureInfo = tifFrameCapture.m_errorMessage.value_or("");
                    }
                    else if (extension == "png")
                    {
                        if (readbackResult.m_imageDescriptor.m_format == RHI::Format::R8G8B8A8_UNORM ||
                            readbackResult.m_imageDescriptor.m_format == RHI::Format::B8G8R8A8_UNORM)
                        {
                            AZStd::string folderPath;
                            AzFramework::StringFunc::Path::GetFolderPath(capture->m_outputFilePath.c_str(), folderPath);
                            AZ::IO::SystemFile::CreateDir(folderPath.c_str());

                            const auto frameCaptureResult = PngFrameCaptureOutput(capture->m_outputFilePath, readbackResult);
                            capture->m_result = frameCaptureResult.m_result;
                            capture->m_latestCaptureInfo = frameCaptureResult.m_errorMessage.value_or("");
                        }
                        else
                        {
                            capture->m_latestCaptureInfo = AZStd::string::format(
                                "Can't save image with format %s to a png file", RHI::ToString(readbackResult.m_imageDescriptor.m_format));
                            capture->m_result = FrameCaptureResult::UnsupportedFormat;
                        }
                    }
                    else
                    {
                        capture->m_latestCaptureInfo = AZStd::string::format("Only supports saving image to ppm or dds files");
                        capture->m_result = FrameCaptureResult::InvalidArgument;
                    }
                }
            }
            else
            {
                capture->m_latestCaptureInfo = AZStd::string::format("Failed to read back attachment [%s]", readbackResult.m_name.GetCStr());
                capture->m_result = FrameCaptureResult::InternalError;
            }


            if (capture->m_result == FrameCaptureResult::Success)
            {
                // Normalize the path so the slashes will be in the right direction for the local platform allowing easy copy/paste into file browsers.
                AZStd::string normalizedPath = capture->m_outputFilePath;
                AzFramework::StringFunc::Path::Normalize(normalizedPath);
                AZ_Printf("FrameCaptureSystemComponent", "Attachment [%s] was saved to file %s\n", readbackResult.m_name.GetCStr(), normalizedPath.c_str());
            }
            else
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "%s", capture->m_latestCaptureInfo.c_str());
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // CaptureHandle implementation
        FrameCaptureSystemComponent::CaptureHandle::CaptureHandle(FrameCaptureSystemComponent* frameCaptureSystemComponent, uint32_t captureStateIndex)
        : m_frameCaptureSystemComponent(frameCaptureSystemComponent)
        , m_captureStateIndex(captureStateIndex)
        {
        }

        FrameCaptureSystemComponent::CaptureHandle FrameCaptureSystemComponent::CaptureHandle::Null()
        {
            return CaptureHandle(nullptr, InvalidCaptureHandle);
        }

        void FrameCaptureSystemComponent::CaptureHandle::lock()
        {
            AZ_Assert(IsValid() && m_frameCaptureSystemComponent != nullptr, "FrameCaptureSystemComponent attempting to lock an invalid handle");

            m_frameCaptureSystemComponent->m_handleLock.lock_shared();
        }

        void FrameCaptureSystemComponent::CaptureHandle::unlock()
        {
            AZ_Assert(IsValid() && m_frameCaptureSystemComponent != nullptr, "FrameCaptureSystemComponent attempting to unlock an invalid handle");

            m_frameCaptureSystemComponent->m_handleLock.unlock_shared();
        }

        FrameCaptureSystemComponent::CaptureState* FrameCaptureSystemComponent::CaptureHandle::GetCaptureState()
        {
            AZ_Assert(IsValid() && m_frameCaptureSystemComponent != nullptr, "FrameCaptureSystemComponent GetCaptureState called on an invalid handle");
            if (IsNull() || m_frameCaptureSystemComponent == nullptr)
            {
                return nullptr;
            }
            // Ideally we could check the state of the handle lock here to check that a shared lock is being held.
            // Nearest available check is can we try an exclusive lock, 
            // this will also fail if someone else is holding the exclusive lock though.
            if(m_frameCaptureSystemComponent->m_handleLock.try_lock())
            {
                AZ_Assert(false, "FrameCaptureSystemComponent::CaptureHandle::GetCaptureState called without holding a read lock");
                m_frameCaptureSystemComponent->m_handleLock.unlock();
                return nullptr;
            }


            size_t captureIdx = aznumeric_cast<size_t>(m_captureStateIndex);
            if (captureIdx < m_frameCaptureSystemComponent->m_allCaptures.size())
            {
                return &m_frameCaptureSystemComponent->m_allCaptures[captureIdx];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // CaptureState implementation
        FrameCaptureSystemComponent::CaptureState::CaptureState(uint32_t captureIndex)
        {
            AZStd::fixed_string<128> scope_name = AZStd::fixed_string<128>::format("FrameCapture_%d", captureIndex);
            m_readback = AZStd::make_shared<AZ::RPI::AttachmentReadback>(AZ::RHI::ScopeId{ scope_name });
            AZ_Assert(m_readback, "Failed to allocate an AttachmentReadback for the capture state");
        }

        FrameCaptureSystemComponent::CaptureState::CaptureState(CaptureState&& other)
        : m_readback(AZStd::move(other.m_readback))
        , m_outputFilePath(AZStd::move(other.m_outputFilePath))
        , m_latestCaptureInfo(AZStd::move(other.m_latestCaptureInfo))

        {
            // atomic doesn't support move or copy construction, or direct assignment.
            // This function is only used during m_allCaptures resize due to CaptureState addition
            // and the m_handleLock is exclusively locked during that operation.
            // Manually copy the atomic value to work around the other issues.
            FrameCaptureResult result = other.m_result;
            m_result = result; 
        }

        void FrameCaptureSystemComponent::CaptureState::Reset()
        {
            //m_readback->Reset();
            m_outputFilePath.clear();
            m_latestCaptureInfo.clear();
            m_result = FrameCaptureResult::None;
        }

        void FrameCaptureSystemComponent::SetScreenshotFolder(const AZStd::string& screenshotFolder)
        {
            m_screenshotFolder = ResolvePath(screenshotFolder);
        }

        void FrameCaptureSystemComponent::SetTestEnvPath(const AZStd::string& envPath)
        {
            m_testEnvPath = envPath;
        }

        void FrameCaptureSystemComponent::SetOfficialBaselineImageFolder(const AZStd::string& baselineFolder)
        {
            m_officialBaselineImageFolder = ResolvePath(baselineFolder);
        }

        void FrameCaptureSystemComponent::SetLocalBaselineImageFolder(const AZStd::string& baselineFolder)
        {
            m_localBaselineImageFolder = ResolvePath(baselineFolder);
        }

        FrameCapturePathOutcome FrameCaptureSystemComponent::BuildScreenshotFilePath(const AZStd::string& imageName, bool useEnvPath)
        {
            AZStd::string imagePath = useEnvPath
                ? ResolvePath(AZStd::string::format("%s/%s/%s", m_screenshotFolder.c_str(), m_testEnvPath.c_str(), imageName.c_str()))
                : ResolvePath(AZStd::string::format("%s/%s", m_screenshotFolder.c_str(), imageName.c_str()));

            if (imagePath.size())
            {
                return AZ::Success(imagePath);
            }
            else
            {
                FrameCaptureTestError error;
                error.m_errorMessage = "Failed to build image path.";
                return AZ::Failure(error);
            }
        }

        FrameCapturePathOutcome FrameCaptureSystemComponent::BuildOfficialBaselineFilePath(const AZStd::string& imageName, bool useEnvPath)
        {
            AZStd::string imagePath = useEnvPath
                ? ResolvePath(AZStd::string::format("%s/%s/%s", m_officialBaselineImageFolder.c_str(), m_testEnvPath.c_str(), imageName.c_str()))
                : ResolvePath(AZStd::string::format("%s/%s", m_officialBaselineImageFolder.c_str(), imageName.c_str()));

            if (imagePath.size())
            {
                return AZ::Success(imagePath);
            }
            else
            {
                FrameCaptureTestError error;
                error.m_errorMessage = "Failed to build image path.";
                return AZ::Failure(error);
            }
        }

        FrameCapturePathOutcome FrameCaptureSystemComponent::BuildLocalBaselineFilePath(const AZStd::string& imageName, bool useEnvPath)
        {
            AZStd::string imagePath = useEnvPath
                ? ResolvePath(AZStd::string::format("%s/%s/%s", m_localBaselineImageFolder.c_str(), m_testEnvPath.c_str(), imageName.c_str()))
                : ResolvePath(AZStd::string::format("%s/%s", m_localBaselineImageFolder.c_str(), imageName.c_str()));

            if (imagePath.size())
            {
                return AZ::Success(imagePath);
            }
            else
            {
                FrameCaptureTestError error;
                error.m_errorMessage = "Failed to build image path.";
                return AZ::Failure(error);
            }
        }

        FrameCaptureComparisonOutcome FrameCaptureSystemComponent::CompareScreenshots(
            const AZStd::string& filePathA, const AZStd::string& filePathB, float minDiffFilter)
        {
            FrameCaptureTestError error;

            char resolvedFilePathA[AZ_MAX_PATH_LEN] = { 0 };
            char resolvedFilePathB[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(filePathA.c_str(), resolvedFilePathA, AZ_MAX_PATH_LEN);
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(filePathB.c_str(), resolvedFilePathB, AZ_MAX_PATH_LEN);

            if (!filePathA.ends_with(".png") || !filePathB.ends_with(".png"))
            {
                error.m_errorMessage = "Image comparison only supports png files for now.";
                return AZ::Failure(error);
            }

            // Load image A
            Utils::PngFile imageA = Utils::PngFile::Load(resolvedFilePathA);

            if (!imageA.IsValid())
            {
                error.m_errorMessage = AZStd::string::format("Failed to load image file: %s.", resolvedFilePathA);
                return AZ::Failure(error);
            }
            else if (imageA.GetBufferFormat() != Utils::PngFile::Format::RGBA)
            {
                error.m_errorMessage = AZStd::string::format("Image comparison only supports 8-bit RGBA png. %s is not.", resolvedFilePathA);
                return AZ::Failure(error);
            }

            // Load image B
            Utils::PngFile imageB = Utils::PngFile::Load(resolvedFilePathB);

            if (!imageB.IsValid())
            {
                error.m_errorMessage = AZStd::string::format("Failed to load image file: %s.", resolvedFilePathB);
                return AZ::Failure(error);
            }
            else if (imageA.GetBufferFormat() != Utils::PngFile::Format::RGBA)
            {
                error.m_errorMessage = AZStd::string::format("Image comparison only supports 8-bit RGBA png. %s is not.", resolvedFilePathB);
                return AZ::Failure(error);
            }

            // Compare
            auto compOutcome = Utils::CalcImageDiffRms(
                imageA.GetBuffer(), RHI::Size(imageA.GetWidth(), imageA.GetHeight(), 1), AZ::RHI::Format::R8G8B8A8_UNORM,
                imageB.GetBuffer(), RHI::Size(imageB.GetWidth(), imageB.GetHeight(), 1), AZ::RHI::Format::R8G8B8A8_UNORM,
                minDiffFilter
            );

            if (!compOutcome.IsSuccess())
            {
                error.m_errorMessage = compOutcome.GetError().m_errorMessage;
                return AZ::Failure(error);
            }

            return AZ::Success(compOutcome.TakeValue());
        }
    }
}
