/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <AudioAllocators.h>
#include <ATLComponents.h>
#include <ATLUtils.h>
#include <ATL.h>
#include <AudioProxy.h>

#include <Mocks/ATLEntitiesMock.h>
#include <Mocks/AudioSystemImplementationMock.h>
#include <Mocks/FileCacheManagerMock.h>

#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>


using namespace Audio;
using ::testing::NiceMock;

void CreateAudioAllocators()
{
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    if (!AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Create();
    }
}

void DestroyAudioAllocators()
{
    if (AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Destroy();
    }

    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
}


// This is the global test environment (global to the module under test).
// Use it to stub out an environment with mocks.
class AudioSystemTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    AZ_TEST_CLASS_ALLOCATOR(AudioSystemTestEnvironment)

    ~AudioSystemTestEnvironment() override = default;

protected:
    struct MockHolder
    {
        AZ_TEST_CLASS_ALLOCATOR(MockHolder)

        NiceMock<ConsoleMock> m_console;
        NiceMock<SystemMock> m_system;
    };

    void SetupEnvironment() override
    {
        CreateAudioAllocators();

        m_mocks = new MockHolder;

        // setup mocks
        m_stubEnv.pConsole = &m_mocks->m_console;
        m_stubEnv.pSystem = &m_mocks->m_system;
        gEnv = &m_stubEnv;

    }

    void TeardownEnvironment() override
    {
        delete m_mocks;
        DestroyAudioAllocators();
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    MockHolder* m_mocks = nullptr;
};


AZ_UNIT_TEST_HOOK(new AudioSystemTestEnvironment);

/*
// This is an example of a test fixture:
// Use it to set up a common scenario across multiple related unit tests.
// Each test will execute SetUp and TearDown before and after running the test.

class AudioExampleTestFixture
    : public ::testing::Test
{
public:
    AudioExampleTestFixture()
    {
        // use ctor to initialize data
    }

    void SetUp() override
    {
        // additional setup, called before the test begins
    }

    void TearDown() override
    {
        // called after test ends
    }

    // data used for testing...
};

// To test using this fixture, do:
TEST_F(AudioExampleTestFixture, ThingUnderTest_WhatThisTests_ExpectedResult)
{
    ...
}
*/


TEST(ATLWorldPositionTest, TransformGetColumn_GetColumn_Passes)
{
    SATLWorldPosition position;
    AZ::Vector3 column0 = position.GetRightVec();
    EXPECT_TRUE(column0.IsClose(AZ::Vector3::CreateAxisX()));
    AZ::Vector3 column1 = position.GetForwardVec();
    EXPECT_TRUE(column1.IsClose(AZ::Vector3::CreateAxisY()));
    AZ::Vector3 column2 = position.GetUpVec();
    EXPECT_TRUE(column2.IsClose(AZ::Vector3::CreateAxisZ()));
}

TEST(ATLWorldPositionTest, TransformNormalize_NormalizeNonUnitVectors_GivesUnitLengthVectors)
{
    AZ::Matrix3x4 matrix3x4Test;
    matrix3x4Test.SetBasisX(AZ::Vector3::CreateAxisX());
    matrix3x4Test.SetBasisY(1.f, 2.f, 1.f);
    matrix3x4Test.SetBasisZ(1.f, 1.f, 2.f);

    SATLWorldPosition position(matrix3x4Test);

    position.NormalizeForwardVec();
    AZ::Vector3 forward = position.GetForwardVec();

    EXPECT_TRUE(AZ::IsClose(forward.GetLength(), 1.f, 1e-3f));

    position.NormalizeUpVec();
    AZ::Vector3 up = position.GetUpVec();

    EXPECT_TRUE(AZ::IsClose(up.GetLength(), 1.f, 1e-3f));
}

TEST(ATLWorldPositionTest, TransformNormalize_NormalizeZeroVectors_GivesBasisVectors)
{
    AZ::Matrix3x4 matrix3x4Test = AZ::Matrix3x4::CreateZero();

    SATLWorldPosition position(matrix3x4Test);

    EXPECT_EQ(position.GetForwardVec(), AZ::Vector3::CreateZero());
    EXPECT_EQ(position.GetUpVec(), AZ::Vector3::CreateZero());

    position.NormalizeForwardVec();
    EXPECT_EQ(position.GetForwardVec(), AZ::Vector3::CreateAxisY());

    position.NormalizeUpVec();
    EXPECT_EQ(position.GetUpVec(), AZ::Vector3::CreateAxisZ());
}


// Tests related to new PhysX-compatible raycast code.

constexpr TAudioObjectID testAudioObjectId = 123;

class ATLAudioObjectTest : public ::testing::Test
{
public:
    RaycastProcessor& GetRaycastProcessor(CATLAudioObject& audioObject) const
    {
        return audioObject.m_raycastProcessor;
    }
    const RaycastProcessor& GetRaycastProcessor(const CATLAudioObject& audioObject) const
    {
        return audioObject.m_raycastProcessor;
    }
};

TEST_F(ATLAudioObjectTest, SetRaycastCalcType_SetAllTypes_AffectsCanRunRaycasts)
{
    RaycastProcessor::s_raycastsEnabled = true;
    CATLAudioObject audioObject(testAudioObjectId, nullptr);

    audioObject.SetRaycastCalcType(Audio::ObstructionType::SingleRay);
    EXPECT_TRUE(audioObject.CanRunRaycasts());

    audioObject.SetRaycastCalcType(Audio::ObstructionType::Ignore);
    EXPECT_FALSE(audioObject.CanRunRaycasts());

    audioObject.SetRaycastCalcType(Audio::ObstructionType::MultiRay);
    EXPECT_TRUE(audioObject.CanRunRaycasts());
}


TEST_F(ATLAudioObjectTest, OnAudioRaycastResults_MultiRaycastZeroDistanceHits_ZeroObstructionAndOcclusion)
{
    RaycastProcessor::s_raycastsEnabled = true;
    CATLAudioObject audioObject(testAudioObjectId, nullptr);
    RaycastProcessor& raycastProcessor = GetRaycastProcessor(audioObject);

    audioObject.SetRaycastCalcType(Audio::ObstructionType::MultiRay);
    for (AZStd::decay_t<decltype(Audio::s_maxRaysPerObject)> i = 0; i < Audio::s_maxRaysPerObject; ++i)
    {
        raycastProcessor.SetupTestRay(i);
    }

    // maximum number of hits, but we don't set the distance in any of them.
    AZStd::vector<AzPhysics::SceneQueryHit> hits(Audio::s_maxHitResultsPerRaycast);

    AudioRaycastResult hitResults(AZStd::move(hits), testAudioObjectId, 0);
    audioObject.OnAudioRaycastResults(hitResults);

    raycastProcessor.Update(17.f);

    // Now get the contribution amounts.  In this case multiple hits w/ zero distance,
    // both obstruction & occlusion should be zero.
    SATLSoundPropagationData data;
    audioObject.GetObstOccData(data);

    EXPECT_NEAR(data.fObstruction, 0.f, AZ::Constants::FloatEpsilon);
    EXPECT_NEAR(data.fOcclusion, 0.f, AZ::Constants::FloatEpsilon);
}


TEST_F(ATLAudioObjectTest, OnAudioRaycastResults_SingleRaycastHit_NonZeroObstruction)
{
    RaycastProcessor::s_raycastsEnabled = true;
    CATLAudioObject audioObject(testAudioObjectId, nullptr);
    RaycastProcessor& raycastProcessor = GetRaycastProcessor(audioObject);

    audioObject.SetRaycastCalcType(Audio::ObstructionType::SingleRay);
    raycastProcessor.SetupTestRay(0);

    AZStd::vector<AzPhysics::SceneQueryHit> hits(3);     // three hits
    hits[0].m_distance = 10.f;
    hits[1].m_distance = 11.f;
    hits[2].m_distance = 12.f;
    AudioRaycastResult hitResults(AZStd::move(hits), testAudioObjectId, 0);

    audioObject.OnAudioRaycastResults(hitResults);

    raycastProcessor.Update(0.17f);

    // Now get the contribution amounts.  In this case a single ray had three hits,
    // and the obstruction value will be non-zero.
    SATLSoundPropagationData data;
    audioObject.GetObstOccData(data);

    EXPECT_GT(data.fObstruction, 0.f);
    EXPECT_LE(data.fObstruction, 1.f);
    EXPECT_NEAR(data.fOcclusion, 0.f, AZ::Constants::FloatEpsilon);
}


TEST_F(ATLAudioObjectTest, OnAudioRaycastResults_MultiRaycastHit_NonZeroOcclusion)
{
    RaycastProcessor::s_raycastsEnabled = true;
    CATLAudioObject audioObject(testAudioObjectId, nullptr);
    RaycastProcessor& raycastProcessor = GetRaycastProcessor(audioObject);

    audioObject.SetRaycastCalcType(Audio::ObstructionType::MultiRay);
    for (AZStd::decay_t<decltype(Audio::s_maxRaysPerObject)> i = 1; i < Audio::s_maxRaysPerObject; ++i)
    {
        raycastProcessor.SetupTestRay(i);
    }

    AZStd::vector<AzPhysics::SceneQueryHit> hits(3);     // three hits
    hits[0].m_distance = 10.f;
    hits[1].m_distance = 11.f;
    hits[2].m_distance = 12.f;
    AudioRaycastResult hitResults(AZStd::move(hits), testAudioObjectId, 1);

    audioObject.OnAudioRaycastResults(hitResults);
    hitResults.m_rayIndex++;    // 2
    audioObject.OnAudioRaycastResults(hitResults);
    hitResults.m_rayIndex++;    // 3
    audioObject.OnAudioRaycastResults(hitResults);
    hitResults.m_rayIndex++;    // 4
    audioObject.OnAudioRaycastResults(hitResults);

    raycastProcessor.Update(17.f);

    // Now get the contribution amounts.  In this case a single ray had three hits,
    // and the obstruction value will be non-zero.
    SATLSoundPropagationData data;
    audioObject.GetObstOccData(data);

    EXPECT_NEAR(data.fObstruction, 0.f, AZ::Constants::FloatEpsilon);
    EXPECT_GT(data.fOcclusion, 0.f);
    EXPECT_LE(data.fOcclusion, 1.f);
}


class AudioRaycastManager_Test
    : public AudioRaycastManager
{
public:
    // Helpers
    size_t GetNumRequests() const
    {
        return m_raycastRequests.size();
    }

    size_t GetNumResults() const
    {
        return m_raycastResults.size();
    }
};


TEST(AudioRaycastManagerTest, AudioRaycastRequest_FullProcessFlow_CorrectRequestAndResultCounts)
{
    AudioRaycastManager_Test raycastManager;

    AzPhysics::RayCastRequest physicsRequest;
    physicsRequest.m_direction = AZ::Vector3::CreateAxisX();
    physicsRequest.m_distance = 5.f;
    physicsRequest.m_maxResults = Audio::s_maxHitResultsPerRaycast;
    physicsRequest.m_reportMultipleHits = true;

    AudioRaycastRequest raycastRequest(AZStd::move(physicsRequest), testAudioObjectId, 0);

    EXPECT_EQ(0, raycastManager.GetNumRequests());
    EXPECT_EQ(0, raycastManager.GetNumResults());

    raycastManager.PushAudioRaycastRequest(raycastRequest);

    EXPECT_EQ(1, raycastManager.GetNumRequests());
    EXPECT_EQ(0, raycastManager.GetNumResults());

    raycastManager.OnPhysicsSubtickFinished();

    EXPECT_EQ(0, raycastManager.GetNumRequests());
    EXPECT_EQ(1, raycastManager.GetNumResults());

    raycastManager.ProcessRaycastResults(17.f);     // milliseconds

    EXPECT_EQ(0, raycastManager.GetNumRequests());
    EXPECT_EQ(0, raycastManager.GetNumResults());
}




//---------------//
// Test ATLUtils //
//---------------//


class ATLUtilsTestFixture
    : public ::testing::Test
{
protected:
    using KeyType = AZStd::string;
    using ValType = int;
    using MapType = AZStd::map<KeyType, ValType, AZStd::less<KeyType>, Audio::AudioSystemStdAllocator>;

    void SetUp() override
    {
        m_testMap["Hello"] = 10;
        m_testMap["World"] = 15;
        m_testMap["GoodBye"] = 20;
        m_testMap["Orange"] = 25;
        m_testMap["Apple"] = 30;
    }

    void TearDown() override
    {
        m_testMap.clear();
    }

    MapType m_testMap;
};


TEST_F(ATLUtilsTestFixture, FindPlace_ContainerContainsItem_FindsItem)
{
    MapType::iterator placeIterator;

    EXPECT_TRUE(FindPlace(m_testMap, KeyType { "Hello" }, placeIterator));
    EXPECT_NE(placeIterator, m_testMap.end());
}


TEST_F(ATLUtilsTestFixture, FindPlace_ContainerDoesntContainItem_FindsNone)
{
    MapType::iterator placeIterator;

    EXPECT_FALSE(FindPlace(m_testMap, KeyType { "goodbye" }, placeIterator));
    EXPECT_EQ(placeIterator, m_testMap.end());
}

TEST_F(ATLUtilsTestFixture, FindPlaceConst_ContainerContainsItem_FindsItem)
{
    MapType::const_iterator placeIterator;

    EXPECT_TRUE(FindPlaceConst(m_testMap, KeyType { "Orange" }, placeIterator));
    EXPECT_NE(placeIterator, m_testMap.end());
}

TEST_F(ATLUtilsTestFixture, FindPlaceConst_ContainerDoesntContainItem_FindsNone)
{
    MapType::const_iterator placeIterator;

    EXPECT_FALSE(FindPlaceConst(m_testMap, KeyType { "Bananas" }, placeIterator));
    EXPECT_EQ(placeIterator, m_testMap.end());
}




//-------------------//
// Test Audio::Flags //
//-------------------//

TEST(AudioFlagsTest, AudioFlags_ZeroFlags_NoFlagsAreSet)
{
    const AZ::u8 noFlags = 0;
    const AZ::u8 allFlags = static_cast<AZ::u8>(~0);
    Audio::Flags<AZ::u8> testFlags;
    EXPECT_FALSE(testFlags.AreAnyFlagsActive(allFlags));
    EXPECT_FALSE(testFlags.AreMultipleFlagsActive());
    EXPECT_FALSE(testFlags.IsOneFlagActive());
    EXPECT_EQ(testFlags.GetRawFlags(), noFlags);
}

TEST(AudioFlagsTest, AudioFlags_OneFlag_OneFlagIsSet)
{
    const AZ::u8 flagBit = 1 << 4;
    Audio::Flags<AZ::u8> testFlags(flagBit);
    EXPECT_FALSE(testFlags.AreAnyFlagsActive(static_cast<AZ::u8>(~flagBit)));
    EXPECT_TRUE(testFlags.AreAnyFlagsActive(flagBit));
    EXPECT_TRUE(testFlags.AreAnyFlagsActive(flagBit | 1));
    EXPECT_TRUE(testFlags.AreAllFlagsActive(flagBit));
    EXPECT_FALSE(testFlags.AreAllFlagsActive(flagBit | 1));
    EXPECT_FALSE(testFlags.AreMultipleFlagsActive());
    EXPECT_TRUE(testFlags.IsOneFlagActive());
    EXPECT_EQ(testFlags.GetRawFlags(), flagBit);
}

TEST(AudioFlagsTest, AudioFlags_MultipleFlags_MultipleFlagsAreSet)
{
    const AZ::u8 flagBits = (1 << 5) | (1 << 2) | (1 << 3);
    Audio::Flags<AZ::u8> testFlags(flagBits);
    EXPECT_FALSE(testFlags.AreAnyFlagsActive(static_cast<AZ::u8>(~flagBits)));
    EXPECT_TRUE(testFlags.AreAnyFlagsActive(flagBits));
    EXPECT_TRUE(testFlags.AreAllFlagsActive(flagBits));
    EXPECT_FALSE(testFlags.AreAllFlagsActive(flagBits | 1));
    EXPECT_TRUE(testFlags.AreMultipleFlagsActive());
    EXPECT_FALSE(testFlags.IsOneFlagActive());
    EXPECT_EQ(testFlags.GetRawFlags(), flagBits);
}

TEST(AudioFlagsTest, AudioFlags_AddAndClear_FlagsAreCorrect)
{
    const AZ::u8 flagBits = (1 << 2) | (1 << 6);
    Audio::Flags<AZ::u8> testFlags;
    Audio::Flags<AZ::u8> zeroFlags;

    testFlags.AddFlags(flagBits);
    EXPECT_TRUE(testFlags != zeroFlags);

    testFlags.ClearFlags(flagBits);
    EXPECT_TRUE(testFlags == zeroFlags);
}

TEST(AudioFlagsTest, AudioFlags_SetAndClearAll_FlagsAreCorrect)
{
    const AZ::u8 flagBits = (1 << 3) | (1 << 5) | (1 << 7);
    Audio::Flags<AZ::u8> testFlags;
    Audio::Flags<AZ::u8> zeroFlags;

    testFlags.SetFlags(flagBits, true);
    EXPECT_TRUE(testFlags != zeroFlags);
    EXPECT_EQ(testFlags.GetRawFlags(), flagBits);

    testFlags.SetFlags((1 << 3), false);
    EXPECT_TRUE(testFlags != zeroFlags);
    EXPECT_NE(testFlags.GetRawFlags(), flagBits);

    testFlags.ClearAllFlags();
    EXPECT_TRUE(testFlags == zeroFlags);
}



//-------------------------//
// Test CATLDebugNameStore //
//-------------------------//

#if !defined(AUDIO_RELEASE)
class ATLDebugNameStoreTestFixture
    : public ::testing::Test
{
public:
    ATLDebugNameStoreTestFixture()
        : m_audioObjectName("SomeAudioObject1")
        , m_audioTriggerName("SomeAudioTrigger1")
        , m_audioRtpcName("SomeAudioRtpc1")
        , m_audioSwitchName("SomeAudioSwitch1")
        , m_audioSwitchStateName("SomeAudioSwitchState1")
        , m_audioEnvironmentName("SomeAudioEnvironment1")
        , m_audioPreloadRequestName("SomeAudioPreloadRequest1")
    {}

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

protected:
    CATLDebugNameStore m_atlNames;
    AZStd::string m_audioObjectName;
    AZStd::string m_audioTriggerName;
    AZStd::string m_audioRtpcName;
    AZStd::string m_audioSwitchName;
    AZStd::string m_audioSwitchStateName;
    AZStd::string m_audioEnvironmentName;
    AZStd::string m_audioPreloadRequestName;
};


TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioObject_Success)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    bool added = m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioObjectAndLookupName_FindsName)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioObjectName(audioObjectID), m_audioObjectName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioTrigger_Success)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    bool added = m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioTriggerAndLookupName_FindsName)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioTriggerName(audioTriggerID), m_audioTriggerName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioRtpc_Success)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    bool added = m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioRtpcAndLookupName_FindsName)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioRtpcName(audioRtpcID), m_audioRtpcName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitch_Success)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    bool added = m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitchAndLookupName_FindsName)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), m_audioSwitchName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitchState_Success)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    bool added = m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitchStateAndLookupNames_FindsNames)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), m_audioSwitchName.c_str());
    EXPECT_STREQ(m_atlNames.LookupAudioSwitchStateName(audioSwitchID, audioSwitchStateID), m_audioSwitchStateName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioPreload_Success)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    bool added = m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioPreloadAndLookupName_FindsName)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioPreloadRequestName(audioPreloadID), m_audioPreloadRequestName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioEnvironment_Success)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    bool added = m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());
    EXPECT_TRUE(added);
    added = m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());
    EXPECT_FALSE(added);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioEnvironmentAndLookupName_FindsName)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioEnvironmentName(audioEnvironmentID), m_audioEnvironmentName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioObjectNotFound_Fails)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    bool removed = m_atlNames.RemoveAudioObject(audioObjectID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioTriggerNotFound_Fails)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    bool removed = m_atlNames.RemoveAudioTrigger(audioTriggerID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioRtpcNotFound_Fails)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    bool removed = m_atlNames.RemoveAudioRtpc(audioRtpcID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchNotFound_Fails)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    bool removed = m_atlNames.RemoveAudioSwitch(audioSwitchID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchStateNotFound_Fails)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    bool removed = m_atlNames.RemoveAudioSwitchState(audioSwitchID, audioSwitchStateID);

    EXPECT_FALSE(removed);

    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    removed = m_atlNames.RemoveAudioSwitchState(audioSwitchID, audioSwitchStateID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioPreloadRequestNotFound_Fails)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    bool removed = m_atlNames.RemoveAudioPreloadRequest(audioPreloadID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioEnvironmentNotFound_Fails)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    bool removed = m_atlNames.RemoveAudioEnvironment(audioEnvironmentID);

    EXPECT_FALSE(removed);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioObjectAndLookupName_FindsNone)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    bool added = m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());
    bool removed = m_atlNames.RemoveAudioObject(audioObjectID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioObjectName(audioObjectID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioTriggerAndLookupName_FindsNone)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    bool added = m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());
    bool removed = m_atlNames.RemoveAudioTrigger(audioTriggerID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioTriggerName(audioTriggerID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioRtpcAndLookupName_FindsNone)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    bool added = m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());
    bool removed = m_atlNames.RemoveAudioRtpc(audioRtpcID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioRtpcName(audioRtpcID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchAndLookupName_FindsNone)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    bool added = m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    bool removed = m_atlNames.RemoveAudioSwitch(audioSwitchID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchStateAndLookupName_FindsNone)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    bool added = m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());
    bool removed = m_atlNames.RemoveAudioSwitchState(audioSwitchID, audioSwitchStateID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioSwitchStateName(audioSwitchID, audioSwitchStateID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioPreloadRequestAndLookupName_FindsNone)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    bool added = m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());
    bool removed = m_atlNames.RemoveAudioPreloadRequest(audioPreloadID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioPreloadRequestName(audioPreloadID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioEnvironmentAndLookupName_FindsNone)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    bool added = m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());
    bool removed = m_atlNames.RemoveAudioEnvironment(audioEnvironmentID);

    EXPECT_TRUE(added && removed);
    EXPECT_EQ(m_atlNames.LookupAudioEnvironmentName(audioEnvironmentID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupGlobalAudioObjectName_FindsName)
{
    const char* globalAudioObjectName = m_atlNames.LookupAudioObjectName(GLOBAL_AUDIO_OBJECT_ID);
    EXPECT_STREQ(globalAudioObjectName, "GlobalAudioObject");
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioObjectName_FindsName)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioObjectName(audioObjectID), m_audioObjectName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioTriggerName_FindsName)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioTriggerName(audioTriggerID), m_audioTriggerName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioRtpcName_FindsName)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioRtpcName(audioRtpcID), m_audioRtpcName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioSwitchName_FindsName)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), m_audioSwitchName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioSwitchStateName_FindsName)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchStateName(audioSwitchID, audioSwitchStateID), m_audioSwitchStateName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioPreloadRequestName_FindsName)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioPreloadRequestName(audioPreloadID), m_audioPreloadRequestName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioEnvironmentName_FindsName)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioEnvironmentName(audioEnvironmentID), m_audioEnvironmentName.c_str());
}
#endif



//-----------------------//
// Test CATLXmlProcessor //
//-----------------------//

class ATLPreloadXmlParsingTestFixture
    : public ::testing::Test
{
public:
    AZ_TEST_CLASS_ALLOCATOR(ATLPreloadXmlParsingTestFixture)

    ATLPreloadXmlParsingTestFixture()
        : m_triggers()
        , m_rtpcs()
        , m_switches()
        , m_environments()
        , m_preloads()
        , m_mockFileCacheManager(m_preloads)
        , m_xmlProcessor(m_triggers, m_rtpcs, m_switches, m_environments, m_preloads, m_mockFileCacheManager)
    {
#if !defined(AUDIO_RELEASE)
        m_xmlProcessor.SetDebugNameStore(&m_mockDebugNameStore);
#endif
    }

    void SetUp() override
    {
        m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
        if (m_prevFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        // Replace with a new LocalFileIO...
        m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        AZ::IO::FileIOBase::SetInstance(m_fileIO.get());

        AZStd::string rootFolder(AZ::Test::GetCurrentExecutablePath());
        AZ::StringFunc::Path::Join(rootFolder.c_str(), "Test.Assets/Gems/AudioSystem/ATLData", rootFolder);

        // Set up paths...
#if !defined(AUDIO_RELEASE)
        m_xmlProcessor.SetRootPath(m_audioTestAlias);
#endif
        m_fileIO->SetAlias(m_audioTestAlias, rootFolder.c_str());
    }

    void TearDown() override
    {
        // Destroy our LocalFileIO...
        m_fileIO->ClearAlias(m_audioTestAlias);
        m_fileIO.reset();

        // Replace the old fileIO (if any)...
        AZ::IO::FileIOBase::SetInstance(nullptr);
        if (m_prevFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            m_prevFileIO = nullptr;
        }
    }

    void TestSuccessfulPreloadParsing(const char* controlsFolder, int numExpectedPreloads, int numExpectedBanksPerPreload)
    {
        using ::testing::_;
        using ::testing::InvokeWithoutArgs;
        ON_CALL(m_mockFileCacheManager, TryAddFileCacheEntry(_, eADS_GLOBAL, _))
            .WillByDefault(InvokeWithoutArgs(GenerateNewId));

        m_xmlProcessor.ParsePreloadsData(controlsFolder, eADS_GLOBAL);

        EXPECT_EQ(m_preloads.size(), numExpectedPreloads);
        for (auto preloadPair : m_preloads)
        {
            EXPECT_EQ(preloadPair.second->m_cFileEntryIDs.size(), numExpectedBanksPerPreload);
        }

        m_xmlProcessor.ClearPreloadsData(eADS_ALL);
    }

protected:
    TATLTriggerLookup m_triggers;
    TATLRtpcLookup m_rtpcs;
    TATLSwitchLookup m_switches;
    TATLEnvironmentLookup m_environments;
    TATLPreloadRequestLookup m_preloads;
    CATLXmlProcessor m_xmlProcessor;
    NiceMock<FileCacheManagerMock> m_mockFileCacheManager;

private:
    #if !defined(AUDIO_RELEASE)
    NiceMock<ATLDebugNameStoreMock> m_mockDebugNameStore;
    #endif // !AUDIO_RELEASE

    const char* m_audioTestAlias { "@audiotestroot@" };
    AZ::IO::FileIOBase* m_prevFileIO { nullptr };
    AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;

    static TAudioFileEntryID GenerateNewId()
    {
        static TAudioFileEntryID s_id = INVALID_AUDIO_FILE_ENTRY_ID;
        return ++s_id;
    }
};


// Legacy Preload Xml Format...
#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_LegacyOnePreloadOneBank_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_LegacyOnePreloadOneBank_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("Legacy/OneOne", 1, 1);
}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_LegacyMultiplePreloadsMultipleBanks_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_LegacyMultiplePreloadsMultipleBanks_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("Legacy/MultipleMultiple", 2, 2);
}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_LegacyMultiplePreloadsOneBank_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_LegacyMultiplePreloadsOneBank_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("Legacy/MultipleOne", 2, 1);
}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_LegacyOnePreloadMultipleBanks_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_LegacyOnePreloadMultipleBanks_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("Legacy/OneMultiple", 1, 2);
}


// New Preload Xml Format...
#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_OnePreloadOneBank_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_OnePreloadOneBank_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("OneOne", 1, 1);
}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_MultiplePreloadsMultipleBanks_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_MultiplePreloadsMultipleBanks_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("MultipleMultiple", 2, 2);
}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_MultiplePreloadsOneBank_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_MultiplePreloadsOneBank_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("MultipleOne", 2, 1);
}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
TEST_F(ATLPreloadXmlParsingTestFixture, DISABLED_ParsePreloadsXml_OnePreloadMultipleBanks_Success)
#else
TEST_F(ATLPreloadXmlParsingTestFixture, ParsePreloadsXml_OnePreloadMultipleBanks_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_SYSTEM_TESTS
{
    TestSuccessfulPreloadParsing("OneMultiple", 1, 2);
}



//-----------------------------//
// Test CAudioTranslationLayer //
//-----------------------------//

class ATLTestFixture
    : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_requestStatus = EAudioRequestStatus::None;
        m_callbackCaller = [this](AudioRequestVariant&& requestVariant)
        {
            AZStd::visit(
                [this](auto&& request)
                {
                    if (request.m_callback)
                    {
                        request.m_callback(request);
                    }
                    m_requestStatus = request.m_status;
                },
                requestVariant);
        };

        m_atl.Initialize();
        m_impl.BusConnect();
    }

    void TearDown() override
    {
        m_impl.BusDisconnect();
        m_atl.ShutDown();

        m_callbackCaller.clear();
    }

protected:
    NiceMock<AudioSystemImplMock> m_impl;
    NiceMock<AudioSystemMock> m_sys;
    CAudioTranslationLayer m_atl;
    CAudioProxy m_proxy;
    AudioRequestVariant m_requestHolder;

    AZStd::function<void(AudioRequestVariant&&)> m_callbackCaller;
    EAudioRequestStatus m_requestStatus;
};

TEST_F(ATLTestFixture, ATLProcessRequest_CheckCallback_WasCalled)
{
    Audio::SystemRequest::GetFocus getFocus;
    bool callbackRan = false;
    getFocus.m_callback = [&callbackRan]([[maybe_unused]] const Audio::SystemRequest::GetFocus& request)
    {
        callbackRan = true;
    };

    EXPECT_CALL(m_sys, PushCallback).WillOnce(m_callbackCaller);

    m_atl.ProcessRequest(AZStd::move(getFocus));
    EXPECT_TRUE(callbackRan);
}

TEST_F(ATLTestFixture, ATLProcessRequest_CheckResult_Matches)
{
    Audio::SystemRequest::LoseFocus loseFocus;
    loseFocus.m_callback = [](const Audio::SystemRequest::LoseFocus& request)
    {
        // Force a particular result status...
        const_cast<Audio::SystemRequest::LoseFocus&>(request).m_status = EAudioRequestStatus::PartialSuccess;
    };

    EXPECT_CALL(m_sys, PushCallback).WillOnce(m_callbackCaller);
    m_atl.ProcessRequest(AZStd::move(loseFocus));
    EXPECT_EQ(m_requestStatus, EAudioRequestStatus::PartialSuccess);
}

TEST_F(ATLTestFixture, ATLProcessRequest_SimulateInitShutdown_ExpectedResults)
{
    // Don't need to do anything in the callbacks this time,
    // but still need to supply them because it sets the m_requestStatus variable.
    // Use the impl mock to simulate a scucessful init/shutdown pair.
    // Then check the result.
    Audio::SystemRequest::Initialize initialize;
    initialize.m_callback = [](const Audio::SystemRequest::Initialize&)
    {
    };
    Audio::SystemRequest::Shutdown shutdown;
    shutdown.m_callback = [](const Audio::SystemRequest::Shutdown&)
    {
    };

    EXPECT_CALL(m_sys, PushCallback).WillRepeatedly(m_callbackCaller);

    using ::testing::Return;
    using ::testing::_;

    EXPECT_CALL(m_impl, Initialize).WillOnce(Return(EAudioRequestStatus::Success));
    EXPECT_CALL(m_impl, NewGlobalAudioObjectData(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(m_impl, GetImplSubPath).WillOnce(Return("test_subpath"));
    m_atl.ProcessRequest(AZStd::move(initialize));
    EXPECT_EQ(m_requestStatus, EAudioRequestStatus::Success);

    m_requestStatus = EAudioRequestStatus::None;

    EXPECT_CALL(m_impl, ShutDown).WillOnce(Return(EAudioRequestStatus::Success));
    EXPECT_CALL(m_impl, Release).WillOnce(Return(EAudioRequestStatus::Success));
    m_atl.ProcessRequest(AZStd::move(shutdown));
    EXPECT_EQ(m_requestStatus, EAudioRequestStatus::Success);
}

TEST_F(ATLTestFixture, AudioProxy_SimulateQueuedCommands_NumCommandsExecutedMatches)
{
    EXPECT_EQ(m_proxy.GetAudioObjectID(), INVALID_AUDIO_OBJECT_ID);
    constexpr TAudioObjectID objectId{ 2000 };

    // Setup what PushRequest will do when 'Initialize' is called on the proxy...
    EXPECT_CALL(m_sys, PushRequest)
        .WillOnce(
            [this](AudioRequestVariant&& requestVariant)
            {
                AZStd::visit(
                    [](auto&& request)
                    {
                        using T = AZStd::decay_t<decltype(request)>;
                        if constexpr (AZStd::is_same_v<T, Audio::SystemRequest::ReserveObject>)
                        {
                            request.m_objectId = objectId;
                        }
                    },
                    requestVariant);
                // Hold onto the request before executing the callback so we can queue up additional requests.
                m_requestHolder = AZStd::move(requestVariant);
            });

    // 1. Initialize the audio proxy
    m_proxy.Initialize("test_proxy");

    // Confirm the proxy object still doesn't have an ID...
    EXPECT_EQ(m_proxy.GetAudioObjectID(), INVALID_AUDIO_OBJECT_ID);

    constexpr AZ::u32 numCommands = 2;
    AZ::u32 commandCount = 0;

    // Setup what PushRequests will do when additional commands are queued...
    EXPECT_CALL(m_sys, PushRequests)
        .WillOnce(
            [&commandCount](AudioRequestsQueue& queue)
            {
                commandCount += aznumeric_cast<AZ::u32>(queue.size());
            });

    // 2. Call additional commands on the proxy
    m_proxy.SetPosition(AZ::Vector3::CreateOne());
    m_proxy.SetRtpcValue(TAudioControlID{ 123 }, 0.765f);

    // Calling functions on the proxy before it's received an ID
    // shouldn't get pushed to the audio system yet.
    EXPECT_EQ(commandCount, 0);

    // 3. Now execute the initialize callback, which "gives" the ID to the proxy
    // and will also execute the queued commands.
    m_callbackCaller(AZStd::move(m_requestHolder));

    // Check that the proxy has the expected ID and expected number of commands
    // were fake-pushed.
    EXPECT_EQ(m_proxy.GetAudioObjectID(), objectId);
    EXPECT_EQ(commandCount, numCommands);

    // Resets data on the audio proxy object
    EXPECT_CALL(m_sys, PushRequest).WillOnce(::testing::Return());
    m_proxy.Release();
}
