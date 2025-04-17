/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/chrono/chrono.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/UnitTesting/UnitTestBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_EQ(LHS, RHS)\
    if(!(LHS == RHS)) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) == (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_NE(LHS, RHS)\
    if(!(LHS != RHS)) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) != (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GT(LHS, RHS)\
    if(!(LHS > RHS)) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) > (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_GT(LHS, RHS)\
    if(!(LHS.IsGreaterThan(RHS))) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) <= (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GE(LHS, RHS)\
    if(!(LHS >= RHS)) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) >= (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_GE(LHS, RHS)\
    if(!(LHS.IsGreaterEqualThan(RHS))) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) <= (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LT(LHS, RHS)\
    if(!(LHS < RHS)) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) < (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_LT(LHS, RHS)\
    if(!(LHS.IsLessThan(RHS))) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) <= (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LE(LHS, RHS)\
    if(!(LHS <= RHS)) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) <= (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_LE(LHS, RHS)\
    if(!(LHS.IsLessEqualThan(RHS))) { AddFailure(AZStd::string::format("Error | Expected (candidate: %s) <= (reference: %s): %s", Datum(LHS).ToString().data(), Datum(RHS).ToString().data(), report.data())); }

namespace ScriptCanvasEditor
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::UnitTesting;

    constexpr const char* k_unitTestDirPathRelative = "@gemroot:ScriptCanvasTesting@/Assets/ScriptCanvas/UnitTests";

    class Reporter
        : public Bus::Handler
        , public AZ::EntityBus::Handler
        , private ScriptCanvas::ExecutionNotificationsBus::Handler // only to support IsGraphObserved, ErrorHandling in Testing
    {
    public:
        Reporter();
        Reporter(const AZ::EntityId& entityID);
        ~Reporter();

        void CollectPerformanceTiming();

        bool ExpectsParseError() const;

        bool ExpectsRuntimeFailure() const;

        void FinishReport();

        void FinishReport(const bool inErrorState);

        const AZStd::vector<Report>& GetCheckpoints() const;

        ExecutionConfiguration GetExecutionConfiguration() const;

        ExecutionMode GetExecutionMode() const;

        const AZStd::vector<Report>& GetFailure() const;

        const AZ::Data::AssetId& GetGraph() const;

        AZStd::sys_time_t GetParseDuration() const;

        const Execution::PerformanceTrackingReport& GetPerformanceReport() const;

        const AZStd::vector<Report>& GetSuccess() const;

        AZStd::sys_time_t GetTranslateDuration() const;

        const AZ::IO::Path& GetFilePath() const;

        bool IsActivated() const;

        bool IsCompiled() const;

        bool IsComplete() const;

        bool IsDeactivated() const;

        bool IsErrorFree() const;

        bool IsGraphLoaded() const;

        bool IsParseAttemptMade() const;

        bool IsProcessOnly() const;

        bool IsReportFinished() const;

        void MarkCompiled();

        void MarkExpectParseError();

        void MarkExpectRuntimeFailure();

        void MarkGraphLoaded();

        void MarkParseAttemptMade();

        bool operator==(const Reporter& other) const;

        void SetDurations(AZStd::sys_time_t parse, AZStd::sys_time_t translate);

        void SetExecutionConfiguration(ExecutionConfiguration configuration);

        void SetExecutionMode(ExecutionMode mode);

        void SetEntity(const AZ::EntityId& entityID);

        void SetGraph(const AZ::Data::AssetId& graphID);

        void SetProcessOnly(bool processOnly);

        void SetFilePath(const AZ::IO::PathView& filePath);

        // Bus::Handler
        void AddFailure(const Report& report) override;

        void AddSuccess(const Report& report) override;

        void Checkpoint(const Report& report) override;

        void ExpectFalse(const bool value, const Report& report) override;

        void ExpectTrue(const bool value, const Report& report) override;

        void MarkComplete(const Report& report) override;

        SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_OVERRIDES(ExpectEqual);

        SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_OVERRIDES(ExpectNotEqual);

        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectGreaterThan);

        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectGreaterThanEqual);

        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectLessThan);

        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectLessThanEqual);

    protected:
        void OnEntityActivated(const AZ::EntityId&) override;
        void OnEntityDeactivated(const AZ::EntityId&) override;

        //////////////////////////////////////////////////////////////////////////
        // ExecutionNotificationsBus
        // IsGraphObserved is needed for unit testing code support only, no other event is
        void GraphActivated(const GraphActivation&) override {}
        void GraphDeactivated(const GraphDeactivation&) override {}
        bool IsGraphObserved(const AZ::EntityId& entityId, const GraphIdentifier& identifier) override;
        bool IsVariableObserved(const VariableId&) override { return false; }
        void NodeSignaledOutput(const OutputSignal&) override {}
        void NodeSignaledInput(const InputSignal&) override {}
        void GraphSignaledReturn(const ReturnSignal&) override {};
        void NodeStateUpdated(const NodeStateChange&) override {}
        void RuntimeError(const ExecutionState& executionState, const AZStd::string_view& description) override;
        void VariableChanged(const VariableChange&) override {}
        void AnnotateNode(const AnnotateNodeSignal&) override {}

    private:
        bool m_expectParseError = false;
        bool m_expectsRuntimeError = false;
        bool m_graphIsCompiled = false;
        bool m_graphIsActivated = false;
        bool m_graphIsDeactivated = false;
        bool m_graphIsComplete = false;
        bool m_isGraphLoaded = false;
        bool m_isParseAttemptMade = false;
        bool m_isReportFinished = false;
        bool m_processOnly = false;
        AZ::IO::Path m_filePath;
        ExecutionConfiguration m_configuration = ExecutionConfiguration::Release;
        ExecutionMode m_mode;
        Execution::PerformanceTrackingReport m_performanceReport;
        AZStd::sys_time_t m_parseDuration;
        AZStd::sys_time_t m_translationDuration;
        AZ::Data::AssetId m_graph;
        AZ::EntityId m_entityId;
        AZStd::vector<Report> m_checkpoints;
        AZStd::vector<Report> m_failures;
        AZStd::vector<Report> m_successes;
#if defined(LINUX) //////////////////////////////////////////////////////////////////////////
        // Temporarily disable testing on the Linux build until the file name casing discrepancy
        // is sorted out through the SC build and testing pipeline.
    public:
        inline void MarkLinuxDependencyTestBypass()
        {
            m_graphIsCompiled = true;
            m_graphIsActivated = true;
            m_graphIsDeactivated = true;
            m_graphIsComplete = true;
            m_isGraphLoaded = true;
            m_isParseAttemptMade = true;
            m_isReportFinished = true;
        }
#endif ///////////////////////////////////////////////////////////////////////////////////////

    }; // class Reporter

    using Reporters = AZStd::vector<Reporter>;
}

#include "ScriptCanvasReporter.inl"
