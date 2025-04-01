/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/SystemComponent.h>
#include <ScriptCanvas/Execution/ExecutionState.h>

namespace ScriptCanvasEditor
{

    using namespace ScriptCanvas;

    AZ_INLINE Reporter::Reporter()
    {
        m_parseDuration = m_translationDuration = 0;
        ScriptCanvas::ExecutionNotificationsBus::Handler::BusConnect();
    }
        
    AZ_INLINE Reporter::Reporter(const AZ::EntityId& entityID) : Reporter()
    {
        m_parseDuration = m_translationDuration = 0;
        SetEntity(entityID);
        ScriptCanvas::ExecutionNotificationsBus::Handler::BusConnect();
    }

    AZ_INLINE Reporter::~Reporter() {}

    AZ_INLINE void Reporter::Checkpoint(const Report& report)
    {
        if (m_isReportFinished)
        {
            return;
        }

        m_checkpoints.push_back(report);
    }

    AZ_INLINE void Reporter::CollectPerformanceTiming()
    {
        ScriptCanvas::SystemComponent::ModPerformanceTracker()->CalculateReports();
        m_performanceReport = ScriptCanvas::SystemComponent::ModPerformanceTracker()->GetSnapshotReport();
    }

    AZ_INLINE bool Reporter::ExpectsParseError() const
    {
        return m_expectParseError;
    }

    AZ_INLINE bool Reporter::ExpectsRuntimeFailure() const
    {
        return m_expectsRuntimeError;
    }

    AZ_INLINE void Reporter::FinishReport()
    {
        if (m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report is already finished");
        }

        m_isReportFinished = true;
        Bus::Handler::BusDisconnect(m_graph);
        AZ::EntityBus::Handler::BusDisconnect(m_entityId);
    }

    AZ_INLINE const AZStd::vector<Report>& Reporter::GetCheckpoints() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_checkpoints;
    }

    AZ_INLINE ExecutionConfiguration Reporter::GetExecutionConfiguration() const
    {
        return m_configuration;
    }

    AZ_INLINE ExecutionMode Reporter::GetExecutionMode() const
    {
        return m_mode;
    }

    AZ_INLINE const AZStd::vector<Report>& Reporter::GetFailure() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_failures;
    }

    AZ_INLINE const AZ::Data::AssetId& Reporter::GetGraph() const
    {
        return m_graph;
    }

    AZ_INLINE AZStd::sys_time_t Reporter::GetParseDuration() const
    {
        return m_parseDuration;
    }

    AZ_INLINE const Execution::PerformanceTrackingReport& Reporter::GetPerformanceReport() const
    {
        return m_performanceReport;
    }

    AZ_INLINE const AZStd::vector<Report>& Reporter::GetSuccess() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_successes;
    }

    AZ_INLINE AZStd::sys_time_t Reporter::GetTranslateDuration() const
    {
        return m_translationDuration;
    }

    AZ_INLINE const AZ::IO::Path& Reporter::GetFilePath() const
    {
        return m_filePath;
    }

    AZ_INLINE bool Reporter::IsActivated() const
    {
        return m_graphIsActivated;
    }

    AZ_INLINE bool Reporter::IsCompiled() const
    {
        return m_graphIsCompiled;
    }

    AZ_INLINE bool Reporter::IsComplete() const
    {
        return m_graphIsComplete;
    }

    AZ_INLINE bool Reporter::IsDeactivated() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_graphIsDeactivated;
    }

    AZ_INLINE bool Reporter::IsErrorFree() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_failures.empty();
    }

    AZ_INLINE bool Reporter::IsGraphLoaded() const
    {
        return m_isGraphLoaded;
    }

    AZ_INLINE bool Reporter::IsGraphObserved([[maybe_unused]] const AZ::EntityId& entityId, const GraphIdentifier& identifier)
    {
        return m_configuration == ExecutionConfiguration::Traced && identifier.m_assetId == m_graph;
    }

    AZ_INLINE void Reporter::RuntimeError([[maybe_unused]] const ExecutionState& executionState, const AZStd::string_view& description)
    {
        m_failures.push_back(AZStd::string::format("ScriptCanvas runtime error: %s", description.data()));
    }

    AZ_INLINE bool Reporter::IsParseAttemptMade() const
    {
        return m_isParseAttemptMade;
    }

    AZ_INLINE bool Reporter::IsProcessOnly() const
    {
        return m_processOnly;
    }

    AZ_INLINE bool Reporter::IsReportFinished() const
    {
        return m_isReportFinished;
    }
        
    AZ_INLINE void Reporter::MarkCompiled()
    {
        m_graphIsCompiled = true;
    }

    AZ_INLINE void Reporter::MarkExpectParseError()
    {
        m_expectParseError = true;
    }

    AZ_INLINE void Reporter::MarkExpectRuntimeFailure()
    {
        m_expectsRuntimeError = true;
    }

    AZ_INLINE void Reporter::MarkGraphLoaded()
    {
        m_isGraphLoaded = true;
    }

    AZ_INLINE void Reporter::MarkParseAttemptMade()
    {
        m_isParseAttemptMade = true;
    }

    AZ_INLINE bool Reporter::operator==(const Reporter& other) const
    {
        return m_graphIsActivated == other.m_graphIsActivated
            && m_graphIsDeactivated == other.m_graphIsDeactivated
            && m_graphIsComplete == other.m_graphIsComplete
            && m_isReportFinished == other.m_isReportFinished
            && m_failures == other.m_failures
            && m_successes == other.m_successes;
    }

    AZ_INLINE void Reporter::OnEntityActivated(const AZ::EntityId& entity)
    {
        if (m_entityId != entity)
        {
            AZ_Error("ScriptCanvas", false, "This reporter is listening to the wrong entity");
        }

        if (m_isReportFinished)
        {
            return;
        }

        m_graphIsActivated = true;
    }
        
    AZ_INLINE void Reporter::OnEntityDeactivated(const AZ::EntityId& entity)
    {
        if (m_entityId != entity)
        {
            AZ_Error("ScriptCanvas", false, "This reporter is listening to the wrong entity");
        }

        if (m_isReportFinished)
        {
            return;
        }

        m_graphIsDeactivated = true;
    }
      
    AZ_INLINE void Reporter::SetDurations(AZStd::sys_time_t parse, AZStd::sys_time_t translate)
    {
        m_parseDuration = parse;
        m_translationDuration = translate;
    }

    AZ_INLINE void Reporter::SetExecutionConfiguration(ExecutionConfiguration configuration)
    {
        m_configuration = configuration;
    }

    AZ_INLINE void Reporter::SetExecutionMode(ExecutionMode mode)
    {
        m_mode = mode;
    }

    AZ_INLINE void Reporter::SetEntity(const AZ::EntityId& entityID)
    {
        m_entityId = entityID;
        AZ::EntityBus::Handler::BusConnect(m_entityId);
    }

    AZ_INLINE void Reporter::SetGraph(const AZ::Data::AssetId& graph)
    {
        m_graph = graph;
        Bus::Handler::BusConnect(graph);
    }

    AZ_INLINE void Reporter::SetProcessOnly(bool processOnly)
    {
        m_processOnly = processOnly;
    }

    AZ_INLINE void Reporter::SetFilePath(const AZ::IO::PathView& filePath)
    {
        m_filePath = filePath;
    }

    // Handler
    AZ_INLINE void Reporter::MarkComplete(const Report& report)
    {
        if (m_isReportFinished)
        {
            return;
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("MarkComplete: %s", report.data()));
        }

        if (m_graphIsComplete)
        {
            AddFailure("MarkComplete was called twice!");
        }
        else
        {
            m_graphIsComplete = true;
        }
    }

    AZ_INLINE void Reporter::AddFailure(const Report& report)
    {
        if (m_isReportFinished)
        {
            return;
        }

        m_failures.push_back(report);
        
        Checkpoint(AZStd::string::format("AddFailure: %s", report.data()));
    }

    AZ_INLINE void Reporter::AddSuccess(const Report& report)
    {
        if (m_isReportFinished)
        {
            return;
        }

        m_successes.push_back(report);

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("AddSuccess: %s", report.data()));
        }
    }

    AZ_INLINE void Reporter::ExpectFalse(const bool value, const Report& report)
    {
        if (value)
        {
            AddFailure(AZStd::string::format("Error | Expected false.: %s", report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectFalse: %s", report.data()));
        }
    }

    AZ_INLINE void Reporter::ExpectTrue(const bool value, const Report& report)
    {
        if (!value)
        {
            AddFailure(AZStd::string::format("Error | Expected true.: %s", report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectTrue: %s", report.data()));
        }
    }
    
    AZ_INLINE void Reporter::ExpectEqual(const Data::NumberType lhs, const Data::NumberType rhs, const Report& report)
    {
        if (!AZ::IsClose(lhs, rhs, 0.001))
        {
            AddFailure(AZStd::string::format("Error | Expected(candidate: %s) == (reference: %s): %s", Datum(lhs).ToString().data(), Datum(rhs).ToString().data(), report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectEqual: %s", report.data()));
        }
    }
    
    AZ_INLINE void Reporter::ExpectNotEqual(const Data::NumberType lhs, const Data::NumberType rhs, const Report& report)
    {
        if (AZ::IsClose(lhs, rhs, 0.001))
        {
            AddFailure(AZStd::string::format("Error | Expected(candidate: %s) != (reference: %s): %s", Datum(lhs).ToString().data(), Datum(rhs).ToString().data(), report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectNotEqual: %s", report.data()));
        }
    }

    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_EQ)
    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectNotEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_NE)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GT)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GE)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LT)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LE)

    // Vector Implementations
    SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_GT)
    SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_GE)
    SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_LT)
    SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_VECTOR_EXPECT_LE)


}
