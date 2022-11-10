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
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/optional.h>
#include <AzCore/IO/Path/Path.h>

#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

namespace ScriptAutomation
{
    namespace Utils
    {
        bool SupportsResizeClientAreaOfDefaultWindow()
        {
            return AzFramework::NativeWindow::SupportsClientAreaResizeOfDefaultWindow();
        }

        void ResizeClientArea(uint32_t width, uint32_t height, const AzFramework::WindowPosOptions& options)
        {
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            AzFramework::WindowSize clientAreaSize = {width, height};
            AzFramework::WindowRequestBus::Event(windowHandle, &AzFramework::WindowRequestBus::Events::ResizeClientArea, clientAreaSize, options);
            AzFramework::WindowSize newWindowSize;
            AzFramework::WindowRequestBus::EventResult(newWindowSize, windowHandle, &AzFramework::WindowRequests::GetClientAreaSize);
            AZ_Error("ResizeClientArea", newWindowSize.m_width == width && newWindowSize.m_height == height,
                "Requested window resize to %ux%u but got %ux%u. This display resolution is too low or desktop scaling is too high.",
                width, height, newWindowSize.m_width, newWindowSize.m_height);
        }

        bool SupportsToggleFullScreenOfDefaultWindow()
        {
            return AzFramework::NativeWindow::CanToggleFullScreenStateOfDefaultWindow();
        }

        void ToggleFullScreenOfDefaultWindow()
        {
            AzFramework::NativeWindow::ToggleFullScreenStateOfDefaultWindow();
        }

        AZ::IO::FixedMaxPath GetProfilingPath(bool resolvePath)
        {
            AZ::IO::FixedMaxPath path("@user@");
            if (resolvePath)
            {
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    path.clear();
                    settingsRegistry->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath);
                }
            }
            path /= "scriptautomation/profiling";

            return path.LexicallyNormal();
        }

        AZ::IO::FixedMaxPath ResolvePath(const AZ::IO::PathView path)
        {
            AZStd::optional<AZ::IO::FixedMaxPath> resolvedPath = AZ::IO::FileIOBase::GetInstance()->ResolvePath(path);
            return resolvedPath ? resolvedPath.value() : AZ::IO::FixedMaxPath{};
        }
    } // namespace Utils

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
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->SetIdleFrames(numFrames);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleSeconds(float numSeconds)
        {
            auto func = [numSeconds]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->SetIdleSeconds(numSeconds);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void ResizeViewport(int width, int height)
        {
            auto operation = [width,height]()
            {
                if (Utils::SupportsResizeClientAreaOfDefaultWindow())
                {
                    AzFramework::WindowPosOptions options;
                    options.m_ignoreScreenSizeLimit = true;
                    Utils::ResizeClientArea(width, height, options);
                }
                else
                {
                    AZ_Error("ScriptAutomation", false, "ResizeClientArea() is not supported on this platform");
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CapturePassTimestamp(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CapturePassTimestamp, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }
        void CaptureCpuFrameTime(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureCpuFrameTime, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CapturePassPipelineStatistics(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CapturePassPipelineStatistics, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath)
        {
            auto operation = [outputFilePath]()
            {

                if (auto profilerSystem = AZ::Debug::ProfilerSystemInterface::Get(); profilerSystem)
                {
                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                    scriptAutomationInterface->StartProfilingCapture();
                    scriptAutomationInterface->PauseAutomation();
                    
                    profilerSystem->CaptureFrame(outputFilePath);
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureBenchmarkMetadata(const AZStd::string& benchmarkName, const AZStd::string& outputFilePath)
        {
            auto operation = [benchmarkName, outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureBenchmarkMetadata, benchmarkName, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        AZStd::string ResolvePath(const AZStd::string& path)
        {
            return Utils::ResolvePath(AZ::IO::PathView(path)).String();
        }

        AZStd::string GetRenderApiName()
        {
            AZ::RPI::RPISystemInterface* rpiSystem = AZ::RPI::RPISystemInterface::Get();
            return rpiSystem->GetRenderApiName().GetCStr();
        }

        AZStd::string GetProfilingOutputPath(bool normalized)
        {
            return Utils::GetProfilingPath(normalized).String();
        }

        bool PrepareForScreenCapture(const AZStd::string& imageName)
        {
            AZStd::string fullFilePath;
            AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                fullFilePath, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

            if (!AZ::IO::PathView(fullFilePath).IsRelativeTo(Utils::ResolvePath("@user@")))
            {
                // The main reason we require screenshots to be in a specific folder is to ensure we don't delete or replace some other important file.
                AZ_Error("ScriptAutomation", false, "Screenshots must be captured under the '%s' folder. Attempted to save screenshot to '%s'.",
                    Utils::ResolvePath("@user@").c_str(),
                    fullFilePath.c_str());

                return false;
            }

            // Delete the file if it already exists
            if (AZ::IO::LocalFileIO::GetInstance()->Exists(fullFilePath.c_str()) &&
                !AZ::IO::LocalFileIO::GetInstance()->Remove(fullFilePath.c_str()))
            {
                AZ_Error("ScriptAutomation", false, "Failed to delete existing screenshot file '%s'.", fullFilePath.c_str());
                return false;
            }

            auto scriptAutomationInterface = ScriptAutomationInterface::Get();

            scriptAutomationInterface->PauseAutomation();

            return true;
        }

        void SetScreenshotFolder(const AZStd::string& screenshotFolder)
        {
            auto operation = [screenshotFolder]()
            {
                AZ::Render::FrameCaptureTestRequestBus::Broadcast(
                    &AZ::Render::FrameCaptureTestRequestBus::Events::SetScreenshotFolder, screenshotFolder);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void SetTestEnvPath(const AZStd::string& envPath)
        {
            auto operation = [envPath]()
            {
                AZ::Render::FrameCaptureTestRequestBus::Broadcast(&AZ::Render::FrameCaptureTestRequestBus::Events::SetTestEnvPath, envPath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void SetOfficialBaselineImageFolder(const AZStd::string& baselineFolder)
        {
            auto operation = [baselineFolder]()
            {
                AZ::Render::FrameCaptureTestRequestBus::Broadcast(
                    &AZ::Render::FrameCaptureTestRequestBus::Events::SetOfficialBaselineImageFolder, baselineFolder);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void SetLocalBaselineImageFolder(const AZStd::string& baselineFolder)
        {
            auto operation = [baselineFolder]()
            {
                AZ::Render::FrameCaptureTestRequestBus::Broadcast(
                    &AZ::Render::FrameCaptureTestRequestBus::Events::SetLocalBaselineImageFolder, baselineFolder);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureScreenshot(const AZStd::string& imageName)
        {
            auto operation = [imageName]()
            {
                // Note this will pause the script until the capture is complete
                if (PrepareForScreenCapture(imageName))
                {
                    AZStd::string screenshotFilePath;
                    AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                        screenshotFilePath, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    AZ::Render::FrameCaptureId frameCaptureId = AZ::Render::InvalidFrameCaptureId;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(frameCaptureId, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot, screenshotFilePath);
                    if (frameCaptureId != AZ::Render::InvalidFrameCaptureId)
                    {
                        scriptAutomationInterface->SetFrameCaptureId(frameCaptureId);
                    }
                    else
                    {
                        AZ_Error("ScriptAutomation", false, "Script: failed to trigger screenshot capture");
                        scriptAutomationInterface->ResumeAutomation();
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureScreenshotWithPreview(const AZStd::string& imageName)
        {
            auto operation = [imageName]()
            {
                // Note this will pause the script until the capture is complete
                if (PrepareForScreenCapture(imageName))
                {
                    AZStd::string screenshotFilePath;
                    AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                        screenshotFilePath, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    AZ::Render::FrameCaptureId frameCaptureId = AZ::Render::InvalidFrameCaptureId;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(frameCaptureId, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshotWithPreview, screenshotFilePath);
                    if (frameCaptureId != AZ::Render::InvalidFrameCaptureId)
                    {
                        scriptAutomationInterface->SetFrameCaptureId(frameCaptureId);
                    }
                    else
                    {
                        AZ_Error("ScriptAutomation", false, "Script: failed to trigger screenshot capture with preview");
                        scriptAutomationInterface->ResumeAutomation();
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
            AZStd::string imageName;

            // read slot name and output file path
            dc.ReadArg(1, stringValue);
            slot = AZStd::string(stringValue);
            dc.ReadArg(2, stringValue);
            imageName = AZStd::string(stringValue);

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

            auto operation = [passHierarchy, slot, imageName, readbackOption]()
            {
                // Note this will pause the script until the capture is complete
                if (PrepareForScreenCapture(imageName))
                {
                    AZStd::string screenshotFilePath;
                    AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                        screenshotFilePath, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    AZ::Render::FrameCaptureId frameCaptureId = AZ::Render::InvalidFrameCaptureId;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(frameCaptureId, &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment, passHierarchy, slot, screenshotFilePath, readbackOption);
                    if (frameCaptureId != AZ::Render::InvalidFrameCaptureId)
                    {
                        scriptAutomationInterface->SetFrameCaptureId(frameCaptureId);
                    }
                    else
                    {
                        AZ_Error("ScriptAutomation", false, "Script: failed to trigger screenshot capture of pass");
                        scriptAutomationInterface->ResumeAutomation();
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CompareScreenshots(const AZStd::string& filePathA, const AZStd::string& filePathB, float minDiffFilter)
        {
            auto operation = [filePathA, filePathB, minDiffFilter]()
            {
                AZStd::string resolvedPathA = ResolvePath(filePathA);
                AZStd::string resolvedPathB = ResolvePath(filePathB);

                AZ::Utils::ImageDiffResult result;
                AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                    result,
                    &AZ::Render::FrameCaptureTestRequestBus::Events::CompareScreenshots,
                    resolvedPathA,
                    resolvedPathB,
                    minDiffFilter);

                if (result.m_resultCode == AZ::Utils::ImageDiffResultCode::Success)
                {
                    AZ_Printf(
                        "ScriptAutomation",
                        "Diff score is %.5f from %s and %s.",
                        result.m_diffScore,
                        resolvedPathA.c_str(),
                        resolvedPathB.c_str());
                    AZ_Printf(
                        "ScriptAutomation",
                        "Filtered diff score is %.5f from %s and %s.",
                        result.m_filteredDiffScore,
                        resolvedPathA.c_str(),
                        resolvedPathB.c_str());
                }
                else
                {
                    AZ_Error(
                        "ScriptAutomation",
                        false,
                        "Screenshots compare failed for %s and %s.",
                        resolvedPathA.c_str(),
                        resolvedPathB.c_str());
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }
    } // namespace Bindings

    void ReflectScriptBindings(AZ::BehaviorContext* behaviorContext)
    {
        AZ::MathReflect(behaviorContext);
        AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistryToBehaviorContext(*behaviorContext);

        behaviorContext->Method("Print", &Bindings::Print);
        behaviorContext->Method("Warning", &Bindings::Warning);
        behaviorContext->Method("Error", &Bindings::Error);

        behaviorContext->Method("ExecuteConsoleCommand", &Bindings::ExecuteConsoleCommand);

        behaviorContext->Method("IdleFrames", &Bindings::IdleFrames);
        behaviorContext->Method("IdleSeconds", &Bindings::IdleSeconds);
        behaviorContext->Method("ResizeViewport", &Bindings::ResizeViewport);

        behaviorContext->Method("ResolvePath", &Bindings::ResolvePath);
        behaviorContext->Method("NormalizePath", [](AZStd::string_view path) -> AZStd::string { return AZ::IO::PathView(path).LexicallyNormal().String(); });
        behaviorContext->Method("DegToRad", &AZ::DegToRad);
        behaviorContext->Method("GetRenderApiName", &Bindings::GetRenderApiName);
        behaviorContext->Method("GetProfilingOutputPath", &Bindings::GetProfilingOutputPath);

        // Screenshots...
        behaviorContext->Method("SetScreenshotFolder", &Bindings::SetScreenshotFolder);
        behaviorContext->Method("SetTestEnvPath", &Bindings::SetTestEnvPath);
        behaviorContext->Method("SetOfficialBaselineImageFolder", &Bindings::SetOfficialBaselineImageFolder);
        behaviorContext->Method("SetLocalBaselineImageFolder", &Bindings::SetLocalBaselineImageFolder);
        behaviorContext->Method("CaptureScreenshot", &Bindings::CaptureScreenshot);
        behaviorContext->Method("CaptureScreenshotWithPreview", &Bindings::CaptureScreenshotWithPreview);
        behaviorContext->Method("CapturePassAttachment", &Bindings::CapturePassAttachment);
        behaviorContext->Method("CompareScreenshots", &Bindings::CompareScreenshots);

        // Profiling data...
        behaviorContext->Method("CapturePassTimestamp", &Bindings::CapturePassTimestamp);
        behaviorContext->Method("CaptureCpuFrameTime", &Bindings::CaptureCpuFrameTime);
        behaviorContext->Method("CapturePassPipelineStatistics", &Bindings::CapturePassPipelineStatistics);
        behaviorContext->Method("CaptureCpuProfilingStatistics", &Bindings::CaptureCpuProfilingStatistics);
        behaviorContext->Method("CaptureBenchmarkMetadata", &Bindings::CaptureBenchmarkMetadata);
    }
} // namespace ScriptAutomation
