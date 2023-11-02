/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <AZTestShared/Utils/Utils.h>

namespace AZ
{
    // Pretty printers for the SettingsRegistryOrigin struct
    void PrintTo(const AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin& origin, ::std::ostream* os)
    {
        *os << "file path: " << AZ::IO::Path(origin.m_originFilePath.Native(), AZ::IO::PosixPathSeparator).MakePreferred().c_str() << '\n';
        *os << "key path: " << origin.m_settingsKey.c_str() << '\n';
        *os << "value when origin set: " << (origin.m_settingsValue.has_value() ? origin.m_settingsValue->c_str() : "<removed>") << '\n';
    }
} // namespace AZ

namespace SettingsRegistryOriginTrackerTests
{
    const AZStd::string OBJECT_VALUE = "<object>";
    const AZStd::string ARRAY_VALUE = "<array>";

    class SettingsRegistryOriginTrackerFixture : public UnitTest::LeakDetectionFixture
    {
    public:
        SettingsRegistryOriginTrackerFixture()
        {
            if (!AZ::NameDictionary::IsReady())
            {
                AZ::NameDictionary::Create();
                m_createdNameDictionary = true;
            }
        }

        ~SettingsRegistryOriginTrackerFixture()
        {
            if (m_createdNameDictionary)
            {
                AZ::NameDictionary::Destroy();
                m_createdNameDictionary = false;
            }
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

        void CheckOriginsAtPath(
            const AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack& expected, AZStd::string_view path = "")
        {
            AZStd::vector<AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin> originArray;
            auto GetOrigins = [&originArray](AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin origin)
            {
                originArray.push_back(origin);
                return true;
            };
            m_originTracker->VisitOrigins(path, GetOrigins);
            for (const auto& settingsRegistryOrigin : expected)
            {
                EXPECT_THAT(originArray, ::testing::Contains(settingsRegistryOrigin));
            }
            EXPECT_EQ(expected.size(), originArray.size());
        }

    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::SettingsRegistryOriginTracker> m_originTracker;
        AZ::Test::ScopedAutoTempDirectory m_testFolder;
        bool m_createdNameDictionary = false;
    };

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFile_TracksOrigin_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.json";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.json";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
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
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
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
        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        m_registry->MergeSettingsFile(
            (tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_registry->MergeSettingsFile(
            (tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);

        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {
            { tempRootFolder / filePath1, "/O3DE/Settings/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/Settings/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/Settings/ObjectValue/IntKey2", "9001" },
            { tempRootFolder / filePath1, "/O3DE/Settings/ObjectValue/BoolKey1", "true" },
            { tempRootFolder / filePath1, "/O3DE/Settings/ObjectValue/StringKey1", "\"Hello\"" },
        };
        CheckOriginsAtPath(expectedObjectValueOriginStack, "/O3DE/Settings/ObjectValue");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeArraySettings_TracksArrayOriginWithPatch_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.setregpatch";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.setregpatch";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
           [
               {"op": "add", "path": "/O3DE", "value": {}},
               {"op": "add", "path" : "/O3DE/ArrayValue", "value": []},
               {"op": "add", "path" : "/O3DE/ArrayValue/0", "value": 5},
               {"op": "add", "path" : "/O3DE/ArrayValue/1", "value": 7},
               {"op": "add", "path" : "/O3DE/ArrayValue/2", "value": 40000000},
               {"op": "add", "path" : "/O3DE/ArrayValue/3", "value": -89396}
           ]
        )"));
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
           [
               {"op": "replace", "path" : "/O3DE/ArrayValue/0", "value" : 27},
               {"op": "replace", "path" : "/O3DE/ArrayValue/1", "value" : 39},
               {"op": "replace", "path" : "/O3DE/ArrayValue/2", "value" : 42}
           ]
        )"));
        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonPatch);
        m_registry->MergeSettingsFile((tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonPatch);

        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedArrayValueOriginStack = {
            { tempRootFolder / filePath1, "/O3DE/ArrayValue", ARRAY_VALUE },
            { tempRootFolder / filePath1, "/O3DE/ArrayValue/3", "-89396" },
            { tempRootFolder / filePath1, "/O3DE/ArrayValue/2", "40000000" },
            { tempRootFolder / filePath2, "/O3DE/ArrayValue/2", "42" },
            { tempRootFolder / filePath1, "/O3DE/ArrayValue/1", "7" },
            { tempRootFolder / filePath2, "/O3DE/ArrayValue/1", "39" },
            { tempRootFolder / filePath1, "/O3DE/ArrayValue/0", "5" },
            { tempRootFolder / filePath2, "/O3DE/ArrayValue/0", "27" },
        };
        CheckOriginsAtPath(expectedArrayValueOriginStack, "/O3DE/ArrayValue");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFile_RemoveDuplicateOrigin_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.json";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.json";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
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
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
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
        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_registry->MergeSettingsFile((tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {
            { tempRootFolder / filePath1, "/O3DE/Settings/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/Settings/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath1, "/O3DE/Settings/ObjectValue/BoolKey1", "true" },
            { tempRootFolder / filePath1, "/O3DE/Settings/ObjectValue/StringKey1", "\"Hello\"" },
            { tempRootFolder / filePath2, "/O3DE/Settings/ObjectValue/IntKey2", "9001" }
        };
        CheckOriginsAtPath(expectedObjectValueOriginStack, "/O3DE/Settings/ObjectValue");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFileAndCode_TracksOrigin_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.setreg";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.setreg";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
           {
               "O3DE": {
                   "ObjectValue": {
                       "StringKey1": "Hello",
                       "BoolKey1": true
                   }

               }
           }
        )"));
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
           {
               "O3DE": {
                   "ObjectValue": {
                       "StringKey1": "Hi",
                       "IntKey2": 9001
                   }
               }
           }
        )"));

        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        AZStd::string_view stringKey1Path = "/O3DE/ObjectValue/StringKey1";
        AZStd::string_view stringKey1NewValue = R"(Greetings)";
        ASSERT_TRUE(m_registry->Set(stringKey1Path, stringKey1NewValue));
        m_registry->MergeSettingsFile((tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {
            { tempRootFolder / filePath1, "/O3DE/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/ObjectValue/IntKey2", "9001" },
            { tempRootFolder / filePath1, "/O3DE/ObjectValue/BoolKey1", "true" },
            { tempRootFolder / filePath1, "/O3DE/ObjectValue/StringKey1", "\"Hello\"" },
            { "<in-memory>", "/O3DE/ObjectValue/StringKey1", "\"Greetings\"" },
            { tempRootFolder / filePath2, "/O3DE/ObjectValue/StringKey1", "\"Hi\"" }
        };
        CheckOriginsAtPath(expectedObjectValueOriginStack, "/O3DE/ObjectValue");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFiles_NestedMerge_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.setreg";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.setreg";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
            {
                "O3DE": {
                    "foo": 1,
                    "bar": true,
                    "baz": "yes"
                }
            }
        )"));
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
            {
                "O3DE": {
                    "foo": 3,
                    "bar": false,
                    "baz": "no"
                }
            }
        )"));
        bool mergeFile = true;
        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        auto callback = [this, &tempRootFolder, &mergeFile, &filePath2](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            if (notifyEventArgs.m_jsonKeyPath == "/O3DE" && mergeFile)
            {
                mergeFile = false;
                m_registry->MergeSettingsFile((tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
            }
        };
        auto testNotifier = m_registry->RegisterNotifier(callback);
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedO3DEOriginStack = {
            { tempRootFolder / filePath1, "/O3DE", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/foo", "3" },
            { tempRootFolder / filePath1, "/O3DE/foo", "1" },
            { tempRootFolder / filePath1, "/O3DE/bar", "true" },
            { tempRootFolder / filePath2, "/O3DE/bar", "false" },
            { tempRootFolder / filePath1, "/O3DE/baz", R"("yes")" },
            { tempRootFolder / filePath2, "/O3DE/baz", R"("no")" }
        };
        AZStd::vector<AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin> originArray;
        auto GetOrigins = [&originArray](AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin origin)
        {
            originArray.push_back(origin);
            return true;
        };
        m_originTracker->VisitOrigins("/O3DE", GetOrigins);
        for (const auto& settingsRegistryOrigin : expectedO3DEOriginStack)
        {
            EXPECT_THAT(originArray, ::testing::Contains(settingsRegistryOrigin));
        }
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFiles_LargeInput_Successful)
    {
        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        int numIterations = 10;
        for (int i = 0; i < numIterations; i++)
        {
            AZ::IO::FixedMaxPath filePath = AZ::IO::FixedMaxPathString::format("file%d.setreg", i);
            ASSERT_TRUE(AZ::Test::CreateTestFile(
                m_testFolder,
                filePath,
                AZStd::string::format(
                    R"(
                {
                    "O3DE": {
                        "persistentKey": %d,
                        "changingKey%d": true
                    }
                }
            )",
                    i,
                    i)));
            m_registry->MergeSettingsFile((tempRootFolder / filePath).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        }
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedO3DEOriginStack;
        for (int i = 0; i < numIterations; i++)
        {
            AZ::IO::FixedMaxPath filePath = AZ::IO::FixedMaxPathString::format("file%d.setreg", i);
            AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin o3deOrigin{ tempRootFolder / filePath, "/O3DE", OBJECT_VALUE };
            AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin persistentKeyOrigin{ tempRootFolder / filePath,
                                                                                           "/O3DE/persistentKey",
                                                                                           AZStd::string::format("%d", i) };
            AZ::SettingsRegistryOriginTracker::SettingsRegistryOrigin changingKeyOrigin{ tempRootFolder / filePath,
                                                                                         AZStd::string::format("/O3DE/changingKey%d", i),
                                                                                         "true" };
            expectedO3DEOriginStack.emplace_back(o3deOrigin);
            expectedO3DEOriginStack.emplace_back(changingKeyOrigin);
            expectedO3DEOriginStack.emplace_back(persistentKeyOrigin);
        }
        CheckOriginsAtPath(expectedO3DEOriginStack, "/O3DE");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFiles_IncludeTrackingFilter_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.json";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.json";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
           {
               "O3DE": {
                    "ObjectValue": {
                        "StringKey1": "Hello",
                        "BoolKey1": true
                    }
               }
           }
       )"));
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
           {
               "O3DE": {
                    "ObjectValue": {
                        "IntKey2": 9001,
                    },
               }
           }
      )"));
        auto trackingFilterCallback = [](AZStd::string_view keyPath)
        {
            if (keyPath.contains("IntKey2"))
            {
                return false;
            }
            return true;
        };
        m_originTracker->SetTrackingFilter(trackingFilterCallback);
        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        m_registry->MergeSettingsFile((tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        AZ::SettingsRegistryOriginTracker::SettingsRegistryOriginStack expectedObjectValueOriginStack = {
            { tempRootFolder / filePath1, "/O3DE/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath2, "/O3DE/ObjectValue", OBJECT_VALUE },
            { tempRootFolder / filePath1, "/O3DE/ObjectValue/BoolKey1", "true" },
            { tempRootFolder / filePath1, "/O3DE/ObjectValue/StringKey1", "\"Hello\"" }
        };
        CheckOriginsAtPath(expectedObjectValueOriginStack, "/O3DE/ObjectValue");
    }

    TEST_F(SettingsRegistryOriginTrackerFixture, MergeSettingsFiles_FindLastOrigin_Successful)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.setreg";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.setreg";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
            {
                "O3DE": {
                    "foo": 1,
                    "bar": true,
                    "baz": "yes"
                }
            }
        )"));
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
            {
                "O3DE": {
                    "foo": 3,
                    "baz": "no"
                }
            }
        )"));

        auto tempRootFolder = m_testFolder.GetDirectoryAsPath();
        m_registry->MergeSettingsFile((tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        m_registry->MergeSettingsFile((tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        AZ::IO::Path lastFooOriginPath;
        ASSERT_TRUE(m_originTracker->FindLastOrigin(lastFooOriginPath, "/O3DE/foo"));
        EXPECT_EQ(tempRootFolder / filePath2, lastFooOriginPath);
        AZ::IO::Path lastBarOriginPath;
        ASSERT_TRUE(m_originTracker->FindLastOrigin(lastBarOriginPath, "/O3DE/bar"));
        EXPECT_EQ(tempRootFolder / filePath1, lastBarOriginPath);
        AZ::IO::Path lastBazOriginPath;
        ASSERT_TRUE(m_originTracker->FindLastOrigin(lastBazOriginPath, "/O3DE/baz"));
        EXPECT_EQ(tempRootFolder / filePath2, lastBazOriginPath);
    }
} // namespace SettingsRegistryOriginTrackerTests
