/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/deque.h>

#include <AZTestShared/Utils/Utils.h>

namespace SettingsRegistryOriginTrackerTests {

    class SettingsRegistryOriginTrackerFixture : public UnitTest::AllocatorsFixture
    {
    public:

        void SetUp() override
        {
            SetupAllocator();

            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            m_originTracker = AZStd::make_unique<AZ::SettingsRegistryOriginTracker>(*m_registry);
        }

        void TearDown() override
        {

            m_originTracker.reset();
            m_registry.reset();
            

            TeardownAllocator();
        }

        static bool CreateTestFile(const AZ::IO::FixedMaxPath& testPath, AZStd::string_view content)
        {
            AZ::IO::SystemFile file;
            if (!file.Open(testPath.c_str(), AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE
                | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                AZ_Assert(false, "Unable to open test file for writing: %s", testPath.c_str());
                return false;
            }

            if (file.Write(content.data(), content.size()) != content.size())
            {
                AZ_Assert(false, "Unable to write content to test file: %s", testPath.c_str());
                return false;
            }

            return true;
        }

        void CheckOriginsAtPath(const AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack& expected, AZStd::string_view path = "")
        {
            int counter = 0;
            auto callback = [&](AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin origin)
            {
                auto expectedOrigin = expected.at(counter);
                EXPECT_EQ(expectedOrigin, origin);
                counter += 1;
                return true;
            };
            m_originTracker->VisitOrigins(path, callback);
            EXPECT_EQ(counter, expected.size());
        };

    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::SettingsRegistryOriginTracker> m_originTracker;
        AZ::Test::ScopedAutoTempDirectory m_testFolder;
    };

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFile_TracksOrigin_Successful)
    {
        auto tempRootFolder = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory());
        AZ::IO::FixedMaxPath filePath1 = tempRootFolder / "folder1" / "file1.json";
        AZ::IO::FixedMaxPath filePath2 = tempRootFolder / "folder2" / "file2.json";
        AZ::IO::FixedMaxPath filePath3 = tempRootFolder / "folder3" / "file3.json";
        ASSERT_TRUE(CreateTestFile(filePath1, R"(
            {
                "O3DE": {
                    "Settings": {
                        "ArrayValue": [
                        5,
                        7,
                        40000000,
                        -89396
                    ],
                    "ObjectValue": {
                        "StringKey1": "Hello",
                        "BoolKey1": true
                    }
                    }
                }
            }
        )"));
        ASSERT_TRUE(CreateTestFile(filePath2, R"(
            {
                "O3DE": {
                    "Settings": {
                        "ArrayValue": [
                        27,
                        39,
                        42
                        ],
                        "ObjectValue": {
                        "IntKey2": 9001,
                        },
                        "DoubleValue": 4.0
                    }
                }
            }
        )"));
        ASSERT_TRUE(CreateTestFile(filePath3, R"(
            {
                "O3DE": {
                    "Settings": {
                        "ArrayValue": [
                        -5,
                        39,
                        42
                        ],
                        "ObjectValue": {
                        "StringKey1": "Hi",
                        "BoolKey1": true,
                        "IntKey2": 9001
                        },
                        "DoubleValue": 4.0
                    }
                }
            }
        )"));
        m_registry->MergeSettingsFile(filePath1.FixedMaxPathString(),
            AZ::SettingsRegistryInterface::Format::JsonMergePatch, {},
            nullptr);
        m_registry->MergeSettingsFile(filePath2.FixedMaxPathString(),
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "",
            nullptr);
        m_registry->MergeSettingsFile(filePath3.FixedMaxPathString(),
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "",
            nullptr);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedStringKey1OriginStack = {
            { filePath1.Filename(), "/O3DE/Settings/ObjectValue/StringKey1", "Hi" }
        };
        CheckOriginsAtPath(expectedStringKey1OriginStack, "/O3DE/Settings/ObjectValue/StringKey1");
    }
   
}
