/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <Atom/Component/DebugCamera/ArcBallControllerComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <ScriptableImGui.h>
#include <ScriptAutomationScriptBindings.h>
#include <ScriptAutomationSystemComponent.h>
#include <Utils.h>

namespace ScriptAutomation
{
    namespace Bindings
    {
        void Error(const AZStd::string& message)
        {
            auto operation = [message]()
            {
                AZ_Error("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void Warning(const AZStd::string& message)
        {
            auto operation = [message]()
            {
                AZ_Warning("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void Print(const AZStd::string& message)
        {
            auto operation = [message]()
            {
                AZ_TracePrintf("ScriptAutomation", "Script: %s\n", message.c_str());
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

        void RunScript(const AZStd::string& scriptFilePath)
        {
            // Unlike other callback functions, we process immediately instead of pushing onto the m_scriptOperations queue.
            // This function is special because running the script is what adds more commands onto the m_scriptOperations queue.
            ScriptAutomationInterface::Get()->ExecuteScript(scriptFilePath);
        }

        void IdleFrames(int numFrames)
        {
            auto operation = [numFrames]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->SetIdleFrames(numFrames);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void IdleSeconds(float numSeconds)
        {
            auto operation = [numSeconds]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->SetIdleSeconds(numSeconds);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void SetImguiValue(AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() != 2)
            {
                AZ_Error("ScriptAutomation", false, "Wrong number of arguments for SetImguiValue.");
                return;
            }

            if (!dc.IsString(0))
            {
                AZ_Error("ScriptAutomation", false, "SetImguiValue first argument must be a string");
                return;
            }

            const char* fieldName = nullptr;
            dc.ReadArg(0, fieldName);

            AZStd::string fieldNameString = fieldName; // Because the lambda will need to capture a copy of something, not a pointer

            if (dc.IsBoolean(1))
            {
                bool value = false;
                dc.ReadArg(1, value);

                auto func = [fieldNameString, value]()
                {
                    ScriptableImGui::SetBool(fieldNameString, value);
                };

                ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
            }
            else if (dc.IsNumber(1))
            {
                float value = 0.0f;
                dc.ReadArg(1, value);

                auto func = [fieldNameString, value]()
                {
                    ScriptableImGui::SetNumber(fieldNameString, value);
                };

                ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
            }
            else if (dc.IsString(1))
            {
                const char* value = nullptr;
                dc.ReadArg(1, value);

                AZStd::string valueString = value; // Because the lambda will need to capture a copy of something, not a pointer

                auto func = [fieldNameString, valueString]()
                {
                    ScriptableImGui::SetString(fieldNameString, valueString);
                };

                ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
            }
            else if (dc.IsClass<AZ::Vector3>(1))
            {
                AZ::Vector3 value = AZ::Vector3::CreateZero();
                dc.ReadArg(1, value);

                auto func = [fieldNameString, value]()
                {
                    ScriptableImGui::SetVector(fieldNameString, value);
                };

                ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
            }
            else if (dc.IsClass<AZ::Vector2>(1))
            {
                AZ::Vector2 value = AZ::Vector2::CreateZero();
                dc.ReadArg(1, value);

                auto func = [fieldNameString, value]()
                {
                    ScriptableImGui::SetVector(fieldNameString, value);
                };

                ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
            }
        }

        void ResizeViewport(int width, int height)
        {
            auto operation = [width, height]()
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

        void ExecuteConsoleCommand(const AZStd::string& command)
        {
            auto operation = [command]()
            {
                AzFramework::ConsoleRequestBus::Broadcast(&AzFramework::ConsoleRequests::ExecuteConsoleCommand, command.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void SetShowImGui(bool show)
        {
            auto operation = [show]()
            {
                ScriptAutomationInterface::Get()->SetShowImGui(show);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        bool PrepareForScreenCapture(const AZStd::string& imageName, AZStd::string& fullFilePath)
        {
            AZ::Render::FrameCapturePathOutcome pathOutcome;

            AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                pathOutcome,
                &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath,
                imageName, true);

            if (!pathOutcome.IsSuccess())
            {
                AZ_Error("ScriptAutomation", false, "%s", pathOutcome.GetError().m_errorMessage.c_str());
                return false;
            }

            fullFilePath = pathOutcome.GetValue();

            if (!AZ::IO::PathView(fullFilePath).IsRelativeTo(AZ::IO::PathView(ResolvePath("@user@"))))
            {
                // The main reason we require screenshots to be in a specific folder is to ensure we don't delete or replace some other important file.
                AZ_Error("ScriptAutomation", false, "Screenshots must be captured under the '%s' folder. Attempted to save screenshot to '%s'.",
                    ResolvePath("@user@").c_str(),
                    fullFilePath.c_str());

                return false;
            }

            // Delete the file if it already exists because if the screen capture fails, we don't want to do a screenshot comparison test using an old screenshot.
            if (AZ::IO::LocalFileIO::GetInstance()->Exists(fullFilePath.c_str()) && !AZ::IO::LocalFileIO::GetInstance()->Remove(fullFilePath.c_str()))
            {
                AZ_Error("ScriptAutomation", false, "Failed to delete existing screenshot file '%s'.", fullFilePath.c_str());
                return false;
            }

            auto scriptAutomationInterface = ScriptAutomationInterface::Get();

            scriptAutomationInterface->StartFrameCapture(imageName);
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
                AZ::Render::FrameCaptureTestRequestBus::Broadcast(
                    &AZ::Render::FrameCaptureTestRequestBus::Events::SetTestEnvPath, envPath);
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

        void SelectImageComparisonToleranceLevel(const AZStd::string& presetName)
        {
            auto operation = [presetName]()
            {
                ScriptAutomationInterface::Get()->SetImageComparisonToleranceLevel(presetName);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureScreenshot(const AZStd::string& imageName)
        {
            SetShowImGui(false);

            auto operation = [imageName]()
            {
                // Note this will pause the script until the capture is complete
                AZStd::string screenshotFilePath;
                if (PrepareForScreenCapture(imageName, screenshotFilePath))
                {
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                        captureOutcome, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot, screenshotFilePath);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    if (!captureOutcome.IsSuccess())
                    {
                        AZ_Error("ScriptAutomation", false, "Failed to initiate frame capture for '%s'.", screenshotFilePath.c_str());
                        scriptAutomationInterface->StopFrameCapture();
                        scriptAutomationInterface->ResumeAutomation();
                        return;
                    }

                    scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));

            // restore imgui show/hide
            ScriptAutomationInterface::Get()->QueueScriptOperation(
                []()
                {
                    ScriptAutomationInterface::Get()->RestoreShowImGui();
                });
        }

        void CaptureScreenshotWithImGui(const AZStd::string& imageName)
        {
            SetShowImGui(true);

            auto operation = [imageName]()
            {
                // Note this will pause the script until the capture is complete
                AZStd::string screenshotFilePath;
                if (PrepareForScreenCapture(imageName, screenshotFilePath))
                {
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                        captureOutcome, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot, screenshotFilePath);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    if (!captureOutcome.IsSuccess())
                    {
                        AZ_Error("ScriptAutomation", false, "Failed to initiate frame capture for '%s'.", screenshotFilePath.c_str());
                        scriptAutomationInterface->StopFrameCapture();
                        scriptAutomationInterface->ResumeAutomation();
                        return;
                    }

                    scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));

            // restore imgui show/hide
            ScriptAutomationInterface::Get()->QueueScriptOperation(
                []()
                {
                    ScriptAutomationInterface::Get()->RestoreShowImGui();
                });
        }

        void CaptureScreenshotWithPreview(const AZStd::string& imageName)
        {
            auto operation = [imageName]()
            {
                // Note this will pause the script until the capture is complete
                AZStd::string screenshotFilePath;
                if (PrepareForScreenCapture(imageName, screenshotFilePath))
                {
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                        captureOutcome, &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshotWithPreview, screenshotFilePath);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    if (!captureOutcome.IsSuccess())
                    {
                        AZ_Error("ScriptAutomation", false, "Failed to initiate frame capture for '%s'.", screenshotFilePath.c_str());
                        scriptAutomationInterface->StopFrameCapture();
                        scriptAutomationInterface->ResumeAutomation();
                        return;
                    }

                    scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CapturePassAttachment(AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() != 3 && dc.GetNumArguments() != 4)
            {
                AZ_Error("ScriptAutomation", false, "CapturePassAttachment needs three or four arguments.");
                return;
            }

            if (!dc.IsTable(0))
            {
                AZ_Error("ScriptAutomation", false, "CapturePassAttachment's first argument must be a table of strings");
                return;
            }

            if (!dc.IsString(1) || !dc.IsString(2))
            {
                AZ_Error("ScriptAutomation", false, "CapturePassAttachment's second and third argument must be strings");
                return;
            }

            if (dc.GetNumArguments() == 4 && !dc.IsString(3))
            {
                AZ_Error("ScriptAutomation", false, "CapturePassAttachment's forth argument must be a string 'Input' or 'Output'");
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
                        AZ_Error("ScriptAutomation", false, "CapturePassAttachment's first argument must contain only strings.");
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
                AZStd::string screenshotFilePath;
                if (PrepareForScreenCapture(imageName, screenshotFilePath))
                {
                    AZ::Render::FrameCaptureOutcome captureOutcome;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(captureOutcome, &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment, screenshotFilePath, passHierarchy, slot, readbackOption);

                    auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                    if (!captureOutcome.IsSuccess())
                    {
                        AZ_Error("ScriptAutomation", false, "Failed to initiate frame capture for '%s'", screenshotFilePath.c_str());
                        scriptAutomationInterface->StopFrameCapture();
                        scriptAutomationInterface->ResumeAutomation();
                        return;
                    }

                    scriptAutomationInterface->SetFrameCaptureId(captureOutcome.GetValue());

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

                AZ::Render::FrameCaptureComparisonOutcome compareOutcome;
                AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                    compareOutcome,
                    &AZ::Render::FrameCaptureTestRequestBus::Events::CompareScreenshots,
                    resolvedPathA,
                    resolvedPathB,
                    minDiffFilter);

                AZ_Error("ScriptAutomation", compareOutcome.IsSuccess(),
                    "Image comparison failed. %s", compareOutcome.GetError().m_errorMessage.c_str());
                if (compareOutcome.IsSuccess())
                {
                    AZ_Printf(
                        "ScriptAutomation",
                        "Diff score is %.5f from %s and %s.",
                        compareOutcome.GetValue().m_diffScore,
                        resolvedPathA.c_str(),
                        resolvedPathB.c_str());
                    AZ_Printf(
                        "ScriptAutomation",
                        "Filtered diff score is %.5f from %s and %s.",
                        compareOutcome.GetValue().m_filteredDiffScore,
                        resolvedPathA.c_str(),
                        resolvedPathB.c_str());
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        bool ValidateProfilingCaptureScripContexts(AZ::ScriptDataContext& dc, AZStd::string& outputFilePath)
        {
            if (dc.GetNumArguments() != 1)
            {
                AZ_Error("ScriptAutomation", false, "ProfilingCaptureScriptDataContext needs one argument.");
                return false;
            }

            if (!dc.IsString(0))
            {
                AZ_Error(
                    "ScriptAutomation", false, "ProfilingCaptureScriptDataContext's first (and only) argument must be of type string.");
                return false;
            }

            // Read slot name and output file path
            const char* stringValue = nullptr;
            dc.ReadArg(0, stringValue);
            if (stringValue == nullptr)
            {
                AZ_Error("ScriptAutomation", false, "ProfilingCaptureScriptDataContext failed to read the string value.");
                return false;
            }

            outputFilePath = AZStd::string(stringValue);
            return true;
        }

        void CapturePassTimestamp(AZ::ScriptDataContext& dc)
        {
            AZStd::string outputFilePath;
            const bool readScriptDataContext = ValidateProfilingCaptureScripContexts(dc, outputFilePath);
            if (!readScriptDataContext)
            {
                return;
            }

            auto operation = [outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CapturePassTimestamp, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureCpuFrameTime(AZ::ScriptDataContext& dc)
        {
            AZStd::string outputFilePath;
            const bool readScriptDataContext = ValidateProfilingCaptureScripContexts(dc, outputFilePath);
            if (!readScriptDataContext)
            {
                return;
            }

            auto operation = [outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureCpuFrameTime, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
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

        AZStd::string GetProfilingOutputPath(bool resolvePath)
        {
            return Utils::GetProfilingPath(resolvePath).String();
        }

        void CapturePassPipelineStatistics(AZ::ScriptDataContext& dc)
        {
            AZStd::string outputFilePath;
            const bool readScriptDataContext = ValidateProfilingCaptureScripContexts(dc, outputFilePath);
            if (!readScriptDataContext)
            {
                return;
            }

            auto operation = [outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CapturePassPipelineStatistics, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CaptureCpuProfilingStatistics(AZ::ScriptDataContext& dc)
        {
            AZStd::string outputFilePath;
            const bool readScriptDataContext = ValidateProfilingCaptureScripContexts(dc, outputFilePath);
            if (!readScriptDataContext)
            {
                return;
            }

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

        void CaptureBenchmarkMetadata(AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() != 2)
            {
                AZ_Error("ScriptAutomation", false, "CaptureBenchmarkMetadata needs two arguments, benchmarkName and outputFilePath.");
                return;
            }

            if (!dc.IsString(0) || !dc.IsString(1))
            {
                AZ_Error("ScriptAutomation", false, "CaptureBenchmarkMetadata's arguments benchmarkName and outputFilePath must both be of type string.");
                return;
            }

            const char* stringValue = nullptr;
            AZStd::string benchmarkName;
            AZStd::string outputFilePath;

            dc.ReadArg(0, stringValue);
            benchmarkName = AZStd::string(stringValue);
            dc.ReadArg(1, stringValue);
            outputFilePath = AZStd::string(stringValue);

            auto operation = [benchmarkName, outputFilePath]()
            {
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();

                scriptAutomationInterface->StartProfilingCapture();
                scriptAutomationInterface->PauseAutomation();

                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureBenchmarkMetadata, benchmarkName, outputFilePath);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void CheckArcBallControllerHandler()
        {
            auto scriptAutomationInterface = ScriptAutomationInterface::Get();
            if (0 == AZ::Debug::ArcBallControllerRequestBus::GetNumOfEventHandlers(scriptAutomationInterface->GetCameraEntity()->GetId()))
            {
                AZ_Error("ScriptAutomation", false, "There is no handler for ArcBallControllerRequestBus for the camera entity.");
            }
        }

        void CheckNoClipControllerHandler()
        {
            auto scriptAutomationInterface = ScriptAutomationInterface::Get();
            if (0 == AZ::Debug::NoClipControllerRequestBus::GetNumOfEventHandlers(scriptAutomationInterface->GetCameraEntity()->GetId()))
            {
                AZ_Error("ScriptAutomation", false, "There is no handler for NoClipControllerRequestBus for the camera entity.");
            }
        }

        void ArcBallCameraController_SetCenter(AZ::Vector3 center)
        {
            auto operation = [center]()
            {
                CheckArcBallControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::ArcBallControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetCenter, center);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void ArcBallCameraController_SetPan(AZ::Vector3 pan)
        {
            auto operation = [pan]()
            {
                CheckArcBallControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::ArcBallControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetPan, pan);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void ArcBallCameraController_SetDistance(float distance)
        {
            auto operation = [distance]()
            {
                CheckArcBallControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::ArcBallControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(),
                    &AZ::Debug::ArcBallControllerRequestBus::Events::SetDistance,
                    distance);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void ArcBallCameraController_SetHeading(float heading)
        {
            auto operation = [heading]()
            {
                CheckArcBallControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::ArcBallControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(),
                    &AZ::Debug::ArcBallControllerRequestBus::Events::SetHeading,
                    heading);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void ArcBallCameraController_SetPitch(float pitch)
        {
            auto operation = [pitch]()
            {
                CheckArcBallControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::ArcBallControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetPitch, pitch);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void NoClipCameraController_SetPosition(AZ::Vector3 position)
        {
            auto operation = [position]()
            {
                CheckNoClipControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::NoClipControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(),
                    &AZ::Debug::NoClipControllerRequestBus::Events::SetPosition,
                    position);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void NoClipCameraController_SetHeading(float heading)
        {
            auto operation = [heading]()
            {
                CheckNoClipControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::NoClipControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(), &AZ::Debug::NoClipControllerRequestBus::Events::SetHeading, heading);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void NoClipCameraController_SetPitch(float pitch)
        {
            auto operation = [pitch]()
            {
                CheckNoClipControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::NoClipControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(), &AZ::Debug::NoClipControllerRequestBus::Events::SetPitch, pitch);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void NoClipCameraController_SetFov(float fov)
        {
            auto operation = [fov]()
            {
                CheckNoClipControllerHandler();
                auto scriptAutomationInterface = ScriptAutomationInterface::Get();
                AZ::Debug::NoClipControllerRequestBus::Event(
                    scriptAutomationInterface->GetCameraEntity()->GetId(), &AZ::Debug::NoClipControllerRequestBus::Events::SetFov, fov);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void AssetTracking_Start()
        {
            auto operation = []()
            {
                ScriptAutomationInterface::Get()->StartAssetTracking();
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }


        void AssetTracking_ExpectAsset(const AZStd::string& sourceAssetPath, uint32_t expectedCount)
        {
            auto operation = [sourceAssetPath, expectedCount]()
            {
                ScriptAutomationInterface::Get()->ExpectAssets(sourceAssetPath, expectedCount);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void AssetTracking_IdleUntilExpectedAssetsFinish(float timeout)
        {
            auto operation = [timeout]()
            {
                ScriptAutomationInterface::Get()->WaitForExpectAssetsFinish(timeout);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }

        void AssetTracking_Stop()
        {
            auto operation = []()
            {
                ScriptAutomationInterface::Get()->StopAssetTracking();
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(operation));
        }
    } // namespace Bindings

    void ReflectScriptBindings(AZ::BehaviorContext* behaviorContext)
    {
        AZ::MathReflect(behaviorContext);
        AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistryToBehaviorContext(*behaviorContext);

        // Utilities returning data (these special functions do return data because they don't read dynamic state)...
        behaviorContext->Method("ResolvePath", &Bindings::ResolvePath);
        behaviorContext->Method("NormalizePath", &Bindings::NormalizePath);
        behaviorContext->Method("DegToRad", &Bindings::DegToRad);
        behaviorContext->Method("GetRenderApiName", &Bindings::GetRenderApiName);
        behaviorContext->Method("GetProfilingOutputPath", &Bindings::GetProfilingOutputPath);

        // Utilities...
        behaviorContext->Method("Error", &Bindings::Error);
        behaviorContext->Method("Warning", &Bindings::Warning);
        behaviorContext->Method("Print", &Bindings::Print);
        behaviorContext->Method("IdleFrames", &Bindings::IdleFrames);
        behaviorContext->Method("IdleSeconds", &Bindings::IdleSeconds);
        behaviorContext->Method("ResizeViewport", &Bindings::ResizeViewport);

        behaviorContext->Method("RunScript", &Bindings::RunScript);
        behaviorContext->Method("ExecuteConsoleCommand", &Bindings::ExecuteConsoleCommand);

        // ImGui operations...
        behaviorContext->Method("SetShowImGui", &Bindings::SetShowImGui);
        behaviorContext->Method("SetImguiValue", &Bindings::SetImguiValue);

        // Screenshots...
        behaviorContext->Method("SetScreenshotFolder", &Bindings::SetScreenshotFolder);
        behaviorContext->Method("SetTestEnvPath", &Bindings::SetTestEnvPath);
        behaviorContext->Method("SetOfficialBaselineImageFolder", &Bindings::SetOfficialBaselineImageFolder);
        behaviorContext->Method("SetLocalBaselineImageFolder", &Bindings::SetLocalBaselineImageFolder);
        behaviorContext->Method("SelectImageComparisonToleranceLevel", &Bindings::SelectImageComparisonToleranceLevel);

        behaviorContext->Method("CaptureScreenshot", &Bindings::CaptureScreenshot);
        behaviorContext->Method("CaptureScreenshotWithImGui", &Bindings::CaptureScreenshotWithImGui);
        behaviorContext->Method("CaptureScreenshotWithPreview", &Bindings::CaptureScreenshotWithPreview);
        behaviorContext->Method("CapturePassAttachment", &Bindings::CapturePassAttachment);

        // Profiling data...
        behaviorContext->Method("CapturePassTimestamp", &Bindings::CapturePassTimestamp);
        behaviorContext->Method("CaptureCpuFrameTime", &Bindings::CaptureCpuFrameTime);
        behaviorContext->Method("CapturePassPipelineStatistics", &Bindings::CapturePassPipelineStatistics);
        behaviorContext->Method("CaptureCpuProfilingStatistics", &Bindings::CaptureCpuProfilingStatistics);
        behaviorContext->Method("CaptureBenchmarkMetadata", &Bindings::CaptureBenchmarkMetadata);

        // Camera...
        behaviorContext->Method("ArcBallCameraController_SetCenter", &Bindings::ArcBallCameraController_SetCenter);
        behaviorContext->Method("ArcBallCameraController_SetPan", &Bindings::ArcBallCameraController_SetPan);
        behaviorContext->Method("ArcBallCameraController_SetDistance", &Bindings::ArcBallCameraController_SetDistance);
        behaviorContext->Method("ArcBallCameraController_SetHeading", &Bindings::ArcBallCameraController_SetHeading);
        behaviorContext->Method("ArcBallCameraController_SetPitch", &Bindings::ArcBallCameraController_SetPitch);
        behaviorContext->Method("NoClipCameraController_SetPosition", &Bindings::NoClipCameraController_SetPosition);
        behaviorContext->Method("NoClipCameraController_SetHeading", &Bindings::NoClipCameraController_SetHeading);
        behaviorContext->Method("NoClipCameraController_SetPitch", &Bindings::NoClipCameraController_SetPitch);
        behaviorContext->Method("NoClipCameraController_SetFov", &Bindings::NoClipCameraController_SetFov);

        // Asset System...
        AZ::BehaviorParameterOverrides expectedCountDetails = {"expectedCount", "Expected number of asset jobs; default=1", aznew AZ::BehaviorDefaultValue(1u)};
        const AZStd::array<AZ::BehaviorParameterOverrides, 2> assetTrackingExpectAssetArgs = {{ AZ::BehaviorParameterOverrides{}, expectedCountDetails }};

        behaviorContext->Method("AssetTracking_Start", &Bindings::AssetTracking_Start);
        behaviorContext->Method("AssetTracking_ExpectAsset", &Bindings::AssetTracking_ExpectAsset, assetTrackingExpectAssetArgs);
        behaviorContext->Method("AssetTracking_IdleUntilExpectedAssetsFinish", &Bindings::AssetTracking_IdleUntilExpectedAssetsFinish);
        behaviorContext->Method("AssetTracking_Stop", &Bindings::AssetTracking_Stop);
    }
} // namespace ScriptAutomation
