/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <sstream>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>

#include <Atom/RHI/Factory.h>

#include <ScriptReporter.h>
#include <Utils.h>

namespace ScriptAutomation
{
    // Must match ScriptReporter::DisplayOption Enum
    static const char* DiplayOptions[] =
    {
        "All Results", "Warnings & Errors", "Errors Only",
    };
    
    static const char* SortOptions[] =
    {
        "Sort by Script", "Sort by Official Baseline Diff Score", "Sort by Local Baseline Diff Score",
    };

    AZStd::string ScriptReporter::ImageComparisonResult::GetSummaryString() const
    {
        AZStd::string resultString;

        if (m_resultCode == ResultCode::ThresholdExceeded || m_resultCode == ResultCode::Pass)
        {
            resultString = AZStd::string::format("Diff Score: %f", m_diffScore);
        }
        else if (m_resultCode == ResultCode::WrongSize)
        {
            resultString = "Wrong size";
        }
        else if (m_resultCode == ResultCode::FileNotFound)
        {
            resultString = "File not found";
        }
        else if (m_resultCode == ResultCode::FileNotLoaded)
        {
            resultString = "File load failed";
        }
        else if (m_resultCode == ResultCode::WrongFormat)
        {
            resultString = "Format is not supported";
        }
        else if (m_resultCode == ResultCode::NullImageComparisonToleranceLevel)
        {
            resultString = "ImageComparisonToleranceLevel not provided";
        }
        else if (m_resultCode == ResultCode::None)
        {
            // "None" could be the case if the results dialog is open while the script is running
            resultString = "No results";
        }
        else 
        {
            resultString = "Unhandled Image Comparison ResultCode";
            AZ_Assert(false, "Unhandled Image Comparison ResultCode");
        }

        return resultString;
    }

    void ScriptReporter::SetAvailableToleranceLevels(const AZStd::vector<ImageComparisonToleranceLevel>& toleranceLevels)
    {
        m_availableToleranceLevels = toleranceLevels;
    }

    void ScriptReporter::Reset()
    {
        m_scriptReports.clear();
        m_reportsSortedByOfficialBaslineScore.clear();
        m_reportsSortedByLocaBaslineScore.clear();
        m_currentScriptIndexStack.clear();
        m_invalidationMessage.clear();
        m_uniqueTimestamp = GenerateTimestamp();

    }

    void ScriptReporter::SetInvalidationMessage(const AZStd::string& message)
    {
        m_invalidationMessage = message;

        // Reporting this message here instead of when running the script so it won't show up as an error in the ImGui report.
        AZ_Error("ScriptReporter", m_invalidationMessage.empty(), "Subsequent test results will be invalid because '%s'", m_invalidationMessage.c_str());
    }

    void ScriptReporter::PushScript(const AZStd::string& scriptAssetPath)
    {
        if (GetCurrentScriptReport())
        {
            // Only the current script should listen for Trace Errors
            GetCurrentScriptReport()->BusDisconnect();
        }

        m_currentScriptIndexStack.push_back(m_scriptReports.size());
        m_scriptReports.emplace_back().m_scriptAssetPath = scriptAssetPath;
        m_scriptReports.back().BusConnect();
    }

    void ScriptReporter::PopScript()
    {
        AZ_Assert(GetCurrentScriptReport(), "There is no active script");

        if (GetCurrentScriptReport())
        {
            GetCurrentScriptReport()->BusDisconnect();
            m_currentScriptIndexStack.pop_back();
        }

        if (GetCurrentScriptReport())
        {
            // Make sure the newly restored current script is listening for Trace Errors
            GetCurrentScriptReport()->BusConnect();
        }
    }

    bool ScriptReporter::HasActiveScript() const
    {
        return !m_currentScriptIndexStack.empty();
    }

    ScriptReporter::ScreenshotTestInfo::ScreenshotTestInfo(const AZStd::string& screenshotName)
    {   
        AZ_Assert(!screenshotName.empty(), "The screenshot file name shouldn't be empty.");

        AZ::Render::FrameCapturePathOutcome pathOutcome;

        AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
            pathOutcome,
            &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath,
            screenshotName, true);

        if (pathOutcome.IsSuccess())
        {
            m_screenshotFilePath = pathOutcome.GetValue();
        }
        else
        {
            AZ_Error("ScriptReporter", false, "%s", pathOutcome.GetError().m_errorMessage.c_str());
        }

        AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
            pathOutcome,
            &AZ::Render::FrameCaptureTestRequestBus::Events::BuildOfficialBaselineFilePath,
            screenshotName, false);

        if (pathOutcome.IsSuccess())
        {
            m_officialBaselineScreenshotFilePath = pathOutcome.GetValue();
        }
        else
        {
            AZ_Error("ScriptReporter", false, "%s", pathOutcome.GetError().m_errorMessage.c_str());
        }

        AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
            pathOutcome,
            &AZ::Render::FrameCaptureTestRequestBus::Events::BuildLocalBaselineFilePath,
            screenshotName, true);

        if (pathOutcome.IsSuccess())
        {
            m_localBaselineScreenshotFilePath = pathOutcome.GetValue();
        }
        else
        {
            AZ_Error("ScriptReporter", false, "%s", pathOutcome.GetError().m_errorMessage.c_str());
        }
    }

    bool ScriptReporter::AddScreenshotTest(const AZStd::string& imageName)
    {
        AZ_Assert(GetCurrentScriptReport(), "There is no active script");

        ScreenshotTestInfo screenshotTestInfo(imageName);
        GetCurrentScriptReport()->m_screenshotTests.push_back(AZStd::move(screenshotTestInfo));

        return true;
    }

    void ScriptReporter::TickImGui()
    {
        if (m_showReportDialog)
        {
            ShowReportDialog();
        }
    }

    bool ScriptReporter::HasErrorsAssertsInReport() const
    {
        for (const ScriptReport& scriptReport : m_scriptReports)
        {
            if (scriptReport.m_assertCount > 0 || scriptReport.m_generalErrorCount > 0 || scriptReport.m_screenshotErrorCount > 0)
            {
                return true;
            }
        }

        return false;
    }

    void ScriptReporter::DisplayScriptResultsSummary()
    {
        ImGui::Separator();

        if (HasActiveScript())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightWarning);
            ImGui::Text("Script is running... (_ _)zzz");
            ImGui::PopStyleColor();
        }
        else if (m_resultsSummary.m_totalErrors > 0 || m_resultsSummary.m_totalAsserts > 0 || m_resultsSummary.m_totalScreenshotsFailed > 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightFailed);
            ImGui::Text("(>_<)  FAILED  (>_<)");
            ImGui::PopStyleColor();
        }
        else
        {
            if (m_invalidationMessage.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightPassed);
                ImGui::Text("\\(^_^)/  PASSED  \\(^_^)/");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("(-_-) INVALID ... but passed (-_-)");
            }
        }

        if (!m_invalidationMessage.empty())
        {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightFailed);
            ImGui::Text("(%s)", m_invalidationMessage.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        ImGui::Text("Test Script Count: %zu", m_scriptReports.size());

        HighlightTextIf(m_resultsSummary.m_totalAsserts > 0, m_highlightSettings.m_highlightFailed);
        ImGui::Text("Total Asserts:  %u %s", m_resultsSummary.m_totalAsserts, SeeConsole(m_resultsSummary.m_totalAsserts, "Trace::Assert").c_str());

        HighlightTextIf(m_resultsSummary.m_totalErrors > 0, m_highlightSettings.m_highlightFailed);
        ImGui::Text("Total Errors:   %u %s", m_resultsSummary.m_totalErrors, SeeConsole(m_resultsSummary.m_totalErrors, "Trace::Error").c_str());

        HighlightTextIf(m_resultsSummary.m_totalWarnings > 0, m_highlightSettings.m_highlightWarning);
        ImGui::Text("Total Warnings: %u %s", m_resultsSummary.m_totalWarnings, SeeConsole(m_resultsSummary.m_totalWarnings, "Trace::Warning").c_str());

        ResetTextHighlight();
        ImGui::Text("Total Screenshot Count: %u", m_resultsSummary.m_totalScreenshotsCount);

        HighlightTextIf(m_resultsSummary.m_totalScreenshotsFailed > 0, m_highlightSettings.m_highlightFailed);
        ImGui::Text("Total Screenshot Failures: %u %s", m_resultsSummary.m_totalScreenshotsFailed, SeeBelow(m_resultsSummary.m_totalScreenshotsFailed).c_str());

        HighlightTextIf(m_resultsSummary.m_totalScreenshotWarnings > 0, m_highlightSettings.m_highlightWarning);
        ImGui::Text("Total Screenshot Warnings: %u %s", m_resultsSummary.m_totalScreenshotWarnings, SeeBelow(m_resultsSummary.m_totalScreenshotWarnings).c_str());

        ResetTextHighlight();
    }

    const ScriptReporter::ScriptResultsSummary& ScriptReporter::GetScriptResultSummary() const
    {
        return m_resultsSummary;
    }

    void ScriptReporter::ShowDiffButton(const char* buttonLabel, const AZStd::string& imagePathA, const AZStd::string& imagePathB)
    {
        if (ImGui::Button(buttonLabel))
        {
            if (!Utils::RunDiffTool(imagePathA, imagePathB))
            {
                m_messageBox.OpenPopupMessage("Can't Diff", "Image diff is not supported on this platform, or the required diff tool is not installed.");
            }
        }
    }

    AZStd::string ScriptReporter::GenerateTimestamp() const
    {
        const AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
        const float timeFloat = AZStd::chrono::duration<float>(now.time_since_epoch()).count();
        return AZStd::string::format("%.4f", timeFloat);
    }

    AZStd::string ScriptReporter::GenerateAndCreateExportedImageDiffPath(const ScriptReport& scriptReport, const ScreenshotTestInfo& screenshotTest) const
    {
        const auto projectPath = AZ::Utils::GetProjectPath();
        AZStd::string imageDiffPath;
        AZStd::string scriptFilenameWithouExtension;
        AzFramework::StringFunc::Path::GetFileName(scriptReport.m_scriptAssetPath.c_str(), scriptFilenameWithouExtension);
        AzFramework::StringFunc::Path::StripExtension(scriptFilenameWithouExtension);

        AZStd::string screenshotFilenameWithouExtension;
        AzFramework::StringFunc::Path::GetFileName(screenshotTest.m_screenshotFilePath.c_str(), screenshotFilenameWithouExtension);
        AzFramework::StringFunc::Path::StripExtension(screenshotFilenameWithouExtension);

        AZStd::string imageDiffFilename = "imageDiff_" + scriptFilenameWithouExtension + "_" + screenshotFilenameWithouExtension + "_" + m_uniqueTimestamp + ".png";
        AzFramework::StringFunc::Path::Join(projectPath.c_str(), UserFolder, imageDiffPath);
        AzFramework::StringFunc::Path::Join(imageDiffPath.c_str(), TestResultsFolder, imageDiffPath);
        AzFramework::StringFunc::Path::Join(imageDiffPath.c_str(), imageDiffFilename.c_str(), imageDiffPath);

        AZStd::string imageDiffFolderPath;
        AzFramework::StringFunc::Path::GetFolderPath(imageDiffPath.c_str(), imageDiffFolderPath);
        auto io = AZ::IO::LocalFileIO::GetInstance();
        io->CreatePath(imageDiffFolderPath.c_str());

        return imageDiffPath;
    }

    const ImageComparisonToleranceLevel* ScriptReporter::FindBestToleranceLevel(float diffScore, bool filterImperceptibleDiffs) const
    {
        float thresholdChecked = 0.0f;
        bool ignoringMinorDiffs = false;
        for (const ImageComparisonToleranceLevel& level : m_availableToleranceLevels)
        {
            AZ_Assert(level.m_threshold > thresholdChecked || thresholdChecked == 0.0f, "Threshold values are not sequential");
            AZ_Assert(level.m_filterImperceptibleDiffs >= ignoringMinorDiffs, "filterImperceptibleDiffs values are not sequential");
            thresholdChecked = level.m_threshold;
            ignoringMinorDiffs = level.m_filterImperceptibleDiffs;

            if (filterImperceptibleDiffs <= level.m_filterImperceptibleDiffs && diffScore <= level.m_threshold)
            {
                return &level;
            }
        }

        return nullptr;
    }

    void ScriptReporter::ShowReportDialog()
    {
        if (ImGui::Begin("Script Results", &m_showReportDialog) && !m_scriptReports.empty())
        {
            m_highlightSettings.UpdateColorSettings();
            m_colorHasBeenSet = false;

            m_resultsSummary = ScriptResultsSummary();
            for (ScriptReport& scriptReport : m_scriptReports)
            {
                m_resultsSummary.m_totalAsserts += scriptReport.m_assertCount;

                // We don't include screenshot errors and warnings in these totals because those have their own line-items.
                m_resultsSummary.m_totalErrors += scriptReport.m_generalErrorCount;
                m_resultsSummary.m_totalWarnings += scriptReport.m_generalWarningCount;

                m_resultsSummary.m_totalScreenshotWarnings += scriptReport.m_screenshotWarningCount;
                m_resultsSummary.m_totalScreenshotsFailed += scriptReport.m_screenshotErrorCount;

                // This will catch any false-negatives that could occur if the screenshot failure error messages change without also updating ScriptReport::OnPreError()
                m_resultsSummary.m_totalScreenshotsCount += aznumeric_cast<uint32_t>(scriptReport.m_screenshotTests.size());
                for (ScreenshotTestInfo& screenshotTest : scriptReport.m_screenshotTests)
                {
                    if (screenshotTest.m_officialComparisonResult.m_resultCode != ImageComparisonResult::ResultCode::Pass &&
                        screenshotTest.m_officialComparisonResult.m_resultCode != ImageComparisonResult::ResultCode::None)
                    {
                        AZ_Assert(scriptReport.m_screenshotErrorCount > 0, "If screenshot comparison failed in any way, m_screenshotErrorCount should be non-zero.");
                    }
                }
            }

            DisplayScriptResultsSummary();

            ImGui::Text("Exported test results: %s", m_exportedTestResultsPath.c_str());
            if (ImGui::Button("Update All Local Baseline Images"))
            {
                m_messageBox.OpenPopupConfirmation(
                    "Update All Local Baseline Images",
                    "This will replace all local baseline images \n"
                    "with the images captured during this test run. \n"
                    "Are you sure?",
                    [this]() {
                        UpdateAllLocalBaselineImages();
                    });
            }
            if (ImGui::Button("Export Test Results"))
            {
                m_messageBox.OpenPopupConfirmation(
                    "Export Test Results",
                    "All test results will be exported \n"
                    "Proceed?",
                    [this]() {
                        ExportTestResults();
                    });
            }

            int displayOption = m_displayOption;
            ImGui::Combo("Display", &displayOption, DiplayOptions, AZ_ARRAY_SIZE(DiplayOptions));
            m_displayOption = (DisplayOption)displayOption;

            int sortOption = m_currentSortOption;
            ImGui::Combo("Sort Results", &sortOption, SortOptions, AZ_ARRAY_SIZE(SortOptions));
            m_currentSortOption = (SortOption)sortOption;

            ImGui::Checkbox("Force Show 'Update' Buttons", &m_forceShowUpdateButtons);
            ImGui::Checkbox("Force Show 'Export Png Diff' Buttons", &m_forceShowExportPngDiffButtons);

            m_showWarnings = (m_displayOption == DisplayOption::AllResults) || (m_displayOption == DisplayOption::WarningsAndErrors);
            m_showAll = (m_displayOption == DisplayOption::AllResults);

            ImGui::Separator();

            if (m_currentSortOption == SortOption::Unsorted)
            {
                for (ScriptReport& scriptReport : m_scriptReports)
                {
                    const bool scriptPassed = scriptReport.m_assertCount == 0 && scriptReport.m_generalErrorCount == 0 && scriptReport.m_screenshotErrorCount == 0;
                    const bool scriptHasWarnings = scriptReport.m_generalWarningCount > 0 || scriptReport.m_screenshotWarningCount > 0;

                    // Skip if tests passed and
                    //         1). have no warnings and we don't want to show successes OR
                    //         2). only have warnings and we don't want to show warnings
                    bool skipReport = scriptPassed && ((!scriptHasWarnings && !m_showAll) || (scriptHasWarnings && !m_showWarnings));

                    if (skipReport)
                    {
                        continue;
                    }

                    ImGuiTreeNodeFlags scriptNodeFlag = scriptPassed ? FlagDefaultClosed : FlagDefaultOpen;

                    AZStd::string header = AZStd::string::format("%s %s",
                        scriptPassed ? "PASSED" : "FAILED",
                        scriptReport.m_scriptAssetPath.c_str()
                    );

                    HighlightTextFailedOrWarning(!scriptPassed, scriptHasWarnings);

                    if (ImGui::TreeNodeEx(&scriptReport, scriptNodeFlag, "%s", header.c_str()))
                    {
                        ResetTextHighlight();

                        // Number of Asserts
                        HighlightTextIf(scriptReport.m_assertCount > 0, m_highlightSettings.m_highlightFailed);
                        if (m_showAll || scriptReport.m_assertCount > 0)
                        {
                            ImGui::Text("Asserts:  %u %s", scriptReport.m_assertCount, SeeConsole(scriptReport.m_assertCount, "Trace::Assert").c_str());
                        }

                        // Number of Errors
                        HighlightTextIf(scriptReport.m_generalErrorCount > 0, m_highlightSettings.m_highlightFailed);
                        if (m_showAll || scriptReport.m_generalErrorCount > 0)
                        {
                            ImGui::Text("Errors:   %u %s", scriptReport.m_generalErrorCount, SeeConsole(scriptReport.m_generalErrorCount, "Trace::Error").c_str());
                        }

                        // Number of Warnings
                        HighlightTextIf(scriptReport.m_generalWarningCount > 0, m_highlightSettings.m_highlightWarning);
                        if (m_showAll || (m_showWarnings && scriptReport.m_generalWarningCount > 0))
                        {
                            ImGui::Text("Warnings: %u %s", scriptReport.m_generalWarningCount, SeeConsole(scriptReport.m_generalWarningCount, "Trace::Warning").c_str());
                        }

                        ResetTextHighlight();

                        // Number of screenshots
                        if (m_showAll || scriptReport.m_screenshotErrorCount > 0 || (m_showWarnings && scriptReport.m_screenshotWarningCount > 0))
                        {
                            ImGui::Text("Screenshot Test Count: %zu", scriptReport.m_screenshotTests.size());
                        }

                        // Number of screenshot failures
                        HighlightTextIf(scriptReport.m_screenshotErrorCount > 0, m_highlightSettings.m_highlightFailed);
                        if (m_showAll || scriptReport.m_screenshotErrorCount > 0)
                        {
                            ImGui::Text("Screenshot Tests Failed: %u %s", scriptReport.m_screenshotErrorCount, SeeBelow(scriptReport.m_screenshotErrorCount).c_str());
                        }

                        // Number of screenshot warnings
                        HighlightTextIf(scriptReport.m_screenshotWarningCount > 0, m_highlightSettings.m_highlightWarning);
                        if (m_showAll || (m_showWarnings && scriptReport.m_screenshotWarningCount > 0))
                        {
                            ImGui::Text("Screenshot Warnings:     %u %s", scriptReport.m_screenshotWarningCount, SeeBelow(scriptReport.m_screenshotWarningCount).c_str());
                        }

                        ResetTextHighlight();

                        for (ScreenshotTestInfo& screenshotResult : scriptReport.m_screenshotTests)
                        {
                            const bool screenshotPassed = screenshotResult.m_officialComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::Pass;
                            const bool localBaselineWarning = screenshotResult.m_localComparisonResult.m_resultCode != ImageComparisonResult::ResultCode::Pass;

                            AZStd::string fileName;
                            AzFramework::StringFunc::Path::GetFullFileName(screenshotResult.m_screenshotFilePath.c_str(), fileName);

                            std::stringstream headerSummary;
                            if (!screenshotPassed)
                            {
                                headerSummary << "(" << screenshotResult.m_officialComparisonResult.GetSummaryString().c_str() << ") ";
                            }
                            if (localBaselineWarning)
                            {
                                headerSummary << "(Local Baseline Warning)";
                            }

                            AZStd::string screenshotHeader = AZStd::string::format("%s %s %s",
                                screenshotPassed ? "PASSED" : "FAILED",
                                fileName.c_str(),
                                headerSummary.str().c_str());

                            ShowScreenshotTestInfoTreeNode(screenshotHeader, scriptReport, screenshotResult);
                        }

                        ImGui::TreePop();
                    }

                    ResetTextHighlight();
                }
            }
            else
            {
                const SortedReportIndexMap* sortedReportMap = nullptr;
                if (m_currentSortOption == SortOption::OfficialBaselineDiffScore)
                {
                    sortedReportMap = &m_reportsSortedByOfficialBaslineScore;
                }
                else if (m_currentSortOption == SortOption::LocalBaselineDiffScore)
                {
                    sortedReportMap = &m_reportsSortedByLocaBaslineScore;
                }

                AZ_Assert(sortedReportMap, "Unhandled m_currentSortOption");

                if (sortedReportMap)
                {
                    for (const auto& [threshold, reportIndex] : *sortedReportMap)
                    {
                        ScriptReport& scriptReport = m_scriptReports[reportIndex.first];
                        ScreenshotTestInfo& screenshotResult = scriptReport.m_screenshotTests[reportIndex.second];

                        float diffScore = 0.0f;
                        if (m_currentSortOption == SortOption::OfficialBaselineDiffScore)
                        {
                            diffScore = screenshotResult.m_officialComparisonResult.m_diffScore;
                        }
                        else if (m_currentSortOption == SortOption::LocalBaselineDiffScore)
                        {
                            diffScore = screenshotResult.m_localComparisonResult.m_diffScore;
                        }

                        const bool screenshotPassed = screenshotResult.m_officialComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::Pass;

                        AZStd::string fileName;
                        AzFramework::StringFunc::Path::GetFullFileName(screenshotResult.m_screenshotFilePath.c_str(), fileName);

                        AZStd::string header = AZStd::string::format("%f %s %s %s '%s'",
                            diffScore,
                            screenshotPassed ? "PASSED" : "FAILED",
                            scriptReport.m_scriptAssetPath.c_str(),
                            fileName.c_str(),
                            screenshotResult.m_toleranceLevel.m_name.c_str());

                        ShowScreenshotTestInfoTreeNode(header, scriptReport, screenshotResult);
                    }
                }
            }
            ResetTextHighlight();

            // Repeat the m_invalidationMessage at the bottom as well, to make sure the user doesn't miss it.
            if (!m_invalidationMessage.empty())
            {
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightFailed);
                ImGui::Text("(%s)", m_invalidationMessage.c_str());
                ImGui::PopStyleColor();
            }
        }

        m_messageBox.TickPopup();

        ImGui::End();
    }

    void ScriptReporter::ShowScreenshotTestInfoTreeNode(const AZStd::string& header,  ScriptReport& scriptReport, ScreenshotTestInfo& screenshotResult)
    {
        const bool screenshotPassed = screenshotResult.m_officialComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::Pass;
        const bool localBaselineWarning = screenshotResult.m_localComparisonResult.m_resultCode != ImageComparisonResult::ResultCode::Pass;

        // Skip if tests passed without warnings and we don't want to show successes
        bool skipScreenshot = (screenshotPassed && !localBaselineWarning && !m_showAll);

        // Skip if we only have warnings only and we don't want to show warnings
        skipScreenshot = skipScreenshot || (screenshotPassed && localBaselineWarning && !m_showWarnings);

        if (skipScreenshot)
        {
            return;
        }
        ImGuiTreeNodeFlags screenshotNodeFlag = FlagDefaultClosed;
        HighlightTextFailedOrWarning(!screenshotPassed, localBaselineWarning);
        if (ImGui::TreeNodeEx(&screenshotResult, screenshotNodeFlag, "%s", header.c_str()))
        {
            ResetTextHighlight();

            ImGui::Text("Screenshot:        %s", screenshotResult.m_screenshotFilePath.c_str());

            ImGui::Spacing();

            HighlightTextIf(!screenshotPassed, m_highlightSettings.m_highlightFailed);

            ImGui::Text("Official Baseline: %s", screenshotResult.m_officialBaselineScreenshotFilePath.c_str());

            // Official Baseline Result
            ImGui::Indent();
            {
                ImGui::Text("%s", screenshotResult.m_officialComparisonResult.GetSummaryString().c_str());

                if (screenshotResult.m_officialComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::ThresholdExceeded ||
                    screenshotResult.m_officialComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::Pass)
                {
                    ImGui::Text("Used Tolerance: %s", screenshotResult.m_toleranceLevel.ToString().c_str());

                    const ImageComparisonToleranceLevel* suggestedTolerance = ScriptReporter::FindBestToleranceLevel(
                        screenshotResult.m_officialComparisonResult.m_diffScore,
                        screenshotResult.m_toleranceLevel.m_filterImperceptibleDiffs);

                    if (suggestedTolerance)
                    {
                        ImGui::Text("Suggested Tolerance: %s", suggestedTolerance->ToString().c_str());
                    }

                    if (screenshotResult.m_toleranceLevel.m_filterImperceptibleDiffs)
                    {
                        // This gives an idea of what the tolerance level would be if the imperceptible diffs were not filtered out.
                        const ImageComparisonToleranceLevel* unfilteredTolerance =
                            ScriptReporter::FindBestToleranceLevel(screenshotResult.m_officialComparisonResult.m_diffScore, false);

                        ImGui::Text(
                            "(Unfiltered Diff Score: %f%s)", screenshotResult.m_officialComparisonResult.m_diffScore,
                            unfilteredTolerance ? AZStd::string::format(" ~ '%s'", unfilteredTolerance->m_name.c_str()).c_str() : "");
                    }
                }

                ResetTextHighlight();

                ImGui::PushID("Official");
                ShowDiffButton("View Diff", screenshotResult.m_officialBaselineScreenshotFilePath, screenshotResult.m_screenshotFilePath);
                ImGui::PopID();

                if ((m_forceShowExportPngDiffButtons ||
                     screenshotResult.m_officialComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::ThresholdExceeded) &&
                    ImGui::Button("Export Png Diff"))
                {
                    const AZStd::string imageDiffPath = GenerateAndCreateExportedImageDiffPath(scriptReport, screenshotResult);
                    ExportImageDiff(imageDiffPath.c_str(), screenshotResult);
                    m_messageBox.OpenPopupMessage(
                        "Image Diff Exported Successfully",
                        AZStd::string::format("The image diff file was saved in %s", imageDiffPath.c_str()).c_str());
                }

                if ((!screenshotPassed || m_forceShowUpdateButtons) && ImGui::Button("Update##Official"))
                {
                    if (screenshotResult.m_localComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::FileNotFound)
                    {
                        UpdateSourceBaselineImage(screenshotResult, true);
                    }
                    else
                    {
                        m_messageBox.OpenPopupConfirmation(
                            "Update Official Baseline Image",
                            "This will replace the official baseline image \n"
                            "with the image captured during this test run. \n"
                            "Are you sure?",
                            // It's important to bind screenshotResult by reference because UpdateOfficialBaselineImage will update it
                            [this, &screenshotResult]()
                            {
                                UpdateSourceBaselineImage(screenshotResult, true);
                            });
                    }
                }
            }
            ImGui::Unindent();

            ImGui::Spacing();

            HighlightTextIf(localBaselineWarning, m_highlightSettings.m_highlightWarning);

            ImGui::Text("Local Baseline:    %s", screenshotResult.m_localBaselineScreenshotFilePath.c_str());

            // Local Baseline Result
            ImGui::Indent();
            {
                ImGui::Text("%s", screenshotResult.m_localComparisonResult.GetSummaryString().c_str());

                ResetTextHighlight();

                ImGui::PushID("Local");
                ShowDiffButton("View Diff", screenshotResult.m_localBaselineScreenshotFilePath, screenshotResult.m_screenshotFilePath);
                ImGui::PopID();

                if ((localBaselineWarning || m_forceShowUpdateButtons) && ImGui::Button("Update##Local"))
                {
                    if (screenshotResult.m_localComparisonResult.m_resultCode == ImageComparisonResult::ResultCode::FileNotFound)
                    {
                        UpdateLocalBaselineImage(screenshotResult, true);
                    }
                    else
                    {
                        m_messageBox.OpenPopupConfirmation(
                            "Update Local Baseline Image",
                            "This will replace the local baseline image \n"
                            "with the image captured during this test run. \n"
                            "Are you sure?",
                            // It's important to bind screenshotResult by reference because UpdateLocalBaselineImage will update it
                            [this, &screenshotResult]()
                            {
                                UpdateLocalBaselineImage(screenshotResult, true);
                            });
                    }
                }
            }
            ImGui::Unindent();

            ImGui::Spacing();

            ResetTextHighlight();

            ImGui::TreePop();
        }
    }

    void ScriptReporter::OpenReportDialog()
    {
        m_showReportDialog = true;
    }

    void ScriptReporter::HideReportDialog()
    {
        m_showReportDialog = false;
    }

    ScriptReporter::ScriptReport* ScriptReporter::GetCurrentScriptReport()
    {
        if (!m_currentScriptIndexStack.empty())
        {
            return &m_scriptReports[m_currentScriptIndexStack.back()];
        }
        else
        {
            return nullptr;
        }
    }

    AZStd::string ScriptReporter::SeeConsole(uint32_t issueCount, const char* searchString)
    {
        if (issueCount == 0)
        {
            return AZStd::string{};
        }
        else
        {
            return AZStd::string::format("(See \"%s\" messages in console output)", searchString);
        }
    }

    AZStd::string ScriptReporter::SeeBelow(uint32_t issueCount)
    {
        if (issueCount == 0)
        {
            return AZStd::string{};
        }
        else
        {
            return AZStd::string::format("(See below)");
        }
    }

    void ScriptReporter::HighlightTextIf(bool shouldSet, ImVec4 color)
    {
        if (m_colorHasBeenSet)
        {
            ImGui::PopStyleColor();
            m_colorHasBeenSet = false;
        }

        if (shouldSet)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            m_colorHasBeenSet = true;
        }
    }

    void ScriptReporter::ResetTextHighlight()
    {
        if (m_colorHasBeenSet)
        {
            ImGui::PopStyleColor();
            m_colorHasBeenSet = false;
        }
    }

    void ScriptReporter::HighlightTextFailedOrWarning(bool isFailed, bool isWarning)
    {
        if (m_colorHasBeenSet)
        {
            ImGui::PopStyleColor();
            m_colorHasBeenSet = false;
        }

        if (isFailed)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightFailed);
            m_colorHasBeenSet = true;
        }
        else if (isWarning)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_highlightSettings.m_highlightWarning);
            m_colorHasBeenSet = true;
        }
    }

    void ScriptReporter::SortScriptReports()
    {
        for (size_t i = 0; i < m_scriptReports.size(); ++i)
        {
            const AZStd::vector<ScriptReporter::ScreenshotTestInfo>& screenshotTestInfos = m_scriptReports[i].m_screenshotTests;
            for (size_t j = 0; j < screenshotTestInfos.size(); ++j)
            {
                m_reportsSortedByOfficialBaslineScore.insert(AZStd::pair<float, ReportIndex>(
                    screenshotTestInfos[j].m_officialComparisonResult.m_diffScore,
                    ReportIndex{ i, j }));

                m_reportsSortedByLocaBaslineScore.insert(AZStd::pair<float, ReportIndex>(
                    screenshotTestInfos[j].m_localComparisonResult.m_diffScore,
                    ReportIndex{ i, j }));
            }
        }
    }

    void ScriptReporter::ReportScriptError([[maybe_unused]] const AZStd::string& message)
    {
        AZ_Error("ScriptReporter", false, "Script: %s", message.c_str());
    }

    void ScriptReporter::ReportScriptWarning([[maybe_unused]] const AZStd::string& message)
    {
        AZ_Warning("ScriptReporter", false, "Script: %s", message.c_str());
    }

    void ScriptReporter::ReportScriptIssue(const AZStd::string& message, TraceLevel traceLevel)
    {
        switch (traceLevel)
        {
        case TraceLevel::Error:
            ReportScriptError(message);
            break;
        case TraceLevel::Warning:
            ReportScriptWarning(message);
            break;
        default:
            AZ_Assert(false, "Unhandled TraceLevel");
        }
    }

    void ScriptReporter::ReportScreenshotComparisonIssue(const AZStd::string& message, const AZStd::string& expectedImageFilePath, const AZStd::string& actualImageFilePath, TraceLevel traceLevel)
    {
        AZStd::string fullMessage = AZStd::string::format("%s\n    Expected: '%s'\n    Actual:   '%s'",
            message.c_str(),
            expectedImageFilePath.c_str(),
            actualImageFilePath.c_str());

        ReportScriptIssue(fullMessage, traceLevel);
    }

    void ScriptReporter::UpdateAllLocalBaselineImages()
    {
        int failureCount = 0;
        int successCount = 0;

        for (ScriptReport& report : m_scriptReports)
        {
            for (ScreenshotTestInfo& screenshotTest : report.m_screenshotTests)
            {
                if (UpdateLocalBaselineImage(screenshotTest, false))
                {
                    successCount++;
                }
                else
                {
                    failureCount++;
                }
            }
        }

        ShowUpdateLocalBaselineResult(successCount, failureCount);
    }
    
    bool ScriptReporter::UpdateLocalBaselineImage(ScreenshotTestInfo& screenshotTest, bool showResultDialog)
    {
        const AZStd::string destinationFile = screenshotTest.m_localBaselineScreenshotFilePath;

        AZStd::string destinationFolder = destinationFile;
        AzFramework::StringFunc::Path::StripFullName(destinationFolder);

        bool failed = false;

        if (!AZ::IO::LocalFileIO::GetInstance()->CreatePath(destinationFolder.c_str()))
        {
            failed = true;
            AZ_Error("ScriptReporter", false, "Failed to create folder '%s'.", destinationFolder.c_str());
        }

        if (!AZ::IO::LocalFileIO::GetInstance()->Copy(screenshotTest.m_screenshotFilePath.c_str(), destinationFile.c_str()))
        {
            failed = true;
            AZ_Error("ScriptReporter", false, "Failed to copy '%s' to '%s'.", screenshotTest.m_screenshotFilePath.c_str(), destinationFile.c_str());
        }

        if (!failed)
        {
            // Since we just replaced the baseline image, we can update this screenshot test result as an exact match.
            // This will update the ImGui report dialog by the next frame.
            ClearImageComparisonResult(screenshotTest.m_localComparisonResult);
        }

        if (showResultDialog)
        {
            int successCount = !failed;
            int failureCount = failed;
            ShowUpdateLocalBaselineResult(successCount, failureCount);
        }

        return !failed;
    }

    bool ScriptReporter::UpdateSourceBaselineImage(ScreenshotTestInfo& screenshotTest, bool showResultDialog)
    {
        bool success = true;
        auto io = AZ::IO::LocalFileIO::GetInstance();

        // Get source folder
        if (m_officialBaselineSourceFolder.empty())
        {
            m_officialBaselineSourceFolder = (AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "Scripts" / "ExpectedScreenshots").String();

            if (!io->Exists(m_officialBaselineSourceFolder.c_str()))
            {
                AZ_Error("ScriptReporter", false, "Could not find source folder '%s'. Copying to source baseline can only be used on dev platforms.", m_officialBaselineSourceFolder.c_str());
                m_officialBaselineSourceFolder.clear();
                success = false;
            }
        }

        // Get official cache baseline file
        const AZStd::string cacheFilePath = screenshotTest.m_officialBaselineScreenshotFilePath;

        // Divide cache file path into components to we can access the file name and the parent folder
        AZStd::fixed_vector<AZ::IO::FixedMaxPathString, 16> reversePathComponents;
        auto GatherPathSegments = [&reversePathComponents](AZStd::string_view token)
        {
            reversePathComponents.emplace_back(token);
        };
        AzFramework::StringFunc::TokenizeVisitorReverse(cacheFilePath, GatherPathSegments, "/\\");

        // Source folder path
        // ".../AtomSampleViewer/Scripts/ExpectedScreenshots/" + "MyTestFolder/"
        AZStd::string sourceFolderPath = AZStd::string::format("%s\\%s", m_officialBaselineSourceFolder.c_str(), reversePathComponents[1].c_str());

        // Source file path
        // ".../AtomSampleViewer/Scripts/ExpectedScreenshots/MyTestFolder/" + "MyTest.png"
        AZStd::string sourceFilePath = AZStd::string::format("%s\\%s", sourceFolderPath.c_str(), reversePathComponents[0].c_str());

        // Create parent folder if it doesn't exist
        if (success && !io->CreatePath(sourceFolderPath.c_str()))
        {
            success = false;
            AZ_Error("ScriptReporter", false, "Failed to create folder '%s'.", sourceFolderPath.c_str());
        }

        // Replace source screenshot with new result
        if (success && !io->Copy(screenshotTest.m_screenshotFilePath.c_str(), sourceFilePath.c_str()))
        {
            success = false;
            AZ_Error("ScriptReporter", false, "Failed to copy '%s' to '%s'.", screenshotTest.m_screenshotFilePath.c_str(), sourceFilePath.c_str());
        }

        if (success)
        {
            // Since we just replaced the baseline image, we can update this screenshot test result as an exact match.
            // This will update the ImGui report dialog by the next frame.
            ClearImageComparisonResult(screenshotTest.m_officialComparisonResult);
        }

        if (showResultDialog)
        {
            AZStd::string message = "Destination: " + sourceFilePath + "\n";
            message += success
                ? AZStd::string::format("Copy successful!.\n")
                : AZStd::string::format("Copy failed!\n");

            m_messageBox.OpenPopupMessage("Update Baseline Image(s) Result", message);
        }

        return success;
    }

    void ScriptReporter::ClearImageComparisonResult(ImageComparisonResult& comparisonResult)
    {
        comparisonResult.m_resultCode = ImageComparisonResult::ResultCode::Pass;
        comparisonResult.m_diffScore = 0.0f;
    }

    void ScriptReporter::ShowUpdateLocalBaselineResult(int successCount, int failureCount)
    {
        AZStd::string message;
        if (failureCount == 0 && successCount == 0)
        {
            message = "No screenshots found.";
        }
        else
        {
            AZ::Render::FrameCapturePathOutcome pathOutcome;

            AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                pathOutcome,
                &AZ::Render::FrameCaptureTestRequestBus::Events::BuildScreenshotFilePath,
                "", true);

            AZ_Error("ScriptReporter", pathOutcome.IsSuccess(), "%s", pathOutcome.GetError().m_errorMessage.c_str());

            AZStd::string localBaselineFolder = pathOutcome.IsSuccess() ? pathOutcome.GetValue() : "";
            message = "Destination: " + localBaselineFolder + "\n";

            if (successCount > 0)
            {
                message += AZStd::string::format("Successfully copied %d files.\n", successCount);
            }
            if (failureCount > 0)
            {
                message += AZStd::string::format("Failed to copy %d files.\n", failureCount);
            }
        }

        m_messageBox.OpenPopupMessage("Update Baseline Image(s) Result", message);
    }

    void ScriptReporter::CheckLatestScreenshot(const ImageComparisonToleranceLevel* toleranceLevel)
    {
        AZ_Assert(GetCurrentScriptReport(), "There is no active script");

        if (GetCurrentScriptReport() == nullptr || GetCurrentScriptReport()->m_screenshotTests.empty())
        {
            ReportScriptError("CheckLatestScreenshot() did not find any screenshots to check.");
            return;
        }

        ScreenshotTestInfo& screenshotTestInfo = GetCurrentScriptReport()->m_screenshotTests.back();

        if (toleranceLevel == nullptr)
        {
            screenshotTestInfo.m_officialComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::NullImageComparisonToleranceLevel;
            ReportScriptError("Screenshot check failed. No ImageComparisonToleranceLevel provided.");
            return;
        }

        auto io = AZ::IO::LocalFileIO::GetInstance();
        screenshotTestInfo.m_toleranceLevel = *toleranceLevel;
        static constexpr float ImperceptibleDiffFilter = 0.01;

        if (screenshotTestInfo.m_officialBaselineScreenshotFilePath.empty()
            || !io->Exists(screenshotTestInfo.m_officialBaselineScreenshotFilePath.c_str()))
        {
            ReportScriptError(AZStd::string::format("Screenshot check failed. Could not determine expected screenshot path for '%s'", screenshotTestInfo.m_screenshotFilePath.c_str()));
            screenshotTestInfo.m_officialComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::FileNotFound;
        }
        else
        {
            AZ::Render::FrameCaptureComparisonOutcome compOutcome;
            AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                compOutcome,
                &AZ::Render::FrameCaptureTestRequestBus::Events::CompareScreenshots,
                screenshotTestInfo.m_screenshotFilePath,
                screenshotTestInfo.m_officialBaselineScreenshotFilePath,
                ImperceptibleDiffFilter
            );

            screenshotTestInfo.m_officialComparisonResult.m_diffScore = 0.0;

            if (compOutcome.IsSuccess())
            {
                screenshotTestInfo.m_officialComparisonResult.m_diffScore = toleranceLevel->m_filterImperceptibleDiffs
                    ? compOutcome.GetValue().m_diffScore
                    : compOutcome.GetValue().m_filteredDiffScore;

                if (screenshotTestInfo.m_officialComparisonResult.m_diffScore <= toleranceLevel->m_threshold)
                {
                    screenshotTestInfo.m_officialComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::Pass;
                }
                else
                {
                    // Be aware there is an automation test script that looks for the "Screenshot check failed. Diff score" string text to report failures.
                    // If you change this message, be sure to update the associated tests as well located here: "C:/path/to/Lumberyard/AtomSampleViewer/Standalone/PythonTests"
                    ReportScreenshotComparisonIssue(
                        AZStd::string::format("Screenshot check failed. Diff score %f exceeds threshold of %f ('%s').",
                            screenshotTestInfo.m_officialComparisonResult.m_diffScore, toleranceLevel->m_threshold, toleranceLevel->m_name.c_str()),
                        screenshotTestInfo.m_officialBaselineScreenshotFilePath,
                        screenshotTestInfo.m_screenshotFilePath,
                        TraceLevel::Error);
                    screenshotTestInfo.m_officialComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::ThresholdExceeded;
                }
            }
        }

        if (screenshotTestInfo.m_localBaselineScreenshotFilePath.empty()
            || !io->Exists(screenshotTestInfo.m_localBaselineScreenshotFilePath.c_str()))
        {
            ReportScriptWarning(AZStd::string::format("Screenshot check failed. Could not determine local baseline screenshot path for '%s'", screenshotTestInfo.m_screenshotFilePath.c_str()));
            screenshotTestInfo.m_localComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::FileNotFound;
        }
        else
        {
            // Local screenshots should be expected match 100% every time, otherwise warnings are reported. This will help developers track and investigate changes,
            // for example if they make local changes that impact some unrelated AtomSampleViewer sample in an unexpected way, they will see a warning about this.
            AZ::Render::FrameCaptureComparisonOutcome compOutcome;
            AZ::Render::FrameCaptureTestRequestBus::BroadcastResult(
                compOutcome,
                &AZ::Render::FrameCaptureTestRequestBus::Events::CompareScreenshots,
                screenshotTestInfo.m_screenshotFilePath,
                screenshotTestInfo.m_localBaselineScreenshotFilePath,
                ImperceptibleDiffFilter
            );

            screenshotTestInfo.m_localComparisonResult.m_diffScore = 0.0f;

            if (compOutcome.IsSuccess())
            {
                screenshotTestInfo.m_localComparisonResult.m_diffScore = compOutcome.GetValue().m_diffScore;

                if (screenshotTestInfo.m_localComparisonResult.m_diffScore == 0.0f)
                {
                    screenshotTestInfo.m_localComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::Pass;
                }
                else
                {
                    ReportScreenshotComparisonIssue(
                        AZStd::string::format("Screenshot check failed. Screenshot does not match the local baseline; something has changed. Diff score is %f.", screenshotTestInfo.m_localComparisonResult.m_diffScore),
                        screenshotTestInfo.m_localBaselineScreenshotFilePath,
                        screenshotTestInfo.m_screenshotFilePath,
                        TraceLevel::Warning);
                    screenshotTestInfo.m_localComparisonResult.m_resultCode = ImageComparisonResult::ResultCode::ThresholdExceeded;
                }
            }
        }
    }

    void ScriptReporter::ExportTestResults()
    {
        m_exportedTestResultsPath = GenerateAndCreateExportedTestResultsPath();
        for (const ScriptReport& scriptReport : m_scriptReports)
        {
            const AZStd::string assertLogLine = AZStd::string::format("Asserts: %u \n", scriptReport.m_assertCount);
            const AZStd::string errorsLogLine = AZStd::string::format("Errors: %u \n", scriptReport.m_generalErrorCount);
            const AZStd::string warningsLogLine = AZStd::string::format("Warnings: %u \n", scriptReport.m_generalWarningCount);
            const AZStd::string screenshotErrorsLogLine = AZStd::string::format("Screenshot errors: %u \n", scriptReport.m_screenshotErrorCount);
            const AZStd::string screenshotWarningsLogLine = AZStd::string::format("Screenshot warnings: %u \n", scriptReport.m_screenshotWarningCount);
            const AZStd::string failedScreenshotsLogLine = "\nScreenshot test info below.\n";
            
            AZ::IO::HandleType logHandle;
            auto io = AZ::IO::LocalFileIO::GetInstance();
            if (io->Open(m_exportedTestResultsPath.c_str(), AZ::IO::OpenMode::ModeWrite, logHandle))
            {
                io->Write(logHandle, assertLogLine.c_str(), assertLogLine.size());
                io->Write(logHandle, errorsLogLine.c_str(), errorsLogLine.size());
                io->Write(logHandle, warningsLogLine.c_str(), warningsLogLine.size());
                io->Write(logHandle, screenshotErrorsLogLine.c_str(), screenshotErrorsLogLine.size());
                io->Write(logHandle, screenshotWarningsLogLine.c_str(), screenshotWarningsLogLine.size());
                io->Write(logHandle, failedScreenshotsLogLine.c_str(), failedScreenshotsLogLine.size());

                for (const ScreenshotTestInfo& screenshotTest : scriptReport.m_screenshotTests)
                {
                    const AZStd::string screenshotPath = AZStd::string::format("Test screenshot path: %s \n", screenshotTest.m_screenshotFilePath.c_str());
                    const AZStd::string officialBaselineScreenshotPath = AZStd::string::format("Official baseline screenshot path: %s \n", screenshotTest.m_officialBaselineScreenshotFilePath.c_str());
                    const AZStd::string toleranceLevelLogLine = AZStd::string::format("Tolerance level: %s \n", screenshotTest.m_toleranceLevel.ToString().c_str());
                    const AZStd::string officialComparisonLogLine = AZStd::string::format("Image comparison result: %s \n", screenshotTest.m_officialComparisonResult.GetSummaryString().c_str());

                    io->Write(logHandle, toleranceLevelLogLine.c_str(), toleranceLevelLogLine.size());
                    io->Write(logHandle, officialComparisonLogLine.c_str(), officialComparisonLogLine.size());
                }
                io->Close(logHandle);
            }
            m_messageBox.OpenPopupMessage("Exported test results", AZStd::string::format("Results exported to %s", m_exportedTestResultsPath.c_str()));
            AZ_Printf("Test results exported to %s \n", m_exportedTestResultsPath.c_str());
        }
    }

    void ScriptReporter::ExportImageDiff(const char* filePath, const ScreenshotTestInfo& screenshotTestInfo)
    {
        using namespace AZ::Utils;
        PngFile officialBaseline = PngFile::Load(screenshotTestInfo.m_officialBaselineScreenshotFilePath.c_str());
        PngFile actualScreenshot = PngFile::Load(screenshotTestInfo.m_screenshotFilePath.c_str());

        const size_t bufferSize = officialBaseline.GetBuffer().size();

        AZStd::vector<uint8_t> diffBuffer = AZStd::vector<uint8_t>(bufferSize);
        GenerateImageDiff(officialBaseline.GetBuffer(), actualScreenshot.GetBuffer(), diffBuffer);

        AZStd::vector<uint8_t> buffer = AZStd::vector<uint8_t>(bufferSize * 3);
        memcpy(buffer.data(), officialBaseline.GetBuffer().data(), bufferSize);
        memcpy(buffer.data() + bufferSize, actualScreenshot.GetBuffer().data(), bufferSize);
        memcpy(buffer.data() + bufferSize * 2, diffBuffer.data(), bufferSize);

        PngFile imageDiff = PngFile::Create(AZ::RHI::Size(officialBaseline.GetWidth(), officialBaseline.GetHeight() * 3, 1), AZ::RHI::Format::R8G8B8A8_UNORM, buffer);
        imageDiff.Save(filePath);
    }

    AZStd::string ScriptReporter::ExportImageDiff(const ScriptReport& scriptReport, const ScreenshotTestInfo& screenshotTest)
    {
        const AZStd::string imageDiffPath = GenerateAndCreateExportedImageDiffPath(scriptReport, screenshotTest);
        ExportImageDiff(imageDiffPath.c_str(), screenshotTest);
        return imageDiffPath;
    }

    AZStd::string ScriptReporter::GenerateAndCreateExportedTestResultsPath() const
    {
        // Setup our variables for the exported test results path and .txt file.
        const auto projectPath = AZ::Utils::GetProjectPath();
        const AZStd::string exportFileName = AZStd::string::format("exportedTestResults_%s.txt", m_uniqueTimestamp.c_str());
        AZStd::string exportTestResultsFolder;
        AzFramework::StringFunc::Path::Join(projectPath.c_str(), TestResultsFolder, exportTestResultsFolder);

        // Create the exported test results path & return .txt file path.
        auto io = AZ::IO::LocalFileIO::GetInstance();
        io->CreatePath(exportTestResultsFolder.c_str());
        AZStd::string exportFile;
        AzFramework::StringFunc::Path::Join(exportTestResultsFolder.c_str(), exportFileName.c_str(), exportFile);

        return exportFile;
    }

    void ScriptReporter::GenerateImageDiff(AZStd::span<const uint8_t> img1, AZStd::span<const uint8_t> img2, AZStd::vector<uint8_t>& buffer)
    {
        static constexpr size_t BytesPerPixel = 4;
        static constexpr float MinDiffFilter = 0.01;
        static constexpr uint8_t DefaultPixelValue = 122;

        memset(buffer.data(), DefaultPixelValue, buffer.size() * sizeof(uint8_t));

        for (size_t i = 0; i < img1.size(); i += BytesPerPixel)
        {
            const int16_t maxDiff = AZ::Utils::CalcMaxChannelDifference(img1, img2, i);

            if (maxDiff / 255.0f > MinDiffFilter)
            {
                buffer[i] = aznumeric_cast<uint8_t>(maxDiff);
                buffer[i + 1] = 0;
                buffer[i + 2] = 0;
            }
            buffer[i + 3] = 255;
        }
    }

    void ScriptReporter::HighlightColorSettings::UpdateColorSettings()
    {
        const ImVec4& bgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        const bool isDarkStyle = bgColor.x < 0.2 && bgColor.y < 0.2 && bgColor.z < 0.2;
        m_highlightPassed = isDarkStyle ? ImVec4{ 0.5, 1, 0.5, 1 } : ImVec4{ 0, 0.75, 0, 1 };
        m_highlightFailed = isDarkStyle ? ImVec4{ 1, 0.5, 0.5, 1 } : ImVec4{ 0.75, 0, 0, 1 };
        m_highlightWarning = isDarkStyle ? ImVec4{ 1, 1, 0.5, 1 } : ImVec4{ 0.5, 0.5, 0, 1 };
    }

} // namespace ScriptAutomation
