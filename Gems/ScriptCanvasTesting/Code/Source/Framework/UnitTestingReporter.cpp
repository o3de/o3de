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


#include "UnitTestingReporter.h"

#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <AzTest/AzTest.h>

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_EQ(LHS, RHS)\
    EXPECT_EQ(LHS, RHS) << report.data();

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_NE(LHS, RHS)\
    EXPECT_NE(LHS, RHS) << report.data();

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GT(LHS, RHS)\
    EXPECT_GT(LHS, RHS) << report.data();

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GE(LHS, RHS)\
    EXPECT_GE(LHS, RHS) << report.data();

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LT(LHS, RHS)\
    EXPECT_LT(LHS, RHS) << report.data();

#define SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LE(LHS, RHS)\
    EXPECT_LE(LHS, RHS) << report.data();

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        Reporter::Reporter()
            : m_graphId{}
            , m_entityId{}
        {}
        
        Reporter::Reporter(const RuntimeComponent& graph)
        {
            SetGraph(graph);
        }

        Reporter::~Reporter()
        {
            Reset();
        }

        void Reporter::Checkpoint(const Report& report)
        {
            if (m_isReportFinished)
                return;

            m_checkpoints.push_back(report);
        }

        const AZStd::vector<Report>& Reporter::GetCheckpoints() const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_checkpoints;
        }

        const AZStd::vector<Report>& Reporter::GetFailure() const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_failures;
        }

        const AZ::EntityId& Reporter::GetGraphId() const
        {
            return m_graphId;
        }

        const AZStd::vector<Report>& Reporter::GetSuccess() const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_successes;
        }

        bool Reporter::IsActivated() const
        {
            return m_graphIsActivated;
        }

        bool Reporter::IsComplete() const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_graphIsComplete;
        }

        bool Reporter::IsDeactivated() const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_graphIsDeactivated;
        }

        bool Reporter::IsErrorFree() const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_graphIsErrorFree;
        }

        bool Reporter::IsReportFinished() const
        {
            return m_isReportFinished;
        }
        
        void Reporter::FinishReport()
        {
            AZ_Assert(!m_isReportFinished, "the report is already finished");
            m_isReportFinished = true;
        }

        void Reporter::FinishReport(const RuntimeComponent& graph)
        {
            AZ_Assert(!m_isReportFinished, "the report is already finished");
            Bus::Handler::BusDisconnect(m_graphId);
            AZ::EntityBus::Handler::BusDisconnect(m_entityId);
            m_graphIsErrorFree = !graph.IsInErrorState();
            m_isReportFinished = true;
        }

        bool Reporter::operator==(const Reporter& other) const
        {
            AZ_Assert(m_isReportFinished, "the report must be finished before evaluation");
            return m_graphIsActivated == other.m_graphIsActivated
                && m_graphIsDeactivated == other.m_graphIsDeactivated
                && m_graphIsComplete == other.m_graphIsComplete
                && m_graphIsErrorFree == other.m_graphIsErrorFree
                && m_isReportFinished == other.m_isReportFinished
                && m_failures == other.m_failures
                && m_successes == other.m_successes;
        }

        void Reporter::OnEntityActivated(const AZ::EntityId& entity)
        {
            AZ_Assert(m_entityId == entity, "this reporter is listening to the wrong entity");
            if (m_isReportFinished)
                return;

            m_graphIsActivated = true;
        }
        
        void Reporter::OnEntityDeactivated(const AZ::EntityId& entity)
        {
            AZ_Assert(m_entityId == entity, "this reporter is listening to the wrong entity");
            if (m_isReportFinished)
                return;

            m_graphIsDeactivated = true;
        }
        
        void Reporter::Reset()
        {
            if (m_graphId.IsValid())
            {
                Bus::Handler::BusDisconnect();
            }

            if (m_entityId.IsValid())
            {
                AZ::EntityBus::Handler::BusDisconnect();
            }
    
            m_graphIsActivated = false;
            m_graphIsComplete = false;
            m_graphIsErrorFree = false;
            m_isReportFinished = false;
            m_graphId = AZ::EntityId{};
            m_failures.clear();
            m_failures.clear();
        }

        void Reporter::SetGraph(const RuntimeComponent& graph)
        {
            Reset();
            m_graphId = graph.GetUniqueId();
            m_entityId = graph.GetEntityId();
            Bus::Handler::BusConnect(m_graphId);
            AZ::EntityBus::Handler::BusConnect(m_entityId);
        }
       
        // Handler
        void Reporter::MarkComplete(const Report& report)
        {
            if (m_isReportFinished) 
                return;
            
            if (m_graphIsComplete)
            {
                AddFailure(AZStd::string::format("MarkComplete was called twice. %s", report.data()));
            }
            else
            {
                m_graphIsComplete = true;
            }
        }

        void Reporter::AddFailure(const Report& report)
        {
            if (m_isReportFinished)
                return;

            m_failures.push_back(report);
            Checkpoint(AZStd::string::format("AddFailure: %s", report.data()));
        }

        void Reporter::AddSuccess(const Report& report)
        {
            if (m_isReportFinished)
                return;

            m_successes.push_back(report);
            Checkpoint(AZStd::string::format("AddSuccess: %s", report.data()));
        }

        void Reporter::ExpectFalse(const bool value, const Report& report)
        {
            EXPECT_FALSE(value) << report.data();
            Checkpoint(AZStd::string::format("ExpectFalse: %s", report.data()));
        }

        void Reporter::ExpectTrue(const bool value, const Report& report)
        {
            EXPECT_TRUE(value) << report.data();
            Checkpoint(AZStd::string::format("ExpectTrue: %s", report.data()));
        }

        void Reporter::ExpectEqualNumber(const Data::NumberType lhs, const Data::NumberType rhs, const Report& report)
        {
            EXPECT_NEAR(lhs, rhs, 0.001) << report.data();
            Checkpoint(AZStd::string::format("ExpectEqualNumber: %s", report.data()));
        }

        void Reporter::ExpectNotEqualNumber(const Data::NumberType lhs, const Data::NumberType rhs, const Report& report)
        {
            EXPECT_NE(lhs,  rhs) << report.data();
            Checkpoint(AZStd::string::format("ExpectNotEqualNumber: %s", report.data()));
        }

        SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_EQ)
        SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectNotEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_NE)
        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GT)
        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectGreaterThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_GE)
        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThan, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LT)
        SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(Reporter, ExpectLessThanEqual, SCRIPT_CANVAS_UNIT_TEST_REPORTER_EXPECT_LE)

    } // namespace UnitTesting

} // namespace ScriptCanvas
