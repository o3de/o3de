/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <ScriptAutomationScriptBindings.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Debug/ProfilerBus.h>

#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <Utils/Utils.h>

namespace ScriptAutomation
{
    namespace Bindings
    {
        void Print(const AZStd::string& message [[maybe_unused]])
        {
#ifndef _RELEASE //AZ_TracePrintf does nothing in release, ignore this call
            auto func = [message]()
            {
                AZ_TracePrintf("ScriptAutomation", "Script: %s\n", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
#endif
        }

        void Warning(const AZStd::string& message [[maybe_unused]])
        {
#ifndef _RELEASE //AZ_Warning does nothing in release, ignore this call
            auto func = [message]()
            {
                AZ_Warning("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
#endif
        }

        void Error(const AZStd::string& message [[maybe_unused]])
        {
#ifndef _RELEASE //AZ_Error does nothing in release, ignore this call
            auto func = [message]()
            {
                AZ_Error("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
#endif
        }

        void ExecuteConsoleCommand(const AZStd::string& command)
        {
            auto func = [command]()
            {
                AzFramework::ConsoleRequestBus::Broadcast(
                    &AzFramework::ConsoleRequests::ExecuteConsoleCommand, command.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleFrames(int numFrames)
        {
            auto func = [numFrames]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetIdleFrames(numFrames);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleSeconds(float numSeconds)
        {
            auto func = [numSeconds]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetIdleSeconds(numSeconds);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void ResizeViewport(int width, int height)
        {
            auto operation = [width,height]()
            {
                if (Utils::SupportsResizeClientArea())
                {
                    Utils::ResizeClientArea(width, height);
                }
                else
                {
                    AZ_Error("ScriptAutomation", false, "ResizeViewport() is not supported on this platform");
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CapturePassTimestamp(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetCapturePending(true);
                automationComponent->AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
                ScriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CapturePassTimestamp, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }
        void CaptureCpuFrameTime(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetCapturePending(true);
                automationComponent->AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
                ScriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureCpuFrameTime, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CapturePassPipelineStatistics(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetCapturePending(true);
                automationComponent->AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
                ScriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CapturePassPipelineStatistics, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetCapturePending(true);
                automationComponent->AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
                ScriptAutomationInterface->PauseAutomation();

                if (auto profilerSystem = AZ::Debug::ProfilerSystemInterface::Get(); profilerSystem)
                {
                    profilerSystem->CaptureFrame(outputFilePath);
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureBenchmarkMetadata(const AZStd::string& benchmarkName, const AZStd::string& outputFilePath)
        {
            auto operation = [benchmarkName, outputFilePath]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->SetCapturePending(true);
                automationComponent->AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
                ScriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureBenchmarkMetadata, benchmarkName, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        AZStd::string ResolvePath(const AZStd::string& path)
        {
            return Utils::ResolvePath(path);
        }

        AZStd::string NormalizePath(const AZStd::string& path)
        {
            AZStd::string normalizedPath = path;
            AzFramework::StringFunc::Path::Normalize(normalizedPath);
            return normalizedPath;
        }

        float DegToRad(float degrees)
        {
            return AZ::DegToRad(degrees);
        }

        AZStd::string GetRenderApiName()
        {
            AZ::RPI::RPISystemInterface* rpiSystem = AZ::RPI::RPISystemInterface::Get();
            return rpiSystem->GetRenderApiName().GetCStr();
        }

        AZStd::string GetScreenshotOutputPath(bool normalized)
        {
            return Utils::GetScreenshotsPath(normalized);
        }

        AZStd::string GetProfilingOutputPath(bool normalized)
        {
            return Utils::GetProfilingPath(normalized);
        }

        bool PrepareForScreenCapture(const AZStd::string& path)
        {
            if (!Utils::IsFileUnderFolder(Utils::ResolvePath(path), Utils::GetScreenshotsPath(true)))
            {
                // The main reason we require screenshots to be in a specific folder is to ensure we don't delete or replace some other important file.
                AZ_Error("ScriptAutomation", false, "Screenshots must be captured under the '%s' folder. Attempted to save screenshot to '%s'.",
                    Utils::GetScreenshotsPath(false).c_str(), path.c_str());

                return false;
            }

            // Delete the file if it already exists
            if (AZ::IO::LocalFileIO::GetInstance()->Exists(path.c_str()) && !AZ::IO::LocalFileIO::GetInstance()->Remove(path.c_str()))
            {
                AZ_Error("ScriptAutomation", false, "Failed to delete existing screenshot file '%s'.", path.c_str());
                return false;
            }

            auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
            auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

            automationComponent->SetCapturePending(true);
            ScriptAutomationInterface->PauseAutomation();

            return true;
        }

        void CaptureScreenshot(const AZStd::string& filePath)
        {
            auto operation = [filePath]()
            {
                // Note this will pause the script until the capture is complete
                if (PrepareForScreenCapture(filePath))
                {
                    auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                    auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                    uint32_t frameCaptureId = AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(frameCaptureId, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot, filePath);
                    if (frameCaptureId != AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId)
                    {
                        automationComponent->SetFrameCaptureId(frameCaptureId);
                        automationComponent->AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect();
                    }
                    else
                    {
                        AZ_Error("ScriptAutomation", false, "Script: failed to trigger screenshot capture");
                       ScriptAutomationInterface->ResumeAutomation();
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureScreenshotWithPreview(const AZStd::string& filePath)
        {
            auto operation = [filePath]()
            {
                // Note this will pause the script until the capture is complete
                if (PrepareForScreenCapture(filePath))
                {
                    auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                    auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                    uint32_t frameCaptureId = AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(frameCaptureId, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshotWithPreview, filePath);
                    if (frameCaptureId != AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId)
                    {
                        automationComponent->SetFrameCaptureId(frameCaptureId);
                        automationComponent->AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect();
                    }
                    else
                    {
                        AZ_Error("ScriptAutomation", false, "Script: failed to trigger screenshot capture with preview");
                       ScriptAutomationInterface->ResumeAutomation();
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CapturePassAttachment(AZ::ScriptDataContext& dc)
        {
            // manually parse args as this takes a lua table as an arg
            if (dc.GetNumArguments() != 3 && dc.GetNumArguments() != 4)
            {
                 AZ_Error("ScriptAutomation", false, "Script: CapturePassAttachment needs three or four arguments");
                return;
            }

            if (!dc.IsTable(0))
            {
                AZ_Error("ScriptAutomation", false, "Script: CapturePassAttachment's first argument must be a table of strings");
                return;
            }

            if (!dc.IsString(1) || !dc.IsString(2))
            {
                AZ_Error("ScriptAutomation", false, "Script: CapturePassAttachment's second and third argument must be strings");
                return;
            }

            if (dc.GetNumArguments() == 4 && !dc.IsString(3))
            {
                AZ_Error("ScriptAutomation", false, "Script: CapturePassAttachment's forth argument must be a string 'Input' or 'Output'");
                return;
            }

            const char* stringValue = nullptr;

            AZStd::vector<AZStd::string> passHierarchy;
            AZStd::string slot;
            AZStd::string outputFilePath;

            // read slot name and output file path
            dc.ReadArg(1, stringValue);
            slot = AZStd::string(stringValue);
            dc.ReadArg(2, stringValue);
            outputFilePath = AZStd::string(stringValue);

            AZ::RPI::PassAttachmentReadbackOption readbackOption = AZ::RPI::PassAttachmentReadbackOption::Output;
            if (dc.GetNumArguments() == 4)
            {
                AZStd::string option;
                dc.ReadArg(3, option);
                if (option == "Input")
                {
                    readbackOption = AZ::RPI::PassAttachmentReadbackOption::Input;
                }
            }

            // read pass hierarchy
            AZ::ScriptDataContext stringtable;
            dc.InspectTable(0, stringtable);

            const char* fieldName;
            int fieldIndex;
            int elementIndex;

            while (stringtable.InspectNextElement(elementIndex, fieldName, fieldIndex))
            {
                if (fieldIndex != -1)
                {
                    if (!stringtable.IsString(elementIndex))
                    {
                        AZ_Error("ScriptAutomation", false, "Script: CapturePassAttachment's first argument must contain only strings");
                        return;
                    }

                    const char* stringTableValue = nullptr;
                    if (stringtable.ReadValue(elementIndex, stringTableValue))
                    {
                        passHierarchy.push_back(stringTableValue);
                    }
                }
            }

            auto operation = [passHierarchy, slot, outputFilePath, readbackOption]()
            {
                // Note this will pause the script until the capture is complete
                if (PrepareForScreenCapture(outputFilePath))
                {
                    auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                    auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                    uint32_t frameCaptureId = AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(frameCaptureId, &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment, passHierarchy, slot, outputFilePath, readbackOption);
                    if (frameCaptureId != AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId)
                    {
                        automationComponent->SetFrameCaptureId(frameCaptureId);
                        automationComponent->AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect();
                    }
                    else
                    {
                        AZ_Error("ScriptAutomation", false, "Script: failed to trigger screenshot capture of pass");
                       ScriptAutomationInterface->ResumeAutomation();
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }
    } // namespace Bindings

    void ReflectScriptBindings(AZ::BehaviorContext* behaviorContext)
    {
        AZ::MathReflect(behaviorContext);

        behaviorContext->Method("Print", &Bindings::Print);
        behaviorContext->Method("Warning", &Bindings::Warning);
        behaviorContext->Method("Error", &Bindings::Error);

        behaviorContext->Method("ExecuteConsoleCommand", &Bindings::ExecuteConsoleCommand);

        behaviorContext->Method("IdleFrames", &Bindings::IdleFrames);
        behaviorContext->Method("IdleSeconds", &Bindings::IdleSeconds);
        behaviorContext->Method("ResizeViewport", &Bindings::ResizeViewport);

        behaviorContext->Method("ResolvePath", &Bindings::ResolvePath);
        behaviorContext->Method("NormalizePath", &Bindings::NormalizePath);
        behaviorContext->Method("DegToRad", &Bindings::DegToRad);
        behaviorContext->Method("GetRenderApiName", &Bindings::GetRenderApiName);
        behaviorContext->Method("GetScreenshotOutputPath", &Bindings::GetScreenshotOutputPath);
        behaviorContext->Method("GetProfilingOutputPath", &Bindings::GetProfilingOutputPath);

        // Screenshots...
        behaviorContext->Method("CaptureScreenshot", &Bindings::CaptureScreenshot);
        behaviorContext->Method("CaptureScreenshotWithPreview", &Bindings::CaptureScreenshotWithPreview);
        behaviorContext->Method("CapturePassAttachment", &Bindings::CapturePassAttachment);

        // Profiling data...
        behaviorContext->Method("CapturePassTimestamp", &Bindings::CapturePassTimestamp);
        behaviorContext->Method("CaptureCpuFrameTime", &Bindings::CaptureCpuFrameTime);
        behaviorContext->Method("CapturePassPipelineStatistics", &Bindings::CapturePassPipelineStatistics);
        behaviorContext->Method("CaptureCpuProfilingStatistics", &Bindings::CaptureCpuProfilingStatistics);
        behaviorContext->Method("CaptureBenchmarkMetadata", &Bindings::CaptureBenchmarkMetadata);
    }
} // namespace ScriptAutomation
