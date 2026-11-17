/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    class PrefabTemplateFileHandlerTest : public PrefabTestFixture
    {
    protected:
        // Stands in for an alternate format: JSON behind a marker prefix, so a test can tell whether the
        // registered handler ran or the file fell through to built-in JSON parsing.
        static constexpr AZStd::string_view AlternateExtension = ".testprefab";
        static constexpr AZStd::string_view AlternateMarker = "ALTERNATE:";

        bool RegisterAlternateFormat(AZStd::string_view extension = AlternateExtension)
        {
            return m_prefabLoaderInterface->RegisterTemplateFileHandler(
                extension,
                [this](AZStd::string_view fileContent, AZ::IO::PathView) -> AZ::Outcome<PrefabDom, AZStd::string>
                {
                    ++m_loadHandlerCallCount;
                    if (!fileContent.starts_with(AlternateMarker))
                    {
                        return AZ::Failure(AZStd::string("Missing the alternate format marker."));
                    }
                    return AZ::JsonSerializationUtils::ReadJsonString(fileContent.substr(AlternateMarker.size()));
                },
                [this](const PrefabDom&, AZ::IO::PathView absolutePath) -> AZ::Outcome<void, AZStd::string>
                {
                    m_savedPath = absolutePath;
                    return AZ::Success();
                });
        }

        static AZStd::string EmptyPrefabJson()
        {
            return PrefabTestDomUtils::DomToString(PrefabTestDomUtils::CreatePrefabDom());
        }

        static AZStd::string EmptyPrefabInAlternateFormat()
        {
            return AZStd::string::format("%.*s%s", AZ_STRING_ARG(AlternateMarker), EmptyPrefabJson().c_str());
        }

        TemplateId Load(AZStd::string_view content, AZ::IO::PathView path)
        {
            return m_prefabLoaderInterface->LoadTemplateFromString(content, path);
        }

        int m_loadHandlerCallCount = 0;
        AZ::IO::Path m_savedPath;
    };

    TEST_F(PrefabTemplateFileHandlerTest, LoadTemplateFromString_MatchingExtensionAnyCase_UsesRegisteredLoadHandler)
    {
        ASSERT_TRUE(RegisterAlternateFormat());

        EXPECT_NE(Load(EmptyPrefabInAlternateFormat(), "path/to/lower.testprefab"), AzToolsFramework::Prefab::InvalidTemplateId);
        EXPECT_NE(Load(EmptyPrefabInAlternateFormat(), "path/to/mixed.TestPreFab"), AzToolsFramework::Prefab::InvalidTemplateId);
        EXPECT_EQ(m_loadHandlerCallCount, 2);
    }

    TEST_F(PrefabTemplateFileHandlerTest, LoadTemplateFromString_ExtensionOnlyPartOfName_FallsBackToJson)
    {
        ASSERT_TRUE(RegisterAlternateFormat());

        EXPECT_NE(Load(EmptyPrefabJson(), "path/to/prefab.testprefab.data.prefab"), AzToolsFramework::Prefab::InvalidTemplateId);
        EXPECT_EQ(m_loadHandlerCallCount, 0);
    }

    TEST_F(PrefabTemplateFileHandlerTest, LoadTemplateFromString_HandlerFails_InvalidTemplateIdReturned)
    {
        ASSERT_TRUE(RegisterAlternateFormat());

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(Load(EmptyPrefabJson(), "path/to/prefab.testprefab"), AzToolsFramework::Prefab::InvalidTemplateId);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(m_loadHandlerCallCount, 1);
    }

    TEST_F(PrefabTemplateFileHandlerTest, SaveTemplate_MatchingExtension_UsesRegisteredSaveHandler)
    {
        ASSERT_TRUE(RegisterAlternateFormat());
        const TemplateId templateId = Load(EmptyPrefabInAlternateFormat(), "path/to/prefab.testprefab");
        ASSERT_NE(templateId, AzToolsFramework::Prefab::InvalidTemplateId);

        EXPECT_TRUE(m_prefabLoaderInterface->SaveTemplate(templateId));
        EXPECT_EQ(m_savedPath.Filename(), AZ::IO::PathView("prefab.testprefab"));
    }

    TEST_F(PrefabTemplateFileHandlerTest, UnregisterTemplateFileHandler_AnyCase_FormatStopsBeingUsed)
    {
        ASSERT_TRUE(RegisterAlternateFormat());

        EXPECT_TRUE(m_prefabLoaderInterface->UnregisterTemplateFileHandler(".TESTPREFAB"));
        EXPECT_FALSE(m_prefabLoaderInterface->UnregisterTemplateFileHandler(AlternateExtension));

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(Load(EmptyPrefabInAlternateFormat(), "path/to/prefab.testprefab"), AzToolsFramework::Prefab::InvalidTemplateId);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(m_loadHandlerCallCount, 0);
    }

    TEST_F(PrefabTemplateFileHandlerTest, RegisterTemplateFileHandler_InvalidArguments_Rejected)
    {
        auto loadHandler = [](AZStd::string_view, AZ::IO::PathView) -> AZ::Outcome<PrefabDom, AZStd::string>
        {
            return AZ::Failure(AZStd::string());
        };
        auto saveHandler = [](const PrefabDom&, AZ::IO::PathView) -> AZ::Outcome<void, AZStd::string>
        {
            return AZ::Success();
        };

        AZ_TEST_START_TRACE_SUPPRESSION;
        for (AZStd::string_view extension : { "", ".", "prefab.test", "./test", ".prefab.test", ".prefab", ".PREFAB" })
        {
            EXPECT_FALSE(RegisterAlternateFormat(extension)) << extension.data();
        }
        EXPECT_FALSE(m_prefabLoaderInterface->RegisterTemplateFileHandler(AlternateExtension, {}, saveHandler));
        EXPECT_FALSE(m_prefabLoaderInterface->RegisterTemplateFileHandler(AlternateExtension, loadHandler, {}));
        AZ_TEST_STOP_TRACE_SUPPRESSION(9);

        EXPECT_TRUE(m_prefabLoaderInterface->GetRegisteredTemplateFileExtensions().empty());
    }

    TEST_F(PrefabTemplateFileHandlerTest, GetRegisteredTemplateFileExtensions_ReturnsLowerCasedExtensions)
    {
        ASSERT_TRUE(RegisterAlternateFormat(".ZZZ"));
        ASSERT_TRUE(RegisterAlternateFormat(".Aaa"));

        AZStd::vector<AZStd::string> extensions = m_prefabLoaderInterface->GetRegisteredTemplateFileExtensions();
        AZStd::sort(extensions.begin(), extensions.end());
        EXPECT_EQ(extensions, AZStd::vector<AZStd::string>({ ".aaa", ".zzz" }));
    }
} // namespace UnitTest
