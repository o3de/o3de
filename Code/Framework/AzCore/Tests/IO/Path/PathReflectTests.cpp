/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/IO/Path/PathReflect.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace UnitTest
{
    template <typename ParamType>
    class PathSerializationParamFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<ParamType>
    {
    public:
        // We must expose the class for serialization first.
        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            AZ::IO::PathReflect(m_serializeContext.get());

        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            AZ::IO::PathReflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    struct PathSerializationParams
    {
        const char m_preferredSeparator{};
        const char* m_testPath{};
    };
    using PathSerializationFixture = PathSerializationParamFixture<PathSerializationParams>;

    TEST_P(PathSerializationFixture, PathSerializer_SerializesStringBackedPath_Succeeds)
    {
        const auto& testParams = GetParam();
        {
            // Path serialization
            AZ::IO::Path testPath{ testParams.m_testPath, testParams.m_preferredSeparator };

            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&testPath);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZ::IO::Path loadPath{ testParams.m_preferredSeparator };
            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadPath, m_serializeContext.get()));
            EXPECT_EQ(testPath.LexicallyNormal(), loadPath);
        }

        {
            // FixedMaxPath serialization
            AZ::IO::FixedMaxPath testFixedMaxPath{ testParams.m_testPath, testParams.m_preferredSeparator };

            AZStd::vector<char> byteBuffer;
            AZ::IO::ByteContainerStream byteStream(&byteBuffer);
            auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(&testFixedMaxPath);
            objStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZ::IO::FixedMaxPath loadPath{ testParams.m_preferredSeparator };
            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadPath, m_serializeContext.get()));
            EXPECT_EQ(testFixedMaxPath.LexicallyNormal(), loadPath);
        }
    }

    INSTANTIATE_TEST_CASE_P(
        PathSerialization,
        PathSerializationFixture,
        ::testing::Values(
            PathSerializationParams{ AZ::IO::PosixPathSeparator, "" },
            PathSerializationParams{ AZ::IO::PosixPathSeparator, "test" },
            PathSerializationParams{ AZ::IO::PosixPathSeparator, "/test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "/test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "D:test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "D:/test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "test/foo/../bar" }
        )
    );

    template <typename ParamType>
    class PathBehaviorContextParamFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<ParamType>
    {
    public:
        void SetUp() override
        {
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            AZ::IO::PathReflect(m_behaviorContext.get());

        }

        void TearDown() override
        {
            m_behaviorContext.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    };

    using PathScriptingParamFixture = PathBehaviorContextParamFixture<PathSerializationParams>;

    TEST_P(PathScriptingParamFixture, PathViewClass_IsAccessibleInScripting_Succeeds)
    {
        auto testParams = GetParam();
        const AZ::BehaviorClass* pathClass = m_behaviorContext->FindClassByReflectedName("PurePathView");
        ASSERT_NE(nullptr, pathClass);

        AZStd::fixed_vector<AZ::BehaviorArgument, 8> behaviorArguments;
        behaviorArguments.emplace_back(&testParams.m_testPath);
        behaviorArguments.emplace_back(&testParams.m_preferredSeparator);
        auto scopedPathViewObject = pathClass->CreateWithScope(behaviorArguments);
        ASSERT_TRUE(scopedPathViewObject.IsValid());
        auto pathViewPtr = scopedPathViewObject.m_behaviorObject.m_rttiHelper->Cast<AZ::IO::PathView>(
            scopedPathViewObject.m_behaviorObject.m_address);
        ASSERT_NE(nullptr, pathViewPtr) << "Cast to AZ::IO::PathView has failed";

        // Get the PathView string value
        auto strMethod = pathClass->FindMethodByReflectedName("__str__");
        ASSERT_NE(nullptr, strMethod);

        AZ::IO::Path result(testParams.m_preferredSeparator);
        ASSERT_TRUE(strMethod->InvokeResult(result.Native(), pathViewPtr));
        AZ::IO::PathView expectedPath(testParams.m_testPath, testParams.m_preferredSeparator);
        EXPECT_EQ(expectedPath, result);
    }

    TEST_P(PathScriptingParamFixture, PathClass_IsAccessibleInScripting_Succeeds)
    {
        {
            auto testParams = GetParam();
            const AZ::BehaviorClass* pathClass = m_behaviorContext->FindClassByReflectedName("PurePath");
            ASSERT_NE(nullptr, pathClass);

            AZStd::fixed_vector<AZ::BehaviorArgument, 8> behaviorArguments;

            // Try with PathView
            AZ::IO::PathView pathView(testParams.m_testPath, testParams.m_preferredSeparator);
            behaviorArguments.emplace_back(&pathView);
            auto scopedPathObject = pathClass->CreateWithScope(behaviorArguments);
            ASSERT_TRUE(scopedPathObject.IsValid());
            auto pathPtr = scopedPathObject.m_behaviorObject.m_rttiHelper->Cast<AZ::IO::Path>(
                scopedPathObject.m_behaviorObject.m_address);
            ASSERT_NE(nullptr, pathPtr) << "Cast to AZ::IO::Path has failed";

            // Get the PathView string value
            auto strMethod = pathClass->FindMethodByReflectedName("__str__");
            ASSERT_NE(nullptr, strMethod);

            AZ::IO::Path result(testParams.m_preferredSeparator);
            ASSERT_TRUE(strMethod->InvokeResult(result.Native(), pathPtr));
            EXPECT_EQ(pathView, result);

            // Convert back to Path View
            auto toPathViewMethod = pathClass->FindMethodByReflectedName("ToPurePathView");
            ASSERT_NE(nullptr, toPathViewMethod);
            AZ::IO::PathView pathViewResult(testParams.m_preferredSeparator);
            ASSERT_TRUE(toPathViewMethod->InvokeResult(pathViewResult, pathPtr));
            EXPECT_EQ(pathViewResult, *pathPtr);
        }
        {
            auto testParams = GetParam();
            const AZ::BehaviorClass* pathClass = m_behaviorContext->FindClassByReflectedName("PureFixedMaxPath");
            ASSERT_NE(nullptr, pathClass);

            // Try with StringView and PathSeparator
            AZStd::string_view strView(testParams.m_testPath);
            AZStd::fixed_vector<AZ::BehaviorArgument, 8> behaviorArguments;
            behaviorArguments.emplace_back(&strView);
            behaviorArguments.emplace_back(&testParams.m_preferredSeparator);
            auto scopedPathObject = pathClass->CreateWithScope(behaviorArguments);
            ASSERT_TRUE(scopedPathObject.IsValid());
            auto pathPtr = scopedPathObject.m_behaviorObject.m_rttiHelper->Cast<AZ::IO::FixedMaxPath>(
                scopedPathObject.m_behaviorObject.m_address);
            ASSERT_NE(nullptr, pathPtr) << "Cast to AZ::IO::FixedMaxPath has failed";

            // Get the PathView string value
            auto strMethod = pathClass->FindMethodByReflectedName("__str__");
            ASSERT_NE(nullptr, strMethod);

            AZ::IO::Path result(testParams.m_preferredSeparator);
            ASSERT_TRUE(strMethod->InvokeResult(result.Native(), pathPtr));
            AZ::IO::PathView pathView(testParams.m_testPath, testParams.m_preferredSeparator);
            EXPECT_EQ(pathView, result);

            // Convert back to Path View
            auto toPathViewMethod = pathClass->FindMethodByReflectedName("ToPurePathView");
            ASSERT_NE(nullptr, toPathViewMethod);
            AZ::IO::PathView pathViewResult(testParams.m_preferredSeparator);
            ASSERT_TRUE(toPathViewMethod->InvokeResult(pathViewResult, pathPtr));
            EXPECT_EQ(pathViewResult, *pathPtr);
        }
    }

    INSTANTIATE_TEST_CASE_P(
        PathScripting,
        PathScriptingParamFixture,
        ::testing::Values(
            PathSerializationParams{ AZ::IO::PosixPathSeparator, "" },
            PathSerializationParams{ AZ::IO::PosixPathSeparator, "test" },
            PathSerializationParams{ AZ::IO::PosixPathSeparator, "/test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "/test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "D:test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "D:/test" },
            PathSerializationParams{ AZ::IO::WindowsPathSeparator, "test/foo/../bar" }
        )
    );

    TEST_F(PathScriptingParamFixture, QueryGetters_Succeed)
    {
        constexpr AZ::IO::PathView testPathView(R"(C:\Windows\Very\Long\Path\filename.with.many.suffixes)", AZ::IO::WindowsPathSeparator);
        const AZ::BehaviorClass* pathClass = m_behaviorContext->FindClassByReflectedName("PurePathView");
        ASSERT_NE(nullptr, pathClass);
        {
            auto method = pathClass->FindGetterByReflectedName("parts");
            ASSERT_NE(nullptr, method);

            AZStd::vector<AZ::IO::PathView> result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_THAT(result, ::testing::ElementsAre(AZ::IO::PathView(R"(C:\)", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView("Windows", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView("Very", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView("Long", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView("Path", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView("filename.with.many.suffixes", AZ::IO::WindowsPathSeparator)
                ));
        }
        {
            auto method = pathClass->FindGetterByReflectedName("drive");
            ASSERT_NE(nullptr, method);

            AZStd::string result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ("C:", result);
        }
        {
            auto method = pathClass->FindGetterByReflectedName("root");
            ASSERT_NE(nullptr, method);

            AZStd::string result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ(R"(\)", result);
        }
        {
            auto method = pathClass->FindGetterByReflectedName("anchor");
            ASSERT_NE(nullptr, method);

            AZStd::string result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ(R"(C:\)", result);
        }
        {
            auto method = pathClass->FindGetterByReflectedName("parents");
            ASSERT_NE(nullptr, method);

            AZStd::vector<AZ::IO::PathView> result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_THAT(result, ::testing::ElementsAre(
                AZ::IO::PathView(R"(C:\Windows\Very\Long\Path)", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView(R"(C:\Windows\Very\Long)", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView(R"(C:\Windows\Very)", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView(R"(C:\Windows)", AZ::IO::WindowsPathSeparator),
                AZ::IO::PathView(R"(C:\)", AZ::IO::WindowsPathSeparator)));
        }
        {
            auto method = pathClass->FindGetterByReflectedName("parent");
            ASSERT_NE(nullptr, method);

            AZ::IO::PathView result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ(AZ::IO::PathView(R"(C:\Windows\Very\Long\Path)", AZ::IO::WindowsPathSeparator), result);
        }
        {
            auto method = pathClass->FindGetterByReflectedName("name");
            ASSERT_NE(nullptr, method);

            AZStd::string result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ("filename.with.many.suffixes", result);
        }
        {
            auto method = pathClass->FindGetterByReflectedName("suffix");
            ASSERT_NE(nullptr, method);

            AZStd::string result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ(".suffixes", result);
        }
        {
            auto method = pathClass->FindGetterByReflectedName("suffixes");
            ASSERT_NE(nullptr, method);

            AZStd::vector<AZStd::string> result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_THAT(result, ::testing::ElementsAre(".with", ".many", ".suffixes"));
        }
        {
            auto method = pathClass->FindGetterByReflectedName("stem");
            ASSERT_NE(nullptr, method);

            AZStd::string result;
            ASSERT_TRUE(method->InvokeResult(result, &testPathView));
            EXPECT_EQ("filename.with.many", result);
        }
    }

}

