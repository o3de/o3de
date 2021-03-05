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
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSessionRequest.h>
#include <GridMate/Replica/ReplicaFunctions.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <aws/core/Aws.h>

///////////////////////////////////////////////////////////////////////////////
struct TestSettings
{
    const char* m_accessKey = "";
    const char* m_secretKey = "";
    const char* m_region = "";
    const char* m_fleetId = "";
    const char* m_endpoint = "";
};

// Test constants
static const unsigned int s_maxTicks = 1000;
static const unsigned int s_tickDelayMs = 30;

// Global GameLift settings
static TestSettings s_gameLiftSettings;

// AWS Allocator
class AwsAllocator : public Aws::Utils::Memory::MemorySystemInterface
{
public:
    void Begin() override {};
    void End() override {};

    void* AllocateMemory(std::size_t blockSize, std::size_t alignment, const char *allocationTag = nullptr) override
    {
        return AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Get().Allocate(blockSize, alignment, 0, allocationTag);
    }

    void FreeMemory(void* memoryPtr) override
    {
        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Get().DeAllocate(memoryPtr);
    }
};

static AwsAllocator s_awsAllocator;
///////////////////////////////////////////////////////////////////////////////


class GameLiftTest 
    : public GridMate::SessionEventBus::Handler
    , public GridMate::GameLiftClientServiceEventsBus::Handler
{
public:
    GameLiftTest()
        : m_gridMate(nullptr)
        , m_service(nullptr)
    {
    }

    ~GameLiftTest()
    {
        AZ_TEST_ASSERT(!m_service);
        AZ_TEST_ASSERT(!m_gridMate);
    }

    void Init(const TestSettings& settings)
    {
        // Initializing system allocators
        AZ::AllocatorInstance<AZ::OSAllocator>::Create();
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        // Creating Gridmate instance
        GridMate::GridMateDesc gridmateDesc;
        m_gridMate = GridMate::GridMateCreate(gridmateDesc);

        // Initializing GridMate multiplayer service allocator
        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

        // Subscribing for session events
        GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);
        GridMate::GameLiftClientServiceEventsBus::Handler::BusConnect(m_gridMate);

        // Starting GameLift multiplayer service
        GridMate::GameLiftClientServiceDesc serviceDesc;
        serviceDesc.m_accessKey = settings.m_accessKey;
        serviceDesc.m_secretKey = settings.m_secretKey;
        serviceDesc.m_fleetId = settings.m_fleetId;
        serviceDesc.m_endpoint = settings.m_endpoint;
        serviceDesc.m_region = settings.m_region;

        Aws::SDKOptions awsSDKOptions;
        awsSDKOptions.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
        awsSDKOptions.loggingOptions.defaultLogPrefix = "AWS";
        awsSDKOptions.memoryManagementOptions.memoryManager = &s_awsAllocator;
        Aws::InitAPI(awsSDKOptions);

        m_service = GridMate::StartGridMateService<GridMate::GameLiftClientService>(m_gridMate, serviceDesc);
    }

    void Shutdown()
    {
        if (m_gridMate)
        {
            GridMate::GameLiftClientServiceEventsBus::Handler::BusDisconnect();
            GridMate::SessionEventBus::Handler::BusDisconnect();
            GridMateDestroy(m_gridMate);

            Aws::SDKOptions awsSDKOptions;
            awsSDKOptions.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
            awsSDKOptions.loggingOptions.defaultLogPrefix = "AWS";
            awsSDKOptions.memoryManagementOptions.memoryManager = &s_awsAllocator;
            Aws::ShutdownAPI(awsSDKOptions);

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();

            m_service = nullptr;
            m_gridMate = nullptr;
        }
    }

protected:
    GridMate::IGridMate* m_gridMate;
    GridMate::GameLiftClientService* m_service;
};

class GameLiftSessionTest
    : public GameLiftTest
{
public:
    void run()
    {
        Init(s_gameLiftSettings);

        // waiting till service is ready
        int waitReady = 1000;
        while (!m_isReady && waitReady--)
        {
            m_gridMate->Update();
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(s_tickDelayMs));
        }

        AZ_TEST_ASSERT(m_isReady);

        GridMate::GameLiftSessionRequestParams reqParams;
        reqParams.m_instanceName = "TestSession";
        reqParams.m_numPublicSlots = 16;
        reqParams.m_params[0].m_id = "param1";
        reqParams.m_params[0].m_value = "value12";
        reqParams.m_numParams = 1;
        m_sessionRequest = m_service->RequestSession(reqParams);

        while (m_numTicks < s_maxTicks)
        {
            if (m_numTicks == 500) // starting search
            {
                EBUS_EVENT_ID_RESULT(m_search, m_gridMate, GridMate::GameLiftClientServiceBus, StartSearch, GridMate::GameLiftSearchParams());
            }

            ++m_numTicks;
            m_gridMate->Update();
            if (m_session)
            {
                m_session->GetReplicaMgr()->Unmarshal();
                m_session->GetReplicaMgr()->UpdateFromReplicas();
                m_session->GetReplicaMgr()->UpdateReplicas();
                m_session->GetReplicaMgr()->Marshal();
            }
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(s_tickDelayMs));
        }

        AZ_TEST_ASSERT(m_numSessionServiceReady == 1);
        AZ_TEST_ASSERT(m_numSessionCreated == 1);
        AZ_TEST_ASSERT(m_numSessionDeleted == 0);
        AZ_TEST_ASSERT(m_numSessionErrors == 0);
        AZ_TEST_ASSERT(m_numSearchCompleted == 2); // one for session request and one for the search
        AZ_TEST_ASSERT(m_numSearchResults > 0); // at least one search result should've been returned (the session that client requested at startup)


        AZ_TEST_ASSERT(m_session->GetNumParams() == 1);
        AZ_TEST_ASSERT(strcmp(m_session->GetParam(0).m_id.c_str(), "param1") == 0);
        AZ_TEST_ASSERT(strcmp(m_session->GetParam(0).m_value.c_str(), "value12") == 0);

        Shutdown();
    }

private:
    void OnGameLiftSessionServiceReady(GridMate::GameLiftClientService*) override
    {
        AZ_Printf("GameLift", "Service is ready\n");
        ++m_numSessionServiceReady;

        m_isReady = true;
    }

    void OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService*) override
    {
        AZ_Printf("GameLift", "Service failed to initialize\n");
        AZ_TEST_ASSERT(false);
    }

    void OnSessionCreated(GridMate::GridSession* session) override
    {
        (void)session;
        AZ_Printf("GameLift", "Session created: %s\n", session->GetId().c_str());
        ++m_numSessionCreated;
    }

    void OnSessionDelete(GridMate::GridSession* session) override
    {
        (void)session;
        AZ_Printf("GameLift", "Session deleted: %s\n", session->GetId().c_str());
        ++m_numSessionDeleted;
    }

    void OnSessionError(GridMate::GridSession* session, const GridMate::string& errorMsg) override
    {
        (void)session;
        AZ_Printf("GameLift", "Session error: %s\n", errorMsg.c_str());
        ++m_numSessionErrors;
    }

    void OnGridSearchComplete(GridMate::GridSearch* gridSearch)
    {
        ++m_numSearchCompleted;

        if (gridSearch == m_sessionRequest) // got request result, can now join the session
        {
            GridMate::CarrierDesc carrierDesc;
            carrierDesc.m_port = 33435;
            carrierDesc.m_enableDisconnectDetection = true;
            carrierDesc.m_connectionTimeoutMS = 10000;
            carrierDesc.m_threadUpdateTimeMS = 30;

            AZ_Printf("GridMateClient", "Request results returned: %u.\n", gridSearch->GetNumResults());
            AZ_TEST_ASSERT(gridSearch->GetNumResults() == 1); // should get exactly one result from session request, just the session we requested
            AZ_TEST_ASSERT(gridSearch->GetResult(0)->m_numParams == 1); // checking for the single parameter
            AZ_TEST_ASSERT(strcmp(gridSearch->GetResult(0)->m_params[0].m_id.c_str(), "param1") == 0);
            AZ_TEST_ASSERT(strcmp(gridSearch->GetResult(0)->m_params[0].m_value.c_str(), "value12") == 0);

            const GridMate::GameLiftSearchInfo& gameliftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo&>(*gridSearch->GetResult(0));
            EBUS_EVENT_ID_RESULT(m_session,m_gridMate,GridMate::GameLiftClientServiceBus,JoinSessionBySearchInfo,gameliftSearchInfo,carrierDesc);
        }
        else if (gridSearch == m_search) // got search results
        {
            m_numSearchResults = gridSearch->GetNumResults();
            AZ_Printf("GameLift", "Found %u game sessions.\n", m_numSearchResults);
        }
    }

    GridMate::GridSession* m_session = nullptr;
    unsigned int m_numTicks = 0;
    bool m_isReady = false;

    GridMate::GridSearch* m_sessionRequest = nullptr;
    GridMate::GridSearch* m_search = nullptr;

    // test values
    unsigned int m_numSessionServiceReady = 0;
    unsigned int m_numSessionCreated = 0;
    unsigned int m_numSessionDeleted = 0;
    unsigned int m_numSessionErrors = 0;
    unsigned int m_numSearchResults = 0;
    unsigned int m_numSearchCompleted = 0;
};

AZ_TEST_SUITE(GameLiftSession)
AZ_TEST(GameLiftSessionTest)
AZ_TEST_SUITE_END

void main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        AZ_Printf("GameLift", "Supported settings:\n"
            "  accessKey:<string>     - AWS Access Key\n"
            "  secretKey:<string>     - AWS Secret Key\n"
            "  region:<string>        - AWS region\n"
            "  fleetId:<string>       - GameLift fleet id\n"
            "  endpoint:<string>      - GameLift endpoint\n"
            );

        ::exit(-1);
    }

    for (int i = 1; i < argc; ++i)
    {
        char* pos = strchr(argv[i], ':');
        if (!pos)
        {
            AZ_Printf("GameLift", "Invalid argument format: %s. Expected 'key:value'.", argv[i]);
            continue;
        }

        *pos = '\0';
        const char* paramName = argv[i];
        const char* paramVal = pos + 1;

        if (!azstricmp(paramName, "accessKey"))
        {
            s_gameLiftSettings.m_accessKey = paramVal;
        }
        else if (!azstricmp(paramName, "secretKey"))
        {
            s_gameLiftSettings.m_secretKey = paramVal;
        }
        else if (!azstricmp(paramName, "fleetId"))
        {
            s_gameLiftSettings.m_fleetId = paramVal;
        }
        else if (!azstricmp(paramName, "endpoint"))
        {
            s_gameLiftSettings.m_endpoint = paramVal;
        }
        else if (!azstricmp(paramName, "region"))
        {
            s_gameLiftSettings.m_region = paramVal;
        }
        else
        {
            AZ_Printf("GameLift", "Unsupported parameter: %s", argv[i]);
        }
    }

    AZ_TEST_RUN_SUITE(GameLiftSession);
    AZ_TEST_OUTPUT_RESULTS("GameLiftTest", "GameLiftTest.xml");
    ::exit(AZ_TEST_GET_STATUS ? 0 : 1);
}
