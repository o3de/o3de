/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <ScriptAutomationScriptBindings.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/optional.h>
#include <AzCore/IO/Path/Path.h>

#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <ScriptAutomation_Traits.h>

AZ_CVAR(AZ::CVarFixedString, sa_image_compare_app_path, AZ_TRAIT_SCRIPTAUTOMATION_DEFAULT_IMAGE_COMPARE_PATH, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Default image compare app path");
AZ_CVAR(AZ::CVarFixedString, sa_image_compare_arguments, AZ_TRAIT_SCRIPTAUTOMATION_DEFAULT_IMAGE_COMPARE_ARGUMENTS, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Default image compare arguments");
AZ_CVAR(bool, sa_launch_image_compare_for_failed_baseline_compare, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Should ScriptAutomation launch an image compare for every failed screenshot baseline compare");
/* sa_launch_image_compare_for_failed_baseline_compare can be set to true for local work by adding a setreg file containing the below json
 * {
 *      "Amazon": {
 *          "AzCore": {
 *              "Runtime": {
 *                  "ConsoleCommands": {
 *                      "sa_launch_image_compare_for_failed_baseline_compare": 1
 *                  }
 *              }
 *          }
 *      }
 *  }
 */
namespace AZ::Platform
{
    bool LaunchProgram(const AZStd::string& progPath, const AZStd::string& arguments);
}

namespace AZ::ScriptAutomation
{
    static constexpr char NewScreenshotPlaceholder[] = "{NewScreenshotPath}";
    static constexpr char ExpectedScreenshotPlaceholder[] = "{ExpectedScreenshotPath}";
    static constexpr char TestNamePlaceholder[] = "{TestName}";
    static constexpr char ImageNamePlaceholder[] = "{ImageName}";
    static constexpr char PlaceholderEndChar[] = "}";

   namespace Utils
    {
        void ReplacePlaceholder(AZStd::string& string, const char* placeholderName, const AZStd::string& newValue)
        {
            for (auto index = string.find(placeholderName); index != AZStd::string::npos; index = string.find(placeholderName))
            {
                auto endIndex = string.find(PlaceholderEndChar, index);
                string.erase(index, endIndex - index + 1);
                string.insert(index, newValue);
            }
        }
        void RunImageDiff(
            const AZStd::string& newImagePath, 
            const AZStd::string& compareImagePath, 
            const AZStd::string& testName,
            const AZStd::string& imageName)
        {
            AZStd::string appPath = static_cast<AZStd::string_view>(static_cast<AZ::CVarFixedString>(sa_image_compare_app_path));
            AZStd::string arguments = static_cast<AZStd::string_view>(static_cast<AZ::CVarFixedString>(sa_image_compare_arguments));
            ReplacePlaceholder(arguments, NewScreenshotPlaceholder, newImagePath);
            ReplacePlaceholder(arguments, ExpectedScreenshotPlaceholder, compareImagePath);
            ReplacePlaceholder(arguments, TestNamePlaceholder, testName);
            ReplacePlaceholder(arguments, ImageNamePlaceholder, imageName);

            if (!Platform::LaunchProgram(appPath, arguments))
            {
                AZ_Error("ScriptAutomation", false, "Failed to launch image diff - \"%s %s\"", appPath.c_str(), arguments.c_str());
            }
        }

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
        void RunScript(const AZStd::string& scriptFilePath)
        {
            // Unlike other Script_ callback functions, we process immediately instead of pushing onto the m_scriptOperations queue.
            // This function is special because running the script is what adds more commands onto the m_scriptOperations queue.
            ScriptAutomationInterface::Get()->ExecuteScript(scriptFilePath.c_str());
        }

        void Print(const AZStd::string& message [[maybe_unused]])
        {
            auto func = [message]()
            {
                AZ_UNUSED(message); // mark explicitly captured variable `message` as used because AZ_Error is a nop in release builds
                AZ_Info("ScriptAutomation", "Script: %s\n", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void Warning(const AZStd::string& message [[maybe_unused]])
        {
            auto func = [message]()
            {
                AZ_UNUSED(message); // mark explicitly captured variable `message` as used because AZ_Error is a nop in release builds
                AZ_Warning("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void Error(const AZStd::string& message [[maybe_unused]])
        {
            auto func = [message]()
            {
                AZ_UNUSED(message); // mark explicitly captured variable `message` as used because AZ_Error is a nop in release builds
                AZ_Error("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
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

        void SetCamera(const AZStd::string& entityName)
        {
            auto operation = [entityName]()
            {
                // Find all Component Entity Cameras
                AZ::EBusAggregateResults<AZ::EntityId> cameraComponentEntities;
                Camera::CameraBus::BroadcastResult(cameraComponentEntities, &Camera::CameraRequests::GetCameras);

                // add names of all found entities with Camera Components
                for (int i = 0; i < cameraComponentEntities.values.size(); i++)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, cameraComponentEntities.values[i]);
                    if (entity)
                    {
                        if (entity->GetName() == entityName)
                        {
                            Camera::CameraRequestBus::Event(cameraComponentEntities.values[i], &Camera::CameraRequestBus::Events::MakeActiveView);
                        }
                    }
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

        AZStd::vector<AZStd::string> SplitStringImmediate(const AZStd::string& source, const AZStd::string& delimiter)
        {
            AZStd::vector<AZStd::string> splitStringList;
            auto SplitString = [&splitStringList](AZStd::string_view token)
            {
                splitStringList.emplace_back(token);
            };
            AZ::StringFunc::TokenizeVisitor(source, SplitString, delimiter, false, false);
            return splitStringList;
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

        AZStd::string GetRenderPipelineName()
        {
            AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
            AZStd::string renderPipelinePath;
            console->GetCvarValue("r_renderPipelinePath", renderPipelinePath);
            AZ_Assert(renderPipelinePath.size() > 0, "Invalid render pipeline path obtained from r_renderPipelinePath CVAR");
            return AZ::IO::PathView(renderPipelinePath).Stem().String();
        }

        AZStd::string GetPlatformName()
        {
            return AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER;
        }

        AZStd::string GetProfilingOutputPath(bool normalized)
        {
            return Utils::GetProfilingPath(normalized).String();
        }

        void LoadLevel(const AZStd::string& levelName)
        {
            auto operation = [levelName]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->LoadLevel(levelName.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        bool PrepareForScreenCapture(const AZStd::string& imageName)
        {
            AZ::Render::FrameCapturePathOutcome pathOutcome;
            AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                pathOutcome, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

            if (!pathOutcome.IsSuccess())
            {
                return false;
            }

            AZStd::string fullFilePath = pathOutcome.GetValue();

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
                    AZ::Render::FrameCapturePathOutcome pathOutcome;
                    AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                        pathOutcome, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                    AZ_Assert(pathOutcome.IsSuccess(), "Path check should already be done in PrepareForScreenCapture().");
                    AZStd::string screenshotFilePath = pathOutcome.GetValue();

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(captureOutcome, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot, screenshotFilePath);

                    AZ_Error("ScriptAutomation", captureOutcome.IsSuccess(),
                        "Frame capture initialization failed. %s", captureOutcome.GetError().m_errorMessage.c_str());
                    if (captureOutcome.IsSuccess())
                    {
                        scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());
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
                    AZ::Render::FrameCapturePathOutcome pathOutcome;
                    AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                        pathOutcome, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                    AZ_Assert(pathOutcome.IsSuccess(), "Path check should already be done in PrepareForScreenCapture().");
                    AZStd::string screenshotFilePath = pathOutcome.GetValue();

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(captureOutcome, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshotWithPreview, screenshotFilePath);

                    AZ_Error("ScriptAutomation", captureOutcome.IsSuccess(),
                        "Frame capture initialization failed. %s", captureOutcome.GetError().m_errorMessage.c_str());
                    if (captureOutcome.IsSuccess())
                    {
                        scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());
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
                    AZ::Render::FrameCapturePathOutcome pathOutcome;
                    AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                        pathOutcome, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                    AZ_Assert(pathOutcome.IsSuccess(), "Path check should already be done in PrepareForScreenCapture().");
                    AZStd::string screenshotFilePath = pathOutcome.GetValue();

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                        captureOutcome,
                        &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment,
                        screenshotFilePath,
                        passHierarchy,
                        slot,
                        readbackOption);

                    AZ_Error("ScriptAutomation", captureOutcome.IsSuccess(),
                        "Frame capture initialization failed. %s", captureOutcome.GetError().m_errorMessage.c_str());
                    if (captureOutcome.IsSuccess())
                    {
                        scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());
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

        void CompareScreenshots(const AZStd::string& compareName, const AZStd::string& comparisonLevel, const AZStd::string& filePathA, const AZStd::string& filePathB, float minDiffFilter)
        {
            // capture strings by copy or risk them being deleted before we access them.
            auto operation = [=]()
            {

                const ImageComparisonToleranceLevel* toleranceLevel = ScriptAutomationInterface::Get()->FindToleranceLevel(comparisonLevel);
                if (!toleranceLevel)
                {
                    AZ_Error("ScriptAutomation", false, "Failed to find image comparison level named %s", comparisonLevel.c_str());
                    return;
                }
                AZStd::string resolvedPathA = ResolvePath(filePathA);
                AZStd::string resolvedPathB = ResolvePath(filePathB);

                AZ::Render::FrameCaptureComparisonOutcome compareOutcome;
                AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                    compareOutcome,
                    &AZ::Render::FrameCaptureTestRequestBus::Events::CompareScreenshots,
                    resolvedPathA,
                    resolvedPathB,
                    minDiffFilter);

                AZ_Error(
                    "ScriptAutomation", 
                    compareOutcome.IsSuccess(),
                    "%s screenshot compare error. Error \"%s\"", 
                    compareName.c_str(),
                    compareOutcome.GetError().m_errorMessage.c_str()
                );
                if (compareOutcome.IsSuccess())
                {
                    float diffScore = toleranceLevel->m_filterImperceptibleDiffs
                        ? compareOutcome.GetValue().m_diffScore
                        : compareOutcome.GetValue().m_filteredDiffScore;

                    if (diffScore > toleranceLevel->m_threshold)
                    {
                        AZ_Error(
                            "ScriptAutomation", 
                            false,
                            "%s screenshot compare failed. Diff score %.5f exceeds threshold of %.5f ('%s').",
                            compareName.c_str(),
                            diffScore,
                            toleranceLevel->m_threshold,
                            toleranceLevel->m_name.c_str()
                        );

                        // TODO: open image compare app if CVAR is set
                    }
                    else
                    {
                        AZ_Printf(
                            "ScriptAutomation",
                            "%s screenshot compare passed. Diff score is %.5f, threshold of %.5f ('%s').",
                            compareName.c_str(),
                            diffScore,
                            toleranceLevel->m_threshold,
                            toleranceLevel->m_name.c_str()
                        );
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }


        void CompareScreenshotToBaseline(const AZStd::string& compareName, const AZStd::string& comparisonLevel, const AZStd::string& imageName, float minDiffFilter)
        {
            // capture strings by copy or risk them being deleted before we access them.
            auto operation = [=]()
            {

                const ImageComparisonToleranceLevel* toleranceLevel = ScriptAutomationInterface::Get()->FindToleranceLevel(comparisonLevel);
                if (!toleranceLevel)
                {
                    AZ_Error("ScriptAutomation", false, "Failed to find image comparison level named %s", comparisonLevel.c_str());
                    return;
                }

                // build test image filepath
                AZ::Render::FrameCapturePathOutcome pathOutcome;
                AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                    pathOutcome, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath, imageName, true);

                if (!pathOutcome.IsSuccess())
                {
                    AZ_Error(
                        "ScriptAutomation", 
                        false, 
                        "%s screenshot compare error. Failed to build screenshot file path for image name %s",
                        compareName.c_str(),
                        imageName.c_str());
                }
                AZStd::string screenshotFilePath = pathOutcome.GetValue();

                // build official comparison image filepath
                AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                    pathOutcome, &AZ::Render::FrameCaptureTestRequestBus::Events::BuildOfficialBaselineFilePath, imageName, true);

                if (!pathOutcome.IsSuccess())
                {
                    AZ_Error(
                        "ScriptAutomation", 
                        false, 
                        "%s screenshot compare error. Failed to build official baseline file path for image name %s",
                        compareName.c_str(),
                        imageName.c_str());
                }
                AZStd::string baselineFilePath = pathOutcome.GetValue();

                // compare test image against the official baseline
                AZ::Render::FrameCaptureComparisonOutcome compareOutcome;
                AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                    compareOutcome,
                    &AZ::Render::FrameCaptureTestRequestBus::Events::CompareScreenshots,
                    screenshotFilePath,
                    baselineFilePath,
                    minDiffFilter);

                AZ_Error(
                    "ScriptAutomation", 
                    compareOutcome.IsSuccess(),
                    "%s screenshot compare error. Error \"%s\"", 
                    compareName.c_str(),
                    compareOutcome.GetError().m_errorMessage.c_str()
                );
                if (compareOutcome.IsSuccess())
                {
                    float diffScore = toleranceLevel->m_filterImperceptibleDiffs
                        ? compareOutcome.GetValue().m_diffScore
                        : compareOutcome.GetValue().m_filteredDiffScore;

                    if (diffScore > toleranceLevel->m_threshold)
                    {
                        AZ_Error(
                            "ScriptAutomation", 
                            false,
                            "%s screenshot compare failed. Diff score %.5f exceeds threshold of %.5f ('%s').",
                            compareName.c_str(),
                            diffScore,
                            toleranceLevel->m_threshold,
                            toleranceLevel->m_name.c_str()
                        );

                        if (sa_launch_image_compare_for_failed_baseline_compare)
                        {
                            Utils::RunImageDiff(screenshotFilePath, baselineFilePath, compareName, imageName);
                        }
                    }
                    else
                    {
                        AZ_Printf(
                            "ScriptAutomation",
                            "%s screenshot compare passed. Diff score is %.5f, threshold of %.5f ('%s').\n",
                            compareName.c_str(),
                            diffScore,
                            toleranceLevel->m_threshold,
                            toleranceLevel->m_name.c_str()
                        );
                    }
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }
    } // namespace Bindings

    void ReflectScriptBindings(AZ::BehaviorContext* behaviorContext)
    {
        AZ::MathReflect(behaviorContext);
        AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistryToBehaviorContext(*behaviorContext);

        behaviorContext->Method("RunScript", &Bindings::RunScript);
        behaviorContext->Method("Print", &Bindings::Print);
        behaviorContext->Method("Warning", &Bindings::Warning);
        behaviorContext->Method("Error", &Bindings::Error);

        behaviorContext->Method("ExecuteConsoleCommand", &Bindings::ExecuteConsoleCommand);

        behaviorContext->Method("IdleFrames", &Bindings::IdleFrames);
        behaviorContext->Method("IdleSeconds", &Bindings::IdleSeconds);
        behaviorContext->Method("ResizeViewport", &Bindings::ResizeViewport);
        behaviorContext->Method("SetCamera", &Bindings::SetCamera);

        behaviorContext->Method("SplitString", &Bindings::SplitStringImmediate);
        behaviorContext->Method("ResolvePath", &Bindings::ResolvePath);
        behaviorContext->Method("NormalizePath", [](AZStd::string_view path) -> AZStd::string { return AZ::IO::PathView(path).LexicallyNormal().String(); });
        behaviorContext->Method("DegToRad", &AZ::DegToRad);
        behaviorContext->Method("GetRenderApiName", &Bindings::GetRenderApiName);
        behaviorContext->Method("GetRenderPipelineName", &Bindings::GetRenderPipelineName);
        behaviorContext->Method("GetPlatformName", &Bindings::GetPlatformName);
        behaviorContext->Method("GetProfilingOutputPath", &Bindings::GetProfilingOutputPath);

        behaviorContext->Method("LoadLevel", &Bindings::LoadLevel);

        // Screenshots...
        behaviorContext->Method("SetScreenshotFolder", &Bindings::SetScreenshotFolder);
        behaviorContext->Method("SetTestEnvPath", &Bindings::SetTestEnvPath);
        behaviorContext->Method("SetOfficialBaselineImageFolder", &Bindings::SetOfficialBaselineImageFolder);
        behaviorContext->Method("SetLocalBaselineImageFolder", &Bindings::SetLocalBaselineImageFolder);
        behaviorContext->Method("CaptureScreenshot", &Bindings::CaptureScreenshot);
        behaviorContext->Method("CaptureScreenshotWithPreview", &Bindings::CaptureScreenshotWithPreview);
        behaviorContext->Method("CapturePassAttachment", &Bindings::CapturePassAttachment);
        behaviorContext->Method("CompareScreenshots", &Bindings::CompareScreenshots);
        behaviorContext->Method("CompareScreenshotToBaseline", &Bindings::CompareScreenshotToBaseline);


        // Profiling data...
        behaviorContext->Method("CapturePassTimestamp", &Bindings::CapturePassTimestamp);
        behaviorContext->Method("CaptureCpuFrameTime", &Bindings::CaptureCpuFrameTime);
        behaviorContext->Method("CapturePassPipelineStatistics", &Bindings::CapturePassPipelineStatistics);
        behaviorContext->Method("CaptureCpuProfilingStatistics", &Bindings::CaptureCpuProfilingStatistics);
        behaviorContext->Method("CaptureBenchmarkMetadata", &Bindings::CaptureBenchmarkMetadata);
    }
} // namespace AZ::ScriptAutomation
