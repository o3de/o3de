/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveDataSystemComponent.h>

#include <AzTest/AzTest.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "SaveDataTest.h"

#if !AZ_TRAIT_DISABLE_ALL_SAVE_DATA_TESTS

void SaveDataTest::SetUp()
{
    LeakDetectionFixture::SetUp();
    m_saveDataSystemComponent = AZStd::make_unique<SaveData::SaveDataSystemComponent>();
    m_saveDataSystemComponent->Activate();

    SetupInternal();
}

void SaveDataTest::TearDown()
{
    TearDownInternal();

    m_saveDataSystemComponent->Deactivate();
    m_saveDataSystemComponent.reset();
    LeakDetectionFixture::TearDown();
}


class OnSavedHandler : public SaveData::SaveDataNotificationBus::Handler
{
public:
    OnSavedHandler()
    {
        BusConnect();
    }

    void OnDataBufferSaved(const DataBufferSavedParams& dataBufferSavedParams) override
    {
        lastSavedParams = dataBufferSavedParams;
        notificationReceived = true;
    }
    void OnDataBufferLoaded([[maybe_unused]] const DataBufferLoadedParams& dataBufferLoadedParams) override {}

    DataBufferSavedParams lastSavedParams;
    bool notificationReceived = false;
};

class OnLoadedHandler : public SaveData::SaveDataNotificationBus::Handler
{
public:
    OnLoadedHandler()
    {
        BusConnect();
    }

    void OnDataBufferSaved([[maybe_unused]] const DataBufferSavedParams& dataBufferSavedParams) override {}
    void OnDataBufferLoaded(const DataBufferLoadedParams& dataBufferLoadedParams) override
    {
        lastLoadedParams = dataBufferLoadedParams;
        notificationReceived = true;
    }

    DataBufferLoadedParams lastLoadedParams;
    bool notificationReceived = false;
};

const AZ::u64 testSaveDataSize = 9;
const char* testSaveDataName = "TestSaveData";

char testSaveData[testSaveDataSize] = {'a', 'b', 'c', '1', '2', '3', 'x', 'y', 'z'};

AZStd::string GetTestSaveDataCustomDirectoryNameRelative()
{
    return "Amazon/O3DE/SaveDataTest";
}

#if defined(AZ_PLATFORM_WINDOWS)
#   include <windows.h>
AZStd::string GetTestSaveDataCustomDirectoryNameAbsolute()
{
    const DWORD bufferSize = 256;
    char buffer[bufferSize] = {0};
    GetTempPathA(bufferSize, buffer);
    return buffer;
}
#else
AZStd::string GetTestSaveDataCustomDirectoryNameAbsolute()
{
    return "/tmp";
}
#endif // defined(AZ_PLATFORM_WINDOWS)

void SaveTestDataBuffer(const AzFramework::LocalUserId& localUserId = AzFramework::LocalUserIdNone,
                        bool useDataBufferDeleterAzFree = false)
{
    // Setup the save data params
    SaveData::SaveDataRequests::SaveDataBufferParams params;
    if (useDataBufferDeleterAzFree)
    {
        // The default deleter is DataBufferDeleterNone, which cannot be changed when calling unique_ptr::reset,
        // but we are able to reset the pointer and assign a new deleter at the same time using move assignment.
        void* testSaveDataAllocated = azmalloc(testSaveDataSize);
        memcpy(testSaveDataAllocated, testSaveData, testSaveDataSize);
        params.dataBuffer = SaveData::SaveDataRequests::DataBuffer(testSaveDataAllocated,
                                                                   &SaveData::SaveDataRequests::DataBufferDeleterAzFree);
    }
    else
    {
        // The default deleter is DataBufferDeleterNone, so if we don't need to change it we can just call reset
        params.dataBuffer.reset(testSaveData);
    }
    params.dataBufferSize = testSaveDataSize;
    params.dataBufferName = testSaveDataName;
    params.localUserId = localUserId;
    params.callback = [localUserId](const SaveData::SaveDataNotifications::DataBufferSavedParams& onSavedParams)
    {
#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        if (localUserId == AzFramework::LocalUserIdNone)
        {
            EXPECT_TRUE(onSavedParams.result == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
            return;
        }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID

        EXPECT_TRUE(onSavedParams.dataBufferName == testSaveDataName);
        EXPECT_TRUE(onSavedParams.localUserId == localUserId);
        EXPECT_TRUE(onSavedParams.result == SaveData::SaveDataNotifications::Result::Success);
    };

    // Create the notification handler and send the save data request
    OnSavedHandler onSavedHandler;
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SaveDataBuffer, params);

    // Execute queued tick bus events until we receive the notification
    while (!onSavedHandler.notificationReceived)
    {
        AZ::TickBus::ExecuteQueuedEvents();
    }

#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
    if (localUserId == AzFramework::LocalUserIdNone)
    {
        EXPECT_TRUE(onSavedHandler.lastSavedParams.result == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
        return;
    }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID

    EXPECT_TRUE(onSavedHandler.lastSavedParams.dataBufferName == testSaveDataName);
    EXPECT_TRUE(onSavedHandler.lastSavedParams.localUserId == localUserId);
    EXPECT_TRUE(onSavedHandler.lastSavedParams.result == SaveData::SaveDataNotifications::Result::Success);
}

void LoadTestDataBuffer(const AzFramework::LocalUserId& localUserId = AzFramework::LocalUserIdNone)
{
    // Setup the load data params
    SaveData::SaveDataRequests::LoadDataBufferParams params;
    params.dataBufferName = testSaveDataName;
    params.localUserId = localUserId;
    params.callback = [localUserId](const SaveData::SaveDataNotifications::DataBufferLoadedParams& onLoadedParams)
    {
#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        if (localUserId == AzFramework::LocalUserIdNone)
        {
            EXPECT_TRUE(onLoadedParams.result == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
            return;
        }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        EXPECT_TRUE(onLoadedParams.dataBuffer != nullptr);
        EXPECT_TRUE(onLoadedParams.dataBufferName == testSaveDataName);
        EXPECT_TRUE(onLoadedParams.dataBufferSize == testSaveDataSize);
        EXPECT_TRUE(onLoadedParams.localUserId == localUserId);
        EXPECT_TRUE(onLoadedParams.result == SaveData::SaveDataNotifications::Result::Success);
        if (onLoadedParams.result == SaveData::SaveDataNotifications::Result::Success)
        {
            EXPECT_TRUE(memcmp(testSaveData, onLoadedParams.dataBuffer.get(), testSaveDataSize) == 0);
        }
    };

    // Create the notification handler and send the load data request
    OnLoadedHandler onLoadedHandler;
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::LoadDataBuffer, params);

    // Execute queued tick bus events until we receive the notification
    while (!onLoadedHandler.notificationReceived)
    {
        AZ::TickBus::ExecuteQueuedEvents();
    }
#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
    if (localUserId == AzFramework::LocalUserIdNone)
    {
        EXPECT_TRUE(onLoadedHandler.lastLoadedParams.result == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
        return;
    }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID

    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.dataBuffer != nullptr);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.dataBufferName == testSaveDataName);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.dataBufferSize == testSaveDataSize);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.localUserId == localUserId);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.result == SaveData::SaveDataNotifications::Result::Success);
    if (onLoadedHandler.lastLoadedParams.result == SaveData::SaveDataNotifications::Result::Success)
    {
        EXPECT_TRUE(memcmp(testSaveData, onLoadedHandler.lastLoadedParams.dataBuffer.get(), testSaveDataSize) == 0);
    }
}

TEST_F(SaveDataTest, SaveDataBuffer)
{
    SaveTestDataBuffer();
}

TEST_F(SaveDataTest, LoadDataBuffer)
{
    LoadTestDataBuffer();
}

TEST_F(SaveDataTest, SaveDataBufferForUser)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveTestDataBuffer(userId);
}

TEST_F(SaveDataTest, LoadDataBufferForUser)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    LoadTestDataBuffer(userId);
}

TEST_F(SaveDataTest, SaveDataBufferToCustomDirectoryRelative)
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    SaveTestDataBuffer();
}

TEST_F(SaveDataTest, LoadDataBufferFromCustomDirectoryRelative)
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    LoadTestDataBuffer();
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_SaveDataBufferToCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, SaveDataBufferToCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    SaveTestDataBuffer();
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_LoadDataBufferFromCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, LoadDataBufferFromCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    LoadTestDataBuffer();
}

TEST_F(SaveDataTest, SaveDataBufferForUserToCustomDirectoryRelative)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    SaveTestDataBuffer(userId);
}

TEST_F(SaveDataTest, LoadDataBufferForUserFromCustomDirectoryRelative)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    LoadTestDataBuffer(userId);
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_SaveDataBufferForUserToCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, SaveDataBufferForUserToCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    SaveTestDataBuffer(userId);
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_LoadDataBufferForUserFromCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, LoadDataBufferForUserFromCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    LoadTestDataBuffer(userId);
}

TEST_F(SaveDataTest, SaveDataBufferUsingDataBufferDeleterAzFree)
{
    SaveTestDataBuffer(AzFramework::LocalUserIdNone, true);
}

class TestObject
{
public:
    virtual ~TestObject() = default;

    static constexpr const char* DataBufferName = "TestSaveObject";

    AZ_TYPE_INFO(TestObject, "{9CE29971-8FE2-41FF-AD5B-CB15F1B92834}");
    static void Reflect(AZ::SerializeContext& sc)
    {
        sc.Class<TestObject>()
            ->Version(1)
            ->Field("testString", &TestObject::testString)
            ->Field("testFloat", &TestObject::testFloat)
            ->Field("testInt", &TestObject::testInt)
            ->Field("testBool", &TestObject::testBool)
        ;
    }

    bool operator==(const TestObject& other) const
    {
        return (testString == other.testString) &&
               (testFloat == other.testFloat) &&
               (testInt == other.testInt) &&
               (testBool == other.testBool);
    }

    void SetNonDefaultValues()
    {
        testString = "NonDefaultString";
        testFloat = 9.9f;
        testInt = 1234567890;
        testBool = true;
    }

    AZStd::string testString;
    float testFloat = 0.0f;
    int testInt = 0;
    bool testBool = false;
};

void SaveTestObject(const AzFramework::LocalUserId& localUserId = AzFramework::LocalUserIdNone)
{
    // Reflect the test object.
    AZ::SerializeContext serializeContext;
    TestObject::Reflect(serializeContext);

    // Create a test object and change the default values.
    TestObject defaultTestObject;
    AZStd::shared_ptr<TestObject> testObject = AZStd::make_shared<TestObject>();
    EXPECT_TRUE(*testObject == defaultTestObject);
    testObject->SetNonDefaultValues();
    EXPECT_FALSE(*testObject == defaultTestObject);

    // Setup the save data params
    SaveData::SaveDataRequests::SaveOrLoadObjectParams<TestObject> params;
    params.serializableObject = testObject;
    params.serializeContext = &serializeContext;
    params.dataBufferName = TestObject::DataBufferName;
    params.localUserId = localUserId;
    params.callback = [params](const SaveData::SaveDataRequests::SaveOrLoadObjectParams<TestObject>& callbackParams,
                               SaveData::SaveDataNotifications::Result callbackResult)
    {
#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        if (params.localUserId == AzFramework::LocalUserIdNone)
        {
            EXPECT_TRUE(callbackResult == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
            return;
        }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        EXPECT_TRUE(callbackResult == SaveData::SaveDataNotifications::Result::Success);
        EXPECT_TRUE(*(callbackParams.serializableObject) == *(params.serializableObject));
        EXPECT_TRUE(callbackParams.serializableObject == params.serializableObject);
        EXPECT_TRUE(callbackParams.serializeContext == params.serializeContext);
        EXPECT_TRUE(callbackParams.dataBufferName == params.dataBufferName);
        EXPECT_TRUE(callbackParams.localUserId == params.localUserId);
    };

    // Create the notification handler and send the save data request
    OnSavedHandler onSavedHandler;
    SaveData::SaveDataRequests::SaveObject(params);

    // Execute queued tick bus events until we receive the notification
    while (!onSavedHandler.notificationReceived)
    {
        AZ::TickBus::ExecuteQueuedEvents();
    }

#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
    if (params.localUserId == AzFramework::LocalUserIdNone)
    {
        EXPECT_TRUE(onSavedHandler.lastSavedParams.result == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
        return;
    }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
    EXPECT_TRUE(onSavedHandler.lastSavedParams.dataBufferName == TestObject::DataBufferName);
    EXPECT_TRUE(onSavedHandler.lastSavedParams.localUserId == localUserId);
    EXPECT_TRUE(onSavedHandler.lastSavedParams.result == SaveData::SaveDataNotifications::Result::Success);
}

void LoadTestObject(const AzFramework::LocalUserId& localUserId = AzFramework::LocalUserIdNone)
{
    // Reflect the test object.
    AZ::SerializeContext serializeContext;
    TestObject::Reflect(serializeContext);

    // Create a test object to load.
    TestObject defaultTestObject;
    TestObject nonDefaultTestObject;
    nonDefaultTestObject.SetNonDefaultValues();
    AZStd::shared_ptr<TestObject> testObject = AZStd::make_shared<TestObject>();
    EXPECT_TRUE(*testObject == defaultTestObject);
    EXPECT_FALSE(*testObject == nonDefaultTestObject);

    // Setup the load data params
    SaveData::SaveDataRequests::SaveOrLoadObjectParams<TestObject> params;
    params.serializableObject = testObject;
    params.serializeContext = &serializeContext;
    params.dataBufferName = TestObject::DataBufferName;
    params.localUserId = localUserId;
    params.callback = [params, defaultTestObject, nonDefaultTestObject]
        (const SaveData::SaveDataRequests::SaveOrLoadObjectParams<TestObject>& callbackParams,
         SaveData::SaveDataNotifications::Result callbackResult)
    {
#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        if (params.localUserId == AzFramework::LocalUserIdNone)
        {
            EXPECT_TRUE(callbackResult == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
            return;
        }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
        EXPECT_TRUE(callbackResult == SaveData::SaveDataNotifications::Result::Success);
        EXPECT_TRUE(*(callbackParams.serializableObject) == *(params.serializableObject));
        EXPECT_FALSE(*(callbackParams.serializableObject) == defaultTestObject);
        EXPECT_TRUE(*(callbackParams.serializableObject) == nonDefaultTestObject);
        EXPECT_TRUE(callbackParams.serializableObject == params.serializableObject);
        EXPECT_TRUE(callbackParams.serializeContext == params.serializeContext);
        EXPECT_TRUE(callbackParams.dataBufferName == params.dataBufferName);
        EXPECT_TRUE(callbackParams.localUserId == params.localUserId);
    };

    // Create the notification handler and send the load data request
    OnLoadedHandler onLoadedHandler;
    SaveData::SaveDataRequests::LoadObject(params);

    // Execute queued tick bus events until we receive the notification
    while (!onLoadedHandler.notificationReceived)
    {
        AZ::TickBus::ExecuteQueuedEvents();
    }

#if AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
    if (params.localUserId == AzFramework::LocalUserIdNone)
    {
        EXPECT_TRUE(onLoadedHandler.lastLoadedParams.result == SaveData::SaveDataNotifications::Result::ErrorUnknownUser);
        return;
    }
#endif // AZ_TRAIT_SAVEDATA_TEST_REQUIRES_SPECIFIC_USER_ID
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.dataBuffer != nullptr);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.dataBufferName == TestObject::DataBufferName);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.localUserId == localUserId);
    EXPECT_TRUE(onLoadedHandler.lastLoadedParams.result == SaveData::SaveDataNotifications::Result::Success);
}

TEST_F(SaveDataTest, SaveObject)
{
    SaveTestObject();
}

TEST_F(SaveDataTest, LoadObject)
{
    LoadTestObject();
}

TEST_F(SaveDataTest, SaveObjectForUser)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveTestObject(userId);
}

TEST_F(SaveDataTest, LoadObjectForUser)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    LoadTestObject(userId);
}

TEST_F(SaveDataTest, SaveObjectToCustomDirectoryRelative)
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    SaveTestObject();
}

TEST_F(SaveDataTest, LoadObjectFromCustomDirectoryRelative)
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    LoadTestObject();
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_SaveObjectToCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, SaveObjectToCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    SaveTestObject();
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_LoadObjectFromCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, LoadObjectFromCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    LoadTestObject();
}

TEST_F(SaveDataTest, SaveObjectForUserToCustomDirectoryRelative)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    SaveTestObject(userId);
}

TEST_F(SaveDataTest, LoadObjectForUserFromCustomDirectoryRelative)
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameRelative().c_str());
    LoadTestObject(userId);
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_SaveObjectForUserToCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, SaveObjectForUserToCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    SaveTestObject(userId);
}

#if AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
TEST_F(SaveDataTest, DISABLED_LoadObjectForUserFromCustomDirectoryAbsolute)
#else
TEST_F(SaveDataTest, LoadObjectForUserFromCustomDirectoryAbsolute)
#endif // AZ_TRAIT_DISABLE_FAILED_SAVE_DATA_TESTS
{
    AzFramework::LocalUserId userId = SaveDataTest::GetDefaultTestUserId();
    SaveData::SaveDataRequestBus::Broadcast(&SaveData::SaveDataRequests::SetSaveDataDirectoryPath,
                                            GetTestSaveDataCustomDirectoryNameAbsolute().c_str());
    LoadTestObject(userId);
}

#endif // !AZ_TRAIT_DISABLE_ALL_SAVE_DATA_TESTS

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
