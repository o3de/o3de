/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/Feature/Utils/FrameCaptureTestBus.h>
#include <Atom/Utils/ImageComparison.h>
#include <ImageComparisonConfig.h>
#include <ImGui/ImGuiMessageBox.h>
#include <Atom/Utils/PngFile.h>
#include <imgui/imgui.h>

namespace ScriptAutomation
{
    struct ImageComparisonToleranceLevel;

    //! Collects data about each script run by the ScriptManager.
    //! This includes counting errors, checking screenshots, and providing a final report dialog.
    class ScriptReporter
    {
    public:
        // currently set to track the ScriptReport index and the ScreenshotTestInfo index.
        using ReportIndex = AZStd::pair<size_t, size_t>;

        static constexpr const char* TestResultsFolder = "TestResults";
        static constexpr const char* UserFolder = "user";

        //! Set the list of available tolerance levels, so the report can suggest an alternate level that matches the actual results.
        void SetAvailableToleranceLevels(const AZStd::vector<ImageComparisonToleranceLevel>& toleranceLevels);

        //! Clears all recorded data.
        void Reset();

        //! Invalidates the final results when displaying a report to the user. This can be used to highlight
        //! local changes that were made, and remind the user that these results should not be considered official.
        //! Use an empty string to clear the invalidation.
        void SetInvalidationMessage(const AZStd::string& message);

        //! Indicates that a new script has started processing.
        //! Any subsequent errors will be included as part of this script's report.
        void PushScript(const AZStd::string& scriptAssetPath);

        //! Indicates that the current script has finished executing.
        //! Any subsequent errors will be included as part of the prior script's report.
        void PopScript();

        //! Returns whether there are active processing scripts (i.e. more PushScript() calls than PopScript() calls)
        bool HasActiveScript() const;

        //! Indicates that a new screenshot is about to be captured.
        bool AddScreenshotTest(const AZStd::string& imageName);

        //! Check the latest screenshot using default thresholds.
        void CheckLatestScreenshot(const ImageComparisonToleranceLevel* comparisonPreset);

        //! Opens the script report dialog.
        //! This displays all the collected script reporting data, provides links to tools for analyzing data like
        //! viewing screenshot diffs. It can be left open during processing and will update in real-time.
        void OpenReportDialog();
        void HideReportDialog();

        //! Called every frame to update the ImGui dialog
        void TickImGui();

        //! Returns true if there are any errors or asserts in the script report
        bool HasErrorsAssertsInReport() const;

        struct ScriptResultsSummary
        {
            uint32_t m_totalAsserts = 0;
            uint32_t m_totalErrors = 0;
            uint32_t m_totalWarnings = 0;
            uint32_t m_totalScreenshotsCount = 0;
            uint32_t m_totalScreenshotsFailed = 0;
            uint32_t m_totalScreenshotWarnings = 0;
        };

        //! Displays the script results summary in ImGui.
        void DisplayScriptResultsSummary();

        //! Retrieves the current script result summary.
        const ScriptResultsSummary& GetScriptResultSummary() const;

        struct ImageComparisonResult
        {
            enum class ResultCode
            {
                None,
                Pass,
                FileNotFound,
                FileNotLoaded,
                WrongSize,
                WrongFormat,
                NullImageComparisonToleranceLevel,
                ThresholdExceeded
            };

            ResultCode m_resultCode = ResultCode::None;
            //! The diff score that was used for comparison..
            //! The diff score can be before or after filtering out visually imperceptible differences,
            //! depending on the tolerance level settings.
            //! See CalcImageDiffRms.
            float m_diffScore = 0.0f;

            AZStd::string GetSummaryString() const;
        };

        //! Records all the information about a screenshot comparison test.
        struct ScreenshotTestInfo
        {
            AZStd::string m_screenshotFilePath;                 //!< The full path where the screenshot will be generated.
            AZStd::string m_officialBaselineScreenshotFilePath; //!< The full path to the official baseline image that is checked into source control
            AZStd::string m_localBaselineScreenshotFilePath;    //!< The full path to a local baseline image that was established by the user
            ImageComparisonToleranceLevel m_toleranceLevel;     //!< Tolerance for checking against the official baseline image
            ImageComparisonResult m_officialComparisonResult;   //!< Result of comparing against the official baseline image, for reporting test failure
            ImageComparisonResult m_localComparisonResult;      //!< Result of comparing against a local baseline, for reporting warnings

            ScreenshotTestInfo(const AZStd::string& m_screenshotName);
        };

        //! Records all the information about a single test script.
        struct ScriptReport : public AZ::Debug::TraceMessageBus::Handler
        {
            ScriptReport()
            {
                AZ::Debug::TraceMessageBus::Handler::BusConnect();
            }

            ~ScriptReport()
            {
                AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            }

            bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, [[maybe_unused]] const char* message) override
            {
                ++m_assertCount;
                return false;
            }

            bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override
            {
                if (AZStd::string::npos == AzFramework::StringFunc::Find(message, "Screenshot check failed"))
                {
                    ++m_generalErrorCount;
                }
                else
                {
                    ++m_screenshotErrorCount;
                }

                return false;
            }

            bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override
            {
                if (AZStd::string::npos == AzFramework::StringFunc::Find(message, "Screenshot does not match the local baseline"))
                {
                    ++m_generalWarningCount;
                }
                else
                {
                    ++m_screenshotWarningCount;
                }

                return false;
            }

            AZStd::string m_scriptAssetPath;

            uint32_t m_assertCount = 0;

            uint32_t m_generalErrorCount = 0;
            uint32_t m_screenshotErrorCount = 0;

            uint32_t m_generalWarningCount = 0;
            uint32_t m_screenshotWarningCount = 0;

            AZStd::vector<ScreenshotTestInfo> m_screenshotTests;
        };

        const AZStd::vector<ScriptReport>& GetScriptReport() const { return m_scriptReports; }

        // For exporting test results
        void ExportTestResults();
        void ExportImageDiff(const char* filePath, const ScreenshotTestInfo& screenshotTest);
        AZStd::string ExportImageDiff(const ScriptReport& scriptReport, const ScreenshotTestInfo& screenshotTest);

        void SortScriptReports();

    private:
        static const ImGuiTreeNodeFlags FlagDefaultOpen = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
        static const ImGuiTreeNodeFlags FlagDefaultClosed = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        // Reports a script error using standard formatting that matches ScriptManager
        enum class TraceLevel
        {
            Error,
            Warning
        };

        // Controls which results are shown to the user
        // Must match static const char* DiplayOptions in .cpp file
        enum DisplayOption : int
        {
            AllResults,
            WarningsAndErrors,
            ErrorsOnly
        };
        
        // Controls how screenshot reports are sorted
        // Must match static const char* DiplayOptions in .cpp file
        enum SortOption : int
        {
            Unsorted,
            OfficialBaselineDiffScore,
            LocalBaselineDiffScore
        };

        static void ReportScriptError(const AZStd::string& message);
        static void ReportScriptWarning(const AZStd::string& message);
        static void ReportScriptIssue(const AZStd::string& message, TraceLevel traceLevel);
        static void ReportScreenshotComparisonIssue(const AZStd::string& message, const AZStd::string& expectedImageFilePath, const AZStd::string& actualImageFilePath, TraceLevel traceLevel);

        // Copies all captured screenshots to the local baseline folder. These can be used as an alternative to the central baseline for comparison.
        void UpdateAllLocalBaselineImages();

        // Copies a single captured screenshot to the local baseline folder. This can be used as an alternative to the central baseline for comparison.
        bool UpdateLocalBaselineImage(ScreenshotTestInfo& screenshotTest, bool showResultDialog);

        // Copies a single captured screenshot to the official baseline source folder.
        bool UpdateSourceBaselineImage(ScreenshotTestInfo& screenshotTest, bool showResultDialog);

        // Clears comparison result to passing with no errors or warnings
        void ClearImageComparisonResult(ImageComparisonResult& comparisonResult);

        // Show a message box to let the user know the results of updating local baseline images
        void ShowUpdateLocalBaselineResult(int successCount, int failureCount);

        const ImageComparisonToleranceLevel* FindBestToleranceLevel(float diffScore, bool filterImperceptibleDiffs) const;

        void ShowReportDialog();
        void ShowScreenshotTestInfoTreeNode(const AZStd::string& header, ScriptReport& scriptReport, ScreenshotTestInfo& screenshotResult);
        void ShowDiffButton(const char* buttonLabel, const AZStd::string& imagePathA, const AZStd::string& imagePathB);

        // Generates a path to the exported test results file.
        AZStd::string GenerateTimestamp() const;
        AZStd::string GenerateAndCreateExportedImageDiffPath(const ScriptReport& scriptReport, const ScreenshotTestInfo& screenshotTest) const;
        AZStd::string GenerateAndCreateExportedTestResultsPath() const;

        // Generates a diff between two images of the same size.
        void GenerateImageDiff(AZStd::span<const uint8_t> img1, AZStd::span<const uint8_t> img2, AZStd::vector<uint8_t>& buffer);

        ScriptReport* GetCurrentScriptReport();

        AZStd::string SeeConsole(uint32_t issueCount, const char* searchString);
        AZStd::string SeeBelow(uint32_t issueCount);
        void HighlightTextIf(bool shouldSet, ImVec4 color);
        void ResetTextHighlight();
        void HighlightTextFailedOrWarning(bool isFailed, bool isWarning);

        struct HighlightColorSettings
        {
            ImVec4 m_highlightPassed;
            ImVec4 m_highlightFailed;
            ImVec4 m_highlightWarning;

            void UpdateColorSettings();
        };

        using SortedReportIndexMap = AZStd::multimap<float, ReportIndex, AZStd::greater<float>>;

        SortedReportIndexMap m_reportsSortedByOfficialBaslineScore;
        SortedReportIndexMap m_reportsSortedByLocaBaslineScore;
        SortOption m_currentSortOption = SortOption::OfficialBaselineDiffScore;

        ImGuiMessageBox m_messageBox;

        AZStd::vector<ImageComparisonToleranceLevel> m_availableToleranceLevels;
        AZStd::string m_invalidationMessage;

        AZStd::vector<ScriptReport> m_scriptReports; //< Tracks errors for the current active script
        AZStd::vector<size_t> m_currentScriptIndexStack; //< Tracks which of the scripts in m_scriptReports is currently active
        bool m_showReportDialog = false;
        bool m_colorHasBeenSet = false;
        DisplayOption m_displayOption = DisplayOption::AllResults;
        bool m_forceShowUpdateButtons = false; //< By default, the "Update" buttons are visible only for failed screenshots. This forces them to be visible.
        bool m_forceShowExportPngDiffButtons = false; //< By default, "Export Png Diff" buttons are visible only for failed screenshots. This forces them to be visible.
        AZStd::string m_officialBaselineSourceFolder; //< Used for updating official baseline screenshots
        AZStd::string m_exportedTestResultsPath = "Click the 'Export Test Results' button."; //< Path to exported test results file (if exported).
        AZStd::string m_uniqueTimestamp;
        HighlightColorSettings m_highlightSettings;
        ScriptResultsSummary m_resultsSummary;

        // Flags set and used by ShowReportDialog()
        bool m_showAll;
        bool m_showWarnings;
    };

} // namespace ScriptAutomation
