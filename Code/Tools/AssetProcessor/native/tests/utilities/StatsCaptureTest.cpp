/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/AssetProcessorTest.h>
#include <native/utilities/StatsCapture.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/StringFunc/StringFunc.h>

// the simple stats capture system has a trivial interface and only writes to printf.
// So the simplest tests we can do is make sure it only asserts when it should
// and doesn't assert in cases when it shouldn't, and that the stats are reasonable
// in printf format.

namespace AssetProcessor
{
// Its okay to talk to this system when unintialized, you can gain some perf
// by not intializing it at all
TEST_F(AssetProcessorTest, StatsCaptureTest_UninitializedSystemDoesNotAssert)
{
    AssetProcessor::StatsCapture::BeginCaptureStat("Test");
    AssetProcessor::StatsCapture::EndCaptureStat("Test");
    AssetProcessor::StatsCapture::Dump();
    AssetProcessor::StatsCapture::Shutdown();
}

// Double-intiailize is an error
TEST_F(AssetProcessorTest, StatsCaptureTest_DoubleInitializeIsAnAssert)
{
    m_errorAbsorber->Clear();
    
    AssetProcessor::StatsCapture::Initialize();
    AssetProcessor::StatsCapture::Initialize();
    
    EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
    EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 1); // not allowed to assert on this

    AssetProcessor::StatsCapture::BeginCaptureStat("Test");
    AssetProcessor::StatsCapture::Shutdown();
}

class StatsCaptureOutputTest : public AssetProcessorTest, public AZ::Debug::TraceMessageBus::Handler
{
public:
    void SetUp() override
    {
        AssetProcessorTest::SetUp();
        AssetProcessor::StatsCapture::Initialize();
    }
    
    // dump but also capture the dump as a vector of lines:
    void Dump()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        AssetProcessor::StatsCapture::Dump();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }
    
    virtual bool OnPrintf(const char* /*window*/, const char* message)
    {
        m_gatheredMessages.emplace_back(message);
        AZ::StringFunc::TrimWhiteSpace(m_gatheredMessages.back(), true, true);
        return false;
    }
    
    void TearDown() override
    {
        m_gatheredMessages = {};
        
        AssetProcessor::StatsCapture::Shutdown();
        AssetProcessorTest::TearDown();
    }
    
    AZStd::vector<AZStd::string> m_gatheredMessages;
};

// turning off machine and human readable mode, should not dump anything.
TEST_F(StatsCaptureOutputTest, StatsCaptureTest_DisabledByRegset_DumpsNothing)
{
    auto registry = AZ::SettingsRegistry::Get();
    ASSERT_NE(registry, nullptr);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/HumanReadable", false);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/MachineReadable", false);
    AssetProcessor::StatsCapture::BeginCaptureStat("Test");
    AssetProcessor::StatsCapture::EndCaptureStat("Test");
    Dump();
    EXPECT_EQ(m_gatheredMessages.size(), 0);
}

// turning on Human Readable, turn off Machine Readable, should not output any machine readable stats.
TEST_F(StatsCaptureOutputTest, StatsCaptureTest_HumanReadableOnly_DumpsNoMachineReadable)
{
    auto registry = AZ::SettingsRegistry::Get();
    ASSERT_NE(registry, nullptr);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/HumanReadable", true);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/MachineReadable", false);
    AssetProcessor::StatsCapture::BeginCaptureStat("Test");
    AssetProcessor::StatsCapture::EndCaptureStat("Test");
    Dump();
    EXPECT_GT(m_gatheredMessages.size(), 0);
    for (const auto& message : m_gatheredMessages)
    {
        // we expect to see ZERO "Machine Readable" lines
        EXPECT_FALSE(message.contains("MachineReadableStat:")) << "Found unexpected line in output: " << message.c_str();
    }
}

// Turn on Machine Readable, Turn off Human Readable, ensure only Machine Readable stats emitted.
TEST_F(StatsCaptureOutputTest, StatsCaptureTest_MachineReadableOnly_DumpsNoHumanReadable)
{
    auto registry = AZ::SettingsRegistry::Get();
    ASSERT_NE(registry, nullptr);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/HumanReadable", false);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/MachineReadable", true);
    AssetProcessor::StatsCapture::BeginCaptureStat("Test");
    AssetProcessor::StatsCapture::EndCaptureStat("Test");
    Dump();
    for (const auto& message : m_gatheredMessages)
    {
        // we expect to see ONLY "Machine Readable" lines
        EXPECT_TRUE(message.contains("MachineReadableStat:")) << "Found unexpected line in output: " << message.c_str();
    }
    EXPECT_GT(m_gatheredMessages.size(), 0);
}


// The interface for StatsCapture just captures and then dumps.  
// For us to test this, we thus have to capture and parse the dump output.
TEST_F(StatsCaptureOutputTest, StatsCaptureTest_Sanity)
{
    auto registry = AZ::SettingsRegistry::Get();
    ASSERT_NE(registry, nullptr);
    
    // Make it output in "machine raadable" format so that it is simpler to parse.
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/HumanReadable", false);
    registry->Set("/Amazon/AssetProcessor/Settings/Stats/MachineReadable", true);
    AssetProcessor::StatsCapture::BeginCaptureStat("CreateJobs,foo,mybuilder");
    AssetProcessor::StatsCapture::EndCaptureStat("CreateJobs,foo,mybuilder");
    
    // Intentionally not using sleeps in this test.  It means that the 
    // captured duration will be likely 0 but its not worth it to slow down tests.
    // If the durations end up 0 its going to be extremely noticable in day-to-day use.
    AssetProcessor::StatsCapture::BeginCaptureStat("CreateJobs,foo,mybuilder");
    AssetProcessor::StatsCapture::EndCaptureStat("CreateJobs,foo,mybuilder");
    
    // for the second stat, we'll double capture and double end, in order to test debounce
    AssetProcessor::StatsCapture::BeginCaptureStat("CreateJobs,foo2,mybuilder");
    AssetProcessor::StatsCapture::BeginCaptureStat("CreateJobs,foo2,mybuilder");
    AssetProcessor::StatsCapture::EndCaptureStat("CreateJobs,foo2,mybuilder2");
    AssetProcessor::StatsCapture::EndCaptureStat("CreateJobs,foo2,mybuilder2");
    
    m_gatheredMessages.clear();
    Dump();
    EXPECT_GT(m_gatheredMessages.size(), 0);

    // We'll parse the machine readable stat lines here and make sure that the following is true
    // mybuilder appears
    // mybuilder appears only once but count is 2
    bool foundFoo = false;
    bool foundFoo2 = false;
    
    for (const auto& stat : m_gatheredMessages)
    {
        if (stat.contains("MachineReadableStat:"))
        {
            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(stat, tokens, ":", false, false);
            ASSERT_EQ(tokens.size(), 5); // should be "MachineReadableStat:time:count:average:name)
            const auto& countData = tokens[2];
            const auto& nameData = tokens[4];
            
            if (AZ::StringFunc::Equal(nameData, "CreateJobs,foo,mybuilder"))
            {
                EXPECT_FALSE(foundFoo); // should only find one of these
                foundFoo = true;
                EXPECT_STREQ(countData.c_str(), "2");
            }
            
            if (AZ::StringFunc::Equal(nameData, "CreateJobs,foo2,mybuilder2"))
            {
                EXPECT_FALSE(foundFoo2); // should only find one of these
                foundFoo2 = true;
                EXPECT_STREQ(countData.c_str(), "1");
            }
        }
    }
    
    EXPECT_TRUE(foundFoo) << "The expected token CreateJobs,foo,mybuilder did not appear in the output.";
    EXPECT_TRUE(foundFoo2) << "The expected CreateJobs.foo2.mybuilder2 did not appear in the output";
}

}

