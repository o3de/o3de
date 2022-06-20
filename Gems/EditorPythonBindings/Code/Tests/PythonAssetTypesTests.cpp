/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonMarshalComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace UnitTest
{
    class FooMockSimpleAsset
    {
    public:
        AZ_TYPE_INFO(FooMockSimpleAsset, "{0298F78A-77EF-47CE-9912-B0BC80060016}");

        static const char* GetFileFilter()
        {
            return "foo";
        }
    };
}

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/struts

    struct MockBinding final
    {
        AZ_TYPE_INFO(MockBinding, "{0B22887C-6377-4573-8FE5-418947640D3F}");

        AZ::Data::AssetId m_mockAssetId;

        MockBinding() = default;

        MockBinding(const AZ::Data::AssetId& value)
        {
            m_mockAssetId = value;
        }

        const AZ::Data::AssetId& GetAssetId() const
        {
            return m_mockAssetId;
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            auto&& behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
            if (behaviorContext)
            {
                behaviorContext->Class<MockBinding>("MockBinding")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "mock")
                    ->Constructor()
                    ->Constructor<const AZ::Data::AssetId&>()
                    ->Method("GetAssetId", &MockBinding::GetAssetId)
                    ;
            }
        }

    };

    class MockAsset
        : public AzFramework::SimpleAssetReferenceBase
    {
    public:
        AZ_RTTI(MockAsset, "{C783597C-568F-4B94-911C-506CBD161E10}", AzFramework::SimpleAssetReferenceBase);

        MockAsset()
        {
            SetAssetPath("a/fake/path.foo");
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            auto&& serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<MockAsset, AzFramework::SimpleAssetReferenceBase>();
                AzFramework::SimpleAssetReference<FooMockSimpleAsset>::Register(*serializeContext);
            }

            auto&& behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
            if (behaviorContext)
            {
                behaviorContext->Class<MockAsset>("MockAsset")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ;
            }
        }

        AZ::Data::AssetType GetAssetType() const override
        {
            // Use an arbitrary ID for the asset type.
            return AZ::Data::AssetType("{7FD86523-3903-4037-BCD1-542027BFC553}");
        }

        const char* GetFileFilter() const override
        {
            return nullptr;
        }
    };

    struct MockAssetData
        : public AZ::Data::AssetData
    {
        void SetUseCount(AZ::s32 value)
        {
            m_useCount = value;
        }

        void SetAssetId(AZ::Data::AssetId value)
        {
            m_assetId = value;
        }
    };

    struct MyTestAssetData
        : public AZ::Data::AssetData
    {
        AZ_RTTI(MyTestAssetData, "{B78C6629-95F4-4211-AE7F-4DE58C0D3C33}", AZ::Data::AssetData);
        AZ::u64 m_number = 0;

        void SetUseCount(AZ::s32 value)
        {
            m_useCount = value;
        }
    };

    class ClassWithAssets
    {
    public:
        AZ_RTTI(ClassWithAssets, "{06E4DC78-DD42-44A8-83A1-5B333B557DE9}");
        virtual ~ClassWithAssets() = default;

        static void Reflect(AZ::ReflectContext* reflection)
        {
            auto&& serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<ClassWithAssets>()
                    ->Field("assetId", &ClassWithAssets::m_assetId)
                    ->Field("assetData", &ClassWithAssets::m_assetData)
                    ->Field("mockAsset", &ClassWithAssets::m_mockAsset)
                    ->Field("simpleAssetReference", &ClassWithAssets::m_simpleAssetReference)
                    ;
            }

            auto&& behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
            if (behaviorContext)
            {
                behaviorContext->Class<ClassWithAssets>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Property("assetId", BehaviorValueProperty(&ClassWithAssets::m_assetId))
                    ->Property("assetData", BehaviorValueProperty(&ClassWithAssets::m_assetData))
                    ->Property("mockAsset", BehaviorValueProperty(&ClassWithAssets::m_mockAsset))
                    ->Property("simpleAssetReference", BehaviorValueProperty(&ClassWithAssets::m_simpleAssetReference))
                    ->Method("createFooMockSimpleAsset",&ClassWithAssets::CreateFooMockSimpleAsset)
                    ->Method("printFooMockSimpleAsset", &ClassWithAssets::PrintFooMockSimpleAsset)
                    ;
            }
        }

        AzFramework::SimpleAssetReference<FooMockSimpleAsset> CreateFooMockSimpleAsset(AZStd::string_view assetPath)
        {
            AZ_TracePrintf("python", "SimpleAssetReference creating asset for path %.*s", static_cast<int>(assetPath.size()), assetPath.data());
            AzFramework::SimpleAssetReference<FooMockSimpleAsset> fooMockSimpleAsset;
            fooMockSimpleAsset.SetAssetPath(assetPath.data());
            return fooMockSimpleAsset;
        }

        void PrintFooMockSimpleAsset([[maybe_unused]] AzFramework::SimpleAssetReference<FooMockSimpleAsset>& fooMockSimpleAsset)
        {
            AZ_TracePrintf("python", "SimpleAssetReference asset path is (%s) \n", fooMockSimpleAsset.GetAssetPath().c_str());
        }

        AZ::Data::AssetId m_assetId = AZ::Data::AssetId(AZ::Uuid::Create(), 512);
        MockAsset m_mockAsset;
        AZ::Data::Asset<AZ::Data::AssetData> m_assetData;
        AzFramework::SimpleAssetReference<FooMockSimpleAsset> m_simpleAssetReference;
    };

    namespace Internal
    {
        MockAsset s_mockAsset;
        MockAssetData s_mockAssetData;
        AZ::Data::Asset<AZ::Data::AssetData> s_asset;
        AZ::Data::AssetId s_assetId;
    }

    struct PythonReflectionAssetTypes
    {
        AZ_TYPE_INFO(PythonReflectionAssetTypes, "{04C929EE-67FA-4BDB-BC56-3680D61C9DEC}");

        AZ::Data::AssetId m_assetId;
        AZ::Data::Asset<AZ::Data::AssetData> m_assetData;
        AZ::Data::Asset<MyTestAssetData> m_myTestAssetDataAsset;
        MyTestAssetData m_testAssetData;
        ClassWithAssets m_mockDescriptor;
        AZStd::unique_ptr<MyTestAssetData> m_myTestAssetData;

        PythonReflectionAssetTypes()
        {
            m_testAssetData.m_number = 2;
            m_myTestAssetDataAsset = AZ::Data::Asset<MyTestAssetData>(
                                        static_cast<AZ::Data::AssetData*>(&m_testAssetData),
                                        AZ::Data::AssetLoadBehavior::NoLoad);
            m_assetId.m_guid = AZ::Uuid::CreateRandom();
            m_assetId.m_subId = 1234;
        }

        ~PythonReflectionAssetTypes()
        {
            // manually releasing the m_testAssetData
            m_testAssetData.SetUseCount(2);
            m_testAssetData.AcquireWeak();
            m_myTestAssetDataAsset = {};
        }

        static void PrintAssetData([[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& assetData)
        {
            AZ_TracePrintf("python", "Asset Data ID = %s\n",
                assetData.GetId().ToString<AZStd::string>().c_str());
        }

        static void PrintSimpleAssetReference([[maybe_unused]] const AzFramework::SimpleAssetReferenceBase& simpleAssetRef)
        {
            AZ_TracePrintf("python", "SimpleAssetReference of asset type = %s\n",
                simpleAssetRef.GetAssetType().ToString<AZStd::string>().c_str());
        }

        static AZ::Data::Asset<AZ::Data::AssetData> GenerateAsset()
        {
            Internal::s_assetId = AZ::Data::AssetId(AZ::Uuid::Create(), 42);
            Internal::s_mockAssetData.SetAssetId(Internal::s_assetId);
            Internal::s_asset = AZ::Data::Asset<AZ::Data::AssetData>(
                static_cast<AZ::Data::AssetData*>(&Internal::s_mockAssetData),
                AZ::Data::AssetLoadBehavior::NoLoad);
            return Internal::s_asset;
        }

        static AZ::Data::AssetId CreateAssetId(AZStd::string_view assetUuid)
        {
            return AZ::Data::AssetId::CreateString(assetUuid);
        }

        static bool CompareAssetIds(const AZ::Data::AssetId& lhs, const AZ::Data::AssetId& rhs)
        {
            return lhs == rhs;
        }

        static bool CompareAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& lhs, const AZ::Data::Asset<AZ::Data::AssetData>& rhs)
        {
            const bool sameId = lhs.GetId() == rhs.GetId();
            const bool sameType = lhs.GetType() == rhs.GetType();
            const bool sameHint = lhs.GetHint() == rhs.GetHint();
            return sameId && sameType && sameHint;
        }

        static bool CompareMockAssets(const MockAsset& lhs, const MockAsset& rhs)
        {
            return lhs.GetAssetPath() == rhs.GetAssetPath();
        }

        AZ::Data::Asset<MyTestAssetData> CreateMyTestAssetData()
        {
            m_myTestAssetData = AZStd::make_unique<MyTestAssetData>();
            m_myTestAssetData->m_number = 42;
            return AZ::Data::Asset<MyTestAssetData>(
                static_cast<AZ::Data::AssetData*>(m_myTestAssetData.get()),
                AZ::Data::AssetLoadBehavior::NoLoad);
        }

        void ReadMyTestAssetData(const AZ::Data::Asset<MyTestAssetData>& data)
        {
            if (data.Get())
            {
                AZ_TracePrintf("python", "AssetData: MyTestAssetData read in data \n");
            }
        }

        AZ::Data::Asset<AZ::Data::AssetData> CreateAssetHandle(const AZ::Data::AssetId& assetId)
        {
            return AZ::Data::Asset<AZ::Data::AssetData>(assetId, m_mockDescriptor.m_mockAsset.GetAssetType(), "test");
        }

        void Reflect(AZ::ReflectContext* context)
        {
            ClassWithAssets::Reflect(context);
            MockAsset::Reflect(context);

            auto&& serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<PythonReflectionAssetTypes>()
                    ->Field("assetId", &PythonReflectionAssetTypes::m_assetId)
                    ->Field("assetData", &PythonReflectionAssetTypes::m_assetData)
                    ->Field("myTestAssetData", &PythonReflectionAssetTypes::m_myTestAssetData)
                    ->Field("mockDescriptor", &PythonReflectionAssetTypes::m_mockDescriptor)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PythonReflectionAssetTypes>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    // class methods
                    ->Method("compare_asset_ids", &PythonReflectionAssetTypes::CompareAssetIds)
                    ->Method("compare_asset_data", &PythonReflectionAssetTypes::CompareAssetData)
                    ->Method("compare_mock_assets", &PythonReflectionAssetTypes::CompareMockAssets)
                    ->Method("create_asset_id", &PythonReflectionAssetTypes::CreateAssetId)
                    ->Method("print_asset_data", &PythonReflectionAssetTypes::PrintAssetData)
                    ->Method("print_simple_asset_reference", &PythonReflectionAssetTypes::PrintSimpleAssetReference)
                    ->Method("generate_asset", &PythonReflectionAssetTypes::GenerateAsset)
                    // instance methods
                    ->Method("create_asset_handle", &PythonReflectionAssetTypes::CreateAssetHandle)
                    ->Method("create_my_test_asset_data", &PythonReflectionAssetTypes::CreateMyTestAssetData)
                    ->Method("read_my_test_asset_data", &PythonReflectionAssetTypes::ReadMyTestAssetData)
                    // instance properties
                    ->Property("assetId", BehaviorValueProperty(&PythonReflectionAssetTypes::m_assetId))
                    ->Property("assetData", BehaviorValueProperty(&PythonReflectionAssetTypes::m_assetData))
                    ->Property("mockDescriptor", BehaviorValueProperty(&PythonReflectionAssetTypes::m_mockDescriptor))
                    ->Property("myTestAssetDataAsset", BehaviorValueProperty(&PythonReflectionAssetTypes::m_myTestAssetDataAsset))
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct PythonAssetTypesTests
        : public PythonTestingFixture
    {
        PythonTraceMessageSink m_testSink;

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            PythonTestingFixture::RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            // clearing up memory
            m_testSink.CleanUp();
            PythonTestingFixture::TearDown();
        }
    };

    TEST_F(PythonAssetTypesTests, AssetOnDemand)
    {
        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetSerializeContext());
        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetBehaviorContext());

        PythonReflectionAssetTypes pythonReflectionAssetTypes;
        pythonReflectionAssetTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionAssetTypes.Reflect(m_app.GetBehaviorContext());

        // make sure expected class names exist in the Behavior Context
        auto&& behaviorClasses = m_app.GetBehaviorContext()->m_classes;
        EXPECT_TRUE(behaviorClasses.find("Asset<AssetData>") != behaviorClasses.end());
        EXPECT_TRUE(behaviorClasses.find("Asset<MyTestAssetData>") != behaviorClasses.end());
        EXPECT_TRUE(behaviorClasses.find("SimpleAssetReferenceBase") != behaviorClasses.end());
        EXPECT_TRUE(behaviorClasses.find("SimpleAssetReference<AssetType><FooMockSimpleAsset >") != behaviorClasses.end());
    }

    TEST_F(PythonAssetTypesTests, AssetIdValues)
    {
        enum class LogTypes
        {
            Skip = 0,
            AssetId,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "AssetId"))
                {
                    return static_cast<int>(LogTypes::AssetId);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetSerializeContext());
        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetBehaviorContext());

        PythonReflectionAssetTypes pythonReflectionAssetTypes;
        pythonReflectionAssetTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionAssetTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr
                import azlmbr.asset
                import azlmbr.test

                compare_asset_ids = azlmbr.test.PythonReflectionAssetTypes_compare_asset_ids
                create_asset_id = azlmbr.test.PythonReflectionAssetTypes_create_asset_id

                assetIdOne = create_asset_id('{1F5252DC-467A-4E2E-8168-EE1551C92F74}:0')
                assetIdTwo = create_asset_id('{1F5252DC-467A-4E2E-8168-EE1551C92F74}:1')
                assetIdThree = azlmbr.asset.AssetId_CreateString('{BA5EBA11-DEAD-AB1E-FACE-01234567890A}:0')

                if(assetIdTwo.to_string() == '{1F5252DC-467A-4E2E-8168-EE1551C92F74}:1'):
                    print ('AssetId: compare_asset_ids assetIdTwo')

                if(assetIdThree.to_string() == '{BA5EBA11-DEAD-AB1E-FACE-01234567890A}:0'):
                    print ('AssetId: compare_asset_ids assetIdThree')

                if (compare_asset_ids(assetIdOne, assetIdOne)):
                    print ('AssetId: compare_asset_ids AFF')

                if (compare_asset_ids(assetIdOne, assetIdTwo) is False):
                    print ('AssetId: compare_asset_ids NEG')

                tester = azlmbr.test.PythonReflectionAssetTypes()
                tester.assetId = assetIdOne
                if (compare_asset_ids(tester.assetId, assetIdOne)):
                    print ('AssetId: compare_asset_ids tester')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on to run script buffer with %s", e.what());
            FAIL();
        }
        e.Deactivate();
        EXPECT_EQ(5, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::AssetId)]);
    }

    TEST_F(PythonAssetTypesTests, AssetDataTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            AssetData
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "AssetData"))
                {
                    return static_cast<int>(LogTypes::AssetData);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetSerializeContext());
        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetBehaviorContext());

        PythonReflectionAssetTypes pythonReflectionAssetTypes;
        pythonReflectionAssetTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionAssetTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr
                import azlmbr.asset
                import azlmbr.test

                compare_asset_data = azlmbr.test.PythonReflectionAssetTypes_compare_asset_data
                print_asset_data = azlmbr.test.PythonReflectionAssetTypes_print_asset_data
                generate_asset = azlmbr.test.PythonReflectionAssetTypes_generate_asset
                create_asset_id = azlmbr.test.PythonReflectionAssetTypes_create_asset_id

                tester = azlmbr.test.PythonReflectionAssetTypes()

                # AZ::Data::Asset<> testing
                assetIdOne = create_asset_id('{1F5252DC-467A-4E2E-8168-EE1551C92F74}:0')
                dataAsset = tester.create_asset_handle(assetIdOne)
                print_asset_data(tester.assetData)
                print_asset_data(dataAsset)
                tester.assetData = dataAsset

                mockAsset0 = generate_asset()
                mockAsset1 = generate_asset()
                if (compare_asset_data(mockAsset1, mockAsset1)):
                    print ('AssetData: compare_asset_data tester')

                # Compare testing
                if (compare_asset_data(tester.assetData, dataAsset)):
                    print ('AssetData: compare_asset_data tester.assetData')

                # handling generic Asset<MyTestAssetData>
                tester.read_my_test_asset_data(tester.myTestAssetDataAsset)
                testAssetData = tester.create_my_test_asset_data()
                tester.read_my_test_asset_data(testAssetData)
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on to run script buffer with %s", e.what());
            FAIL();
        }
        e.Deactivate();
        EXPECT_EQ(4, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::AssetData)]);
    }

    TEST_F(PythonAssetTypesTests, MockAssetTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            MockAsset
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "MockAsset"))
                {
                    return static_cast<int>(LogTypes::MockAsset);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetSerializeContext());
        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetBehaviorContext());

        PythonReflectionAssetTypes pythonReflectionAssetTypes;
        pythonReflectionAssetTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionAssetTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr
                import azlmbr.asset
                import azlmbr.test

                compare_mock_assets = azlmbr.test.PythonReflectionAssetTypes_compare_mock_assets

                tester0 = azlmbr.test.PythonReflectionAssetTypes()
                tester1 = azlmbr.test.PythonReflectionAssetTypes()

                if (compare_mock_assets(tester0.mockDescriptor.mockAsset, tester1.mockDescriptor.mockAsset)):
                    print('MockAsset: mock assets match')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on to run script buffer with %s", e.what());
            FAIL();
        }
        e.Deactivate();
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::MockAsset)]);
    }

    TEST_F(PythonAssetTypesTests, SimpleAssetReferenceTypes)
    {
        enum class LogTypes
        {
            Skip = 0,
            SimpleAssetReference
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "SimpleAssetReference"))
                {
                    return static_cast<int>(LogTypes::SimpleAssetReference);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetSerializeContext());
        AzFramework::SimpleAssetReferenceBase::Reflect(m_app.GetBehaviorContext());

        PythonReflectionAssetTypes pythonReflectionAssetTypes;
        pythonReflectionAssetTypes.Reflect(m_app.GetSerializeContext());
        pythonReflectionAssetTypes.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr
                import azlmbr.asset
                import azlmbr.test

                create_asset_id = azlmbr.test.PythonReflectionAssetTypes_create_asset_id
                print_simple_asset_reference = azlmbr.test.PythonReflectionAssetTypes_print_simple_asset_reference

                assetIdOne = create_asset_id('{1F5252DC-467A-4E2E-8168-EE1551C92F74}:0')
                assetIdTwo = create_asset_id('{1F5252DC-467A-4E2E-8168-EE1551C92F74}:1')

                tester = azlmbr.test.PythonReflectionAssetTypes()

                # SimpleAssetReferenceBase basic testing
                tester.testAssetId = assetIdOne
                accessAssetPath = tester.mockDescriptor.mockAsset.assetPath
                print_simple_asset_reference(tester.mockDescriptor.simpleAssetReference)

                # SimpleAssetReference<> testing
                fakeAssetPath = 'a/fake/asset_file.foo'
                mocker = tester.mockDescriptor
                simpleAssetReference = mocker.simpleAssetReference
                mocker.printFooMockSimpleAsset(simpleAssetReference)
                outAssetRef = mocker.createFooMockSimpleAsset(fakeAssetPath)
                if(simpleAssetReference.assetPath == fakeAssetPath):
                    print('SimpleAssetReference: path access matches {}'.format(fakeAssetPath))

                # using FooMockSimpleAsset inside a SimpleAssetReference<> template
                newFakeAssetPath = 'another/fake/asset_file.foo'
                simpleRef = azlmbr.object.construct('SimpleAssetReference<AssetType><FooMockSimpleAsset >')
                simpleRef.set_asset_path(newFakeAssetPath)
                if(simpleRef.assetPath == newFakeAssetPath):
                    print('SimpleAssetReference: simpleRef {}'.format(newFakeAssetPath))
                if(simpleRef.assetPath is not simpleAssetReference.assetPath):
                    print('SimpleAssetReference: simpleRef does not match simpleAssetReference')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on to run script buffer with %s", e.what());
            FAIL();
        }
        e.Deactivate();
        EXPECT_EQ(5, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::SimpleAssetReference)]);
    }

    TEST_F(PythonAssetTypesTests, MockBindingAssetIds)
    {
        enum class LogTypes
        {
            Skip = 0,
            MockBinding
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "MockBinding"))
                {
                    return static_cast<int>(LogTypes::MockBinding);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        MockBinding::Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr
                import azlmbr.mock
                import azlmbr.asset

                assetIdStringValue = '{13DACEEC-69B9-4CE4-9F43-50675D73FD8C}:0'
                testId = azlmbr.asset.AssetId_CreateString(assetIdStringValue)
                if (testId is not None):
                    print('MockBinding: created mock asset ID')

                if (testId.to_string() == assetIdStringValue):
                    print('MockBinding: created mock asset ID')

                testMock = azlmbr.mock.MockBinding(testId)
                if (testMock is not None):
                    print('MockBinding: mock binding created with asset ID')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on to run script buffer with %s", e.what());
        }
        e.Deactivate();
        EXPECT_EQ(3, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::MockBinding)]);
    }

    TEST_F(PythonAssetTypesTests, AssetIdsEqualOperators)
    {
        enum class LogTypes
        {
            Skip = 0,
            EqualOperators
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "EqualOperators"))
                {
                    return static_cast<int>(LogTypes::EqualOperators);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        MockBinding::Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr
                import azlmbr.asset

                assetIdStringValue0 = '{13DACEEC-69B9-4CE4-9F43-50675D73FD8C}:0'
                assetIdStringValue1 = '{13DACEEC-69B9-4CE4-9F43-50675D73FD8C}:1'

                testId0 = azlmbr.asset.AssetId_CreateString(assetIdStringValue0)
                if (testId0 is not None):
                    print('EqualOperators: created testId0')

                testId1 = azlmbr.asset.AssetId_CreateString(assetIdStringValue1)
                if (testId1 is not None):
                    print('EqualOperators: created testId1')

                if (testId1 == azlmbr.asset.AssetId_CreateString(assetIdStringValue1)):
                    print('EqualOperators: testId1 == testId1')

                if (testId0 != testId1):
                    print('EqualOperators: testId0 != testId1')

                if ((testId0 == assetIdStringValue0) is not True):
                    print('EqualOperators: testId0 != assetIdStringValue0')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on to run script buffer with %s", e.what());
        }
        e.Deactivate();
        EXPECT_EQ(5, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::EqualOperators)]);
    }
}
