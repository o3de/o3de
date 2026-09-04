/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <EnvelopeInfo.h>

#include <AzCore/IO/Path/Path.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <fstream>

namespace
{
    //! A Sentry envelope is newline-delimited JSON: an envelope header, then one
    //! {item header}\n{payload} pair per item. `length` is optional in the item header - when it is
    //! omitted the payload runs to the next newline - so this fixture stays readable without
    //! hand-computing byte counts. Parsing is done by sentry-native itself, so writing a real
    //! envelope here is what gives the test its value.
    AZ::IO::Path WriteEnvelope(const AZ::Test::ScopedAutoTempDirectory& tempDir, const char* eventJson)
    {
        AZ::IO::Path path = tempDir.Resolve("sample.envelope");
        std::ofstream file(path.c_str(), std::ios::binary);
        file << R"({"event_id":"9ec79c33ec9942ab8353589fcb2e04dc"})" << "\n";
        file << R"({"type":"event"})" << "\n";
        file << eventJson << "\n";
        file.close();
        return path;
    }
}

TEST(EnvelopeInfoTests, ParseEnvelope_ExtractsEventMetadata)
{
    AZ::Test::ScopedAutoTempDirectory tempDir;
    AZ::IO::Path envelope = WriteEnvelope(
        tempDir,
        R"({"event_id":"9ec79c33ec9942ab8353589fcb2e04dc",)"
        R"("release":"o3de-editor@1.0.0","environment":"development",)"
        R"("tags":{"project_path":"C:/projects/Blank_Testing","module":"Editor"},)"
        R"("contexts":{"os":{"name":"Windows","version":"10.0.26200"}},)"
        R"("exception":{"values":[{"type":"EXCEPTION_ACCESS_VIOLATION"}]}})");

    CrashHandler::EnvelopeInfo info = CrashHandler::ParseEnvelope(envelope.c_str());

    EXPECT_STREQ(info.eventId.c_str(), "9ec79c33ec9942ab8353589fcb2e04dc");
    EXPECT_STREQ(info.projectPath.c_str(), "C:/projects/Blank_Testing");
    EXPECT_STREQ(info.release.c_str(), "o3de-editor@1.0.0");
    EXPECT_STREQ(info.environment.c_str(), "development");
    EXPECT_STREQ(info.osName.c_str(), "Windows");
    EXPECT_STREQ(info.osVersion.c_str(), "10.0.26200");
    EXPECT_STREQ(info.exceptionType.c_str(), "EXCEPTION_ACCESS_VIOLATION");
}

TEST(EnvelopeInfoTests, ParseEnvelope_ToleratesMissingOptionalFields)
{
    // A minimal event must not fault the reporter or invent values - the dialog just shows blanks.
    AZ::Test::ScopedAutoTempDirectory tempDir;
    AZ::IO::Path envelope =
        WriteEnvelope(tempDir, R"({"event_id":"9ec79c33ec9942ab8353589fcb2e04dc"})");

    CrashHandler::EnvelopeInfo info = CrashHandler::ParseEnvelope(envelope.c_str());

    EXPECT_STREQ(info.eventId.c_str(), "9ec79c33ec9942ab8353589fcb2e04dc");
    EXPECT_TRUE(info.projectPath.empty());
    EXPECT_TRUE(info.osName.empty());
    EXPECT_TRUE(info.exceptionType.empty());
}

TEST(EnvelopeInfoTests, ParseEnvelope_MissingFile_ReturnsEmpty)
{
    CrashHandler::EnvelopeInfo info = CrashHandler::ParseEnvelope("/no/such/file.envelope");

    EXPECT_TRUE(info.eventId.empty());
    EXPECT_TRUE(info.projectPath.empty());
}
