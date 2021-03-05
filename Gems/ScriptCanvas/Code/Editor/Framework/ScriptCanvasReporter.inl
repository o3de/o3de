/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

namespace ScriptCanvasEditor
{

    using namespace ScriptCanvas;

    inline Reporter::Reporter()
    {
    }
        
    inline Reporter::Reporter(const ScriptCanvasId& scriptCanvasId, const AZ::EntityId& entityID) : Reporter()
    {
        SetGraph(scriptCanvasId, entityID);
    }

    inline Reporter::~Reporter()
    {
        Reset();
    }

    inline void Reporter::Checkpoint(const Report& report)
    {
        if (m_isReportFinished)
        {
            return;
        }

        m_checkpoints.push_back(report);
    }

    inline const AZStd::vector<Report>& Reporter::GetCheckpoints() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_checkpoints;
    }

    inline const AZStd::vector<Report>& Reporter::GetFailure() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_failures;
    }

    inline const ScriptCanvasId& Reporter::GetScriptCanvasId() const
    {
        return m_scriptCanvasId;
    }

    inline const AZStd::vector<Report>& Reporter::GetSuccess() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_successes;
    }

    inline bool Reporter::IsActivated() const
    {
        return m_graphIsActivated;
    }

    inline bool Reporter::IsComplete() const
    {
        return m_graphIsComplete;
    }

    inline bool Reporter::IsDeactivated() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_graphIsDeactivated;
    }

    inline bool Reporter::IsErrorFree() const
    {
        if (!m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report must be finished before evaluation");
        }

        return m_graphIsErrorFree;
    }

    inline bool Reporter::IsReportFinished() const
    {
        return m_isReportFinished;
    }
        
    inline void Reporter::FinishReport()
    {
        if (m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report is already finished");
        }

        m_isReportFinished = true;
    }

    inline void Reporter::FinishReport(const bool inErrorState)
    {
        if (m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report is already finished");
        }
        else
        {
            Bus::Handler::BusDisconnect(m_scriptCanvasId);
            AZ::EntityBus::Handler::BusDisconnect(m_entityId);
            m_graphIsErrorFree = !inErrorState;
            m_isReportFinished = true;
        }
    }

    inline bool Reporter::operator==(const Reporter& other) const
    {
        if (m_isReportFinished)
        {
            AZ_Error("ScriptCanvas", false, "The report is already finished");
        }

        return m_graphIsActivated == other.m_graphIsActivated
            && m_graphIsDeactivated == other.m_graphIsDeactivated
            && m_graphIsComplete == other.m_graphIsComplete
            && m_graphIsErrorFree == other.m_graphIsErrorFree
            && m_isReportFinished == other.m_isReportFinished
            && m_failures == other.m_failures
            && m_successes == other.m_successes;
    }

    inline void Reporter::OnEntityActivated(const AZ::EntityId& entity)
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
        
    inline void Reporter::OnEntityDeactivated(const AZ::EntityId& entity)
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
        
    inline void Reporter::Reset()
    {
        Bus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
            
        m_graphIsActivated = false;
        m_graphIsComplete = false;
        m_graphIsErrorFree = false;
        m_isReportFinished = false;
        m_scriptCanvasId = ScriptCanvasId{};
        m_entityId = AZ::EntityId{};
        m_failures.clear();
    }

    inline void Reporter::SetGraph(const ScriptCanvasId& scriptCanvasId, const AZ::EntityId& entityID)
    {
        Reset();
        m_scriptCanvasId = scriptCanvasId;
        m_entityId = entityID;
        Bus::Handler::BusConnect(m_scriptCanvasId);
        AZ::EntityBus::Handler::BusConnect(m_entityId);
    }
       
    // Handler
    inline void Reporter::MarkComplete(const Report& report)
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

    inline void Reporter::AddFailure(const Report& report)
    {
        if (m_isReportFinished)
        {
            return;
        }

        m_failures.push_back(report);
        Checkpoint(AZStd::string::format("AddFailure: %s", report.data()));
    }

    inline void Reporter::AddSuccess(const Report& report)
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

    inline void Reporter::ExpectFalse(const bool value, const Report& report)
    {
        if (value)
        {
            AddFailure(AZStd::string::format("Error | Expected false.\n%s", report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectFalse: %s", report.data()));
        }
    }

    inline void Reporter::ExpectTrue(const bool value, const Report& report)
    {
        if (!value)
        {
            AddFailure(AZStd::string::format("Error | Expected true.\n%s", report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectTrue: %s", report.data()));
        }
    }
    
    inline void Reporter::ExpectEqualNumber(const Data::NumberType lhs, const Data::NumberType rhs, const Report& report)
    {
        if (!AZ::IsClose(lhs, rhs, 0.001))
        {
            AddFailure(AZStd::string::format("Error | Expected near value.\n%s", report.data()));
        }

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectEqualNumber: %s", report.data()));
        }
    }
    
    inline void Reporter::ExpectNotEqualNumber(const Data::NumberType lhs, const Data::NumberType rhs, const Report& report)
    {
        SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_NE(lhs, rhs)

        if (!report.empty())
        {
            Checkpoint(AZStd::string::format("ExpectNotEqualNumber: %s", report.data()));
        }
    }

    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_EQ)
    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectNotEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_NE)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GT)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GE)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LT)
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LE)

} // namespace ScriptCanvasEditor
