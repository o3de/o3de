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

#include <AzCore/Name/NameDictionary.h>
#include <AzCore/std/string/regex.h>

#include <AZTestShared/Utils/Utils.h>

namespace SettingsRegistryOriginTrackerTests {

    class SettingsRegistryOriginTrackerFixture : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        SettingsRegistryOriginTrackerFixture()
            : ScopedAllocatorSetupFixture(
                  []()
                  {
                      AZ::SystemAllocator::Descriptor desc;
                      desc.m_stackRecordLevels = 30;
                      return desc;
                  }())
        {
            AZ::NameDictionary::Create();
        };

        ~SettingsRegistryOriginTrackerFixture()
        {
            AZ::NameDictionary::Destroy();
        }

        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            m_originTracker = AZStd::make_unique<AZ::SettingsRegistryOriginTracker>(*m_registry);
        }

        void TearDown() override
        {
            m_originTracker.reset();
            m_registry.reset();
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
        AZStd::regex r("\\s+");
        auto tempRootFolder = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory());
        AZ::IO::FixedMaxPath filePath1 = tempRootFolder / "folder1" / "file1.json";
        AZ::IO::FixedMaxPath filePath2 = tempRootFolder / "folder2" / "file2.json";
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
        m_registry->MergeSettingsFile(filePath1.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "",
            nullptr);
        m_registry->MergeSettingsFile(filePath2.FixedMaxPathString(),
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "",
            nullptr);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {
            { filePath1.String(), "/O3DE/Settings/ObjectValue", AZStd::regex_replace(R"(
                {
                    "StringKey1": "Hello",
                    "BoolKey1": true
                }
            )", r,"")
            },
            { filePath2.String(), "/O3DE/Settings/ObjectValue/IntKey2", "9001" },
            { filePath1.String(), "/O3DE/Settings/ObjectValue/BoolKey1", "true" },
            { filePath1.String(), "/O3DE/Settings/ObjectValue/StringKey1", "\"Hello\"" },
        };
        CheckOriginsAtPath(expectedObjectValueOriginStack, "/O3DE/Settings/ObjectValue");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFile_TracksArrayOriginWithPatch_Successful)
    {
        AZStd::regex r("\\s+");
        auto tempRootFolder = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory());
        AZ::IO::FixedMaxPath filePath1 = tempRootFolder / "folder1" / "file1.json";
        AZ::IO::FixedMaxPath filePath2 = tempRootFolder / "folder2" / "file2.json";
        ASSERT_TRUE(CreateTestFile(filePath1, R"(
            {
                "O3DE": {
                    "ArrayValue": [
                        5,
                        7,
                        40000000,
                        -89396
                    ],
                }
            }
        )"));
        ASSERT_TRUE(CreateTestFile(filePath2, R"(
            {
                "O3DE": {
                    "ArrayValue": [
                        27,
                        39,
                        42
                    ],
                    "DoubleValue": 4.0
                }
            }
        )"));
        m_registry->MergeSettingsFile(filePath1.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonPatch, "", nullptr);
        m_registry->MergeSettingsFile(filePath2.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonPatch, "", nullptr);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {
            {
                filePath1.String(),
                "/O3DE/ArrayValue",
                AZStd::regex_replace(
                   R"(
                        {
                            [5, 7, 40000000, -89396]
                        }
                    )", r, "")
            },
            {
                filePath2.String(),
                "/O3DE/ArrayValue",
                AZStd::regex_replace(
                   R"(
                        {
                            [27, 39, 42]
                        }
                    )", r, "")
            },
            { filePath2.String(), "/O3DE/ArrayValue/0", "27" },
            { filePath1.String(), "/O3DE/ArrayValue/0", "5" },
            { filePath2.String(), "/O3DE/ArrayValue/1", "39" },
            { filePath1.String(), "/O3DE/ArrayValue/1", "7" },
            { filePath2.String(), "/O3DE/ArrayValue/2", "42" },
            { filePath1.String(), "/O3DE/ArrayValue/2", "40000000" },
            { filePath1.String(), "/O3DE/ArrayValue/3", "-89396" },
        };
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFile_RemoveDuplicateOrigin_Successful)
    {
        AZStd::regex r("\\s+");
        auto tempRootFolder = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory());
        AZ::IO::FixedMaxPath filePath1 = tempRootFolder / "folder1" / "file1.json";
        AZ::IO::FixedMaxPath filePath2 = tempRootFolder / "folder2" / "file2.json";
        ASSERT_TRUE(CreateTestFile(filePath1, R"(
            {
                "O3DE": {
                    "Settings": {
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
                        "ObjectValue": {
                            "IntKey2": 9001
                         },
                    }
                }
            }
        )"));
        m_registry->MergeSettingsFile(filePath1.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_registry->MergeSettingsFile(filePath2.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_registry->MergeSettingsFile(filePath1.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {

            { filePath1.String(),
              "/O3DE/Settings/ObjectValue",
              AZStd::regex_replace(
                  R"(
                {
                    "StringKey1": "Hello",
                    "BoolKey1": true,
                    "IntKey2": 9001
                }
            )",
                  r,
                  "") },
            { filePath1.String(), "/O3DE/Settings/ObjectValue/BoolKey1", "true" },
            { filePath1.String(), "/O3DE/Settings/ObjectValue/StringKey1", "\"Hello\"" },
            { filePath2.String(), "/O3DE/Settings/ObjectValue/IntKey2", "9001" },
            
        };
        CheckOriginsAtPath(expectedObjectValueOriginStack, "/O3DE/Settings/ObjectValue");
    }
   
}
