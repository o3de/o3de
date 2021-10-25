/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>

#include <AWSGameLiftClientFixture.h>
#include <AWSGameLiftClientLocalTicketTracker.h>
#include <AWSGameLiftClientMocks.h>

#include <Request/IAWSGameLiftInternalRequests.h>

using namespace AWSGameLift;

static constexpr const uint64_t TEST_RACKER_POLLING_PERIOD_MS = 1000;
static constexpr const uint64_t TEST_WAIT_BUFFER_TIME_MS = 10;
static constexpr const uint64_t TEST_WAIT_MAXIMUM_TIME_MS = 10000;

class TestAWSGameLiftClientLocalTicketTracker
    : public AWSGameLiftClientLocalTicketTracker
{
public:
    TestAWSGameLiftClientLocalTicketTracker() = default;
    virtual ~TestAWSGameLiftClientLocalTicketTracker() = default;

    void SetUp()
    {
        ActivateTracker();
        m_pollingPeriodInMS = TEST_RACKER_POLLING_PERIOD_MS;
    }

    void TearDown()
    {
        DeactivateTracker();
    }

    bool IsTrackerIdle()
    {
        return m_status == TicketTrackerStatus::Idle;
    }
};

class AWSGameLiftClientLocalTicketTrackerTest
    : public AWSGameLiftClientFixture
    , public IAWSGameLiftInternalRequests
{
protected:
    void SetUp() override
    {
        AWSGameLiftClientFixture::SetUp();

        AZ::Interface<IAWSGameLiftInternalRequests>::Register(this);

        m_gameliftClientMockPtr = AZStd::make_shared<GameLiftClientMock>();
        m_gameliftClientTicketTracker = AZStd::make_unique<TestAWSGameLiftClientLocalTicketTracker>();
        m_gameliftClientTicketTracker->SetUp();
    }

    void TearDown() override
    {
        m_gameliftClientTicketTracker->TearDown();
        m_gameliftClientTicketTracker.reset();
        m_gameliftClientMockPtr.reset();

        AZ::Interface<IAWSGameLiftInternalRequests>::Unregister(this);

        AWSGameLiftClientFixture::TearDown();
    }

    AZStd::shared_ptr<Aws::GameLift::GameLiftClient> GetGameLiftClient() const
    {
        return m_gameliftClientMockPtr;
    }

    void SetGameLiftClient(AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient)
    {
        AZ_UNUSED(gameliftClient);
        m_gameliftClientMockPtr.reset();
    }

    void WaitForProcessFinish(AZStd::function<bool()> processFinishCondition)
    {
        int processingTime = 0;
        while (processingTime < TEST_WAIT_MAXIMUM_TIME_MS)
        {
            if (processFinishCondition())
            {
                return;
            }
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(TEST_WAIT_BUFFER_TIME_MS));
            processingTime += TEST_WAIT_BUFFER_TIME_MS;
        }
    }

public:
    AZStd::unique_ptr<TestAWSGameLiftClientLocalTicketTracker> m_gameliftClientTicketTracker;
    AZStd::shared_ptr<GameLiftClientMock> m_gameliftClientMockPtr;
};

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallWithoutClientSetup_GetExpectedErrors)
{
    AZ::Interface<IAWSGameLiftInternalRequests>::Get()->SetGameLiftClient(nullptr);

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([](){ return ::UnitTest::TestRunner::Instance().m_numAssertsFailed == 1; });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchError == 1);
    ASSERT_FALSE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_MultipleCallsWithoutClientSetup_GetExpectedErrors)
{
    AZ::Interface<IAWSGameLiftInternalRequests>::Get()->SetGameLiftClient(nullptr);

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([](){ return ::UnitTest::TestRunner::Instance().m_numAssertsFailed == 1; });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchError == 1);
    ASSERT_FALSE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallButWithFailedOutcome_GetExpectedErrors)
{
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(error);

    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([](){ return ::UnitTest::TestRunner::Instance().m_numAssertsFailed == 1; });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchError == 1);
    ASSERT_FALSE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallWithMoreThanOneTicket_GetExpectedErrors)
{
    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(Aws::GameLift::Model::MatchmakingTicket());
    result.AddTicketList(Aws::GameLift::Model::MatchmakingTicket());
    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);

    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([](){ return ::UnitTest::TestRunner::Instance().m_numAssertsFailed == 1; });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchError == 1);
    ASSERT_FALSE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallWithCompleteStatus_ProcessStopsAndGetExpectedResult)
{
    Aws::GameLift::Model::GameSessionConnectionInfo connectionInfo;
    connectionInfo.SetIpAddress("DummyIpAddress");
    connectionInfo.SetPort(123);
    connectionInfo.AddMatchedPlayerSessions(
        Aws::GameLift::Model::MatchedPlayerSession()
        .WithPlayerId("player1")
        .WithPlayerSessionId("playersession1"));

    Aws::GameLift::Model::MatchmakingTicket ticket;
    ticket.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED);
    ticket.SetGameSessionConnectionInfo(connectionInfo);

    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(ticket);

    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);
    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    SessionHandlingClientRequestsMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, RequestPlayerJoinSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(true));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchComplete == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallButNoPlayerSession_ProcessStopsAndGetExpectedError)
{
    Aws::GameLift::Model::GameSessionConnectionInfo connectionInfo;
    connectionInfo.SetIpAddress("DummyIpAddress");
    connectionInfo.SetPort(123);

    Aws::GameLift::Model::MatchmakingTicket ticket;
    ticket.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED);
    ticket.SetGameSessionConnectionInfo(connectionInfo);

    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(ticket);

    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);
    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchComplete == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallButFailedToJoinMatch_ProcessStopsAndGetExpectedError)
{
    Aws::GameLift::Model::GameSessionConnectionInfo connectionInfo;
    connectionInfo.SetIpAddress("DummyIpAddress");
    connectionInfo.SetPort(123);
    connectionInfo.AddMatchedPlayerSessions(
        Aws::GameLift::Model::MatchedPlayerSession()
        .WithPlayerId("player1")
        .WithPlayerSessionId("playersession1"));

    Aws::GameLift::Model::MatchmakingTicket ticket;
    ticket.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED);
    ticket.SetGameSessionConnectionInfo(connectionInfo);

    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(ticket);

    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);
    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    SessionHandlingClientRequestsMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, RequestPlayerJoinSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(false));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchComplete == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallButTicketTimeOut_ProcessStopsAndGetExpectedError)
{
    Aws::GameLift::Model::MatchmakingTicket ticket;
    ticket.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::TIMED_OUT);

    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(ticket);

    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);
    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchFailure == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallButTicketFailed_ProcessStopsAndGetExpectedError)
{
    Aws::GameLift::Model::MatchmakingTicket ticket;
    ticket.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED);

    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(ticket);

    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);
    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchFailure == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallButTicketCancelled_ProcessStopsAndGetExpectedError)
{
    Aws::GameLift::Model::MatchmakingTicket ticket;
    ticket.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::CANCELLED);

    Aws::GameLift::Model::DescribeMatchmakingResult result;
    result.AddTicketList(ticket);

    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome(result);
    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchFailure == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_CallAndTicketCompleteAtLast_ProcessContinuesAndStop)
{
    Aws::GameLift::Model::MatchmakingTicket ticket1;
    ticket1.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::QUEUED);

    Aws::GameLift::Model::DescribeMatchmakingResult result1;
    result1.AddTicketList(ticket1);
    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome1(result1);

    Aws::GameLift::Model::GameSessionConnectionInfo connectionInfo;
    connectionInfo.SetIpAddress("DummyIpAddress");
    connectionInfo.SetPort(123);
    connectionInfo.AddMatchedPlayerSessions(
        Aws::GameLift::Model::MatchedPlayerSession()
        .WithPlayerId("player1")
        .WithPlayerSessionId("playersession1"));

    Aws::GameLift::Model::MatchmakingTicket ticket2;
    ticket2.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED);
    ticket2.SetGameSessionConnectionInfo(connectionInfo);

    Aws::GameLift::Model::DescribeMatchmakingResult result2;
    result2.AddTicketList(ticket2);
    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome2(result2);

    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .WillOnce(::testing::Return(outcome1))
        .WillOnce(::testing::Return(outcome2));

    SessionHandlingClientRequestsMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, RequestPlayerJoinSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(true));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchComplete == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}

TEST_F(AWSGameLiftClientLocalTicketTrackerTest, StartPolling_RequiresAcceptanceAndTicketCompleteAtLast_ProcessContinuesAndStop)
{
    Aws::GameLift::Model::MatchmakingTicket ticket1;
    ticket1.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::REQUIRES_ACCEPTANCE);

    Aws::GameLift::Model::DescribeMatchmakingResult result1;
    result1.AddTicketList(ticket1);
    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome1(result1);

    Aws::GameLift::Model::GameSessionConnectionInfo connectionInfo;
    connectionInfo.SetIpAddress("DummyIpAddress");
    connectionInfo.SetPort(123);
    connectionInfo.AddMatchedPlayerSessions(
        Aws::GameLift::Model::MatchedPlayerSession()
        .WithPlayerId("player1")
        .WithPlayerSessionId("playersession1"));

    Aws::GameLift::Model::MatchmakingTicket ticket2;
    ticket2.SetStatus(Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED);
    ticket2.SetGameSessionConnectionInfo(connectionInfo);

    Aws::GameLift::Model::DescribeMatchmakingResult result2;
    result2.AddTicketList(ticket2);
    Aws::GameLift::Model::DescribeMatchmakingOutcome outcome2(result2);

    EXPECT_CALL(*m_gameliftClientMockPtr, DescribeMatchmaking(::testing::_))
        .WillOnce(::testing::Return(outcome1))
        .WillOnce(::testing::Return(outcome2));

    SessionHandlingClientRequestsMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, RequestPlayerJoinSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(true));

    MatchmakingNotificationsHandlerMock matchmakingHandlerMock;
    m_gameliftClientTicketTracker->StartPolling("ticket1", "player1");
    WaitForProcessFinish([this](){ return m_gameliftClientTicketTracker->IsTrackerIdle(); });
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchAcceptance == 1);
    ASSERT_TRUE(matchmakingHandlerMock.m_numMatchComplete == 1);
    ASSERT_TRUE(m_gameliftClientTicketTracker->IsTrackerIdle());
}
