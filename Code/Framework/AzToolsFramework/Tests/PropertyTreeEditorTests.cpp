/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

#include <QtTest/QtTest>
#include <QApplication>

namespace UnitTest
{
    using namespace AzToolsFramework;

    struct PropertyTreeEditorSubBlockTester
    {
        AZ_TYPE_INFO(PropertyTreeEditorTester, "{E9497A1E-9B41-4A33-8F05-92CE41A0ABD9}");
        AZ::s16 m_myNegativeShort = -42;
    };

    struct MockAssetData
        : public AZ::Data::AssetData
    {
        AZ_RTTI(MyTestAssetData, "{8B0A8DCA-7F29-4B8E-B5D7-08E0EAB2C900}", AZ::Data::AssetData);

        MockAssetData(const AZ::Data::AssetId& assetId)
            : AssetData(assetId)
        {
            // to skip the automatic removal from the asset system
            m_useCount = 2;
        }
    };

    class TestSimpleAsset
    {
    public:
        AZ_TYPE_INFO(TestSimpleAsset, "{10A39072-9287-49FE-93C8-55F7715FC758}");

        bool m_data = false;

        static const char* GetFileFilter()
        {
            return "*.NaN";
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestSimpleAsset>()
                    ->Version(0)
                    ->Field("data", &TestSimpleAsset::m_data)
                    ;

                AzFramework::SimpleAssetReference<TestSimpleAsset>::Register(*serializeContext);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<TestSimpleAsset>("TestSimpleAsset", "Test data block for a simple asset mock data block")
                        ->DataElement(nullptr, &TestSimpleAsset::m_data, "My Data", "A test bool value.")
                        ;
                }
            }
        }
    };

    //! Test class
    struct PropertyTreeEditorTester
    {
        AZ_TYPE_INFO(PropertyTreeEditorTester, "{D3E17BE6-0FEB-4A04-B8BE-105A4666E79F}");

        int m_myInt = 42;
        int m_myNewInt = 43;
        bool m_myBool = true;
        float m_myFloat = 42.0f;
        AZStd::string m_myString = "StringValue";
        AZStd::string m_myGroupedString = "GroupedStringValue";
        PropertyTreeEditorSubBlockTester m_mySubBlock;
        double m_myHiddenDouble = 42.0;
        AZ::u16 m_myReadOnlyShort = 42;
        AZ::Data::Asset<MockAssetData> m_myAssetData;
        AzFramework::SimpleAssetReference<TestSimpleAsset> m_myTestSimpleAsset;

        struct PropertyTreeEditorNestedTester
        {
            AZ_TYPE_INFO(PropertyTreeEditorTester, "{F5814544-424D-41C5-A5AB-632371615B6A}");

            AZStd::string m_myNestedString = "NestedString";
        };

        AZStd::vector<PropertyTreeEditorNestedTester> m_myList;
        AZStd::unordered_map<AZStd::string, PropertyTreeEditorNestedTester> m_myMap;

        PropertyTreeEditorNestedTester m_nestedTester;
        PropertyTreeEditorNestedTester m_nestedTesterHiddenChildren;

        void Reflect(AZ::ReflectContext* context)
        {
            TestSimpleAsset::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PropertyTreeEditorSubBlockTester>()
                    ->Version(0)
                    ->Field("myNegativeShort", &PropertyTreeEditorSubBlockTester::m_myNegativeShort);

                serializeContext->Class<PropertyTreeEditorTester>()
                    ->Version(1)
                    ->Field("myInt", &PropertyTreeEditorTester::m_myInt)
                    ->Field("myBool", &PropertyTreeEditorTester::m_myBool)
                    ->Field("myFloat", &PropertyTreeEditorTester::m_myFloat)
                    ->Field("myString", &PropertyTreeEditorTester::m_myString)
                    ->Field("NestedTester", &PropertyTreeEditorTester::m_nestedTester)
                    ->Field("myNewInt", &PropertyTreeEditorTester::m_myNewInt)
                    ->Field("myGroupedString", &PropertyTreeEditorTester::m_myGroupedString)
                    ->Field("myList", &PropertyTreeEditorTester::m_myList)
                    ->Field("myMap", &PropertyTreeEditorTester::m_myMap)
                    ->Field("mySubBlock", &PropertyTreeEditorTester::m_mySubBlock)
                    ->Field("myHiddenDouble", &PropertyTreeEditorTester::m_myHiddenDouble)
                    ->Field("myReadOnlyShort", &PropertyTreeEditorTester::m_myReadOnlyShort)
                    ->Field("nestedTesterHiddenChildren", &PropertyTreeEditorTester::m_nestedTesterHiddenChildren)
                    ->Field("myAssetData", &PropertyTreeEditorTester::m_myAssetData)
                    ->Field("myTestSimpleAsset", &PropertyTreeEditorTester::m_myTestSimpleAsset)
                    ;

                serializeContext->Class<PropertyTreeEditorNestedTester>()
                    ->Version(1)
                    ->Field("myNestedString", &PropertyTreeEditorNestedTester::m_myNestedString)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<PropertyTreeEditorSubBlockTester>(
                        "PropertyTreeEditorSubBlock Tester", "Tester sub block for the PropertyTreeEditor test")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorSubBlockTester::m_myNegativeShort, "My Negative Short", "A test short int.")
                        ;

                    editContext->Class<PropertyTreeEditorTester>(
                        "PropertyTreeEditor Tester", "Tester for the PropertyTreeEditor")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myInt, "My Int", "A test int.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myBool, "My Bool", "A test bool.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myFloat, "My Float", "A test float.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myString, "My String", "A test string.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_nestedTester, "Nested", "A nested class.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myNewInt, "My New Int", "A test int.", "My Old Int")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myList, "My New List", "A test vector<>.", "My Old List")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myMap, "My Map", "A test unordered_map<>.", "My Old Map")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myAssetData, "My Asset Data", "An test asset data.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myTestSimpleAsset, "My Test Simple Asset", "A test simple asset ref.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myHiddenDouble, "My Hidden Double", "A test hidden node.", "My Old Double")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_nestedTesterHiddenChildren, "Nested Hidden Children", "A test node with hidden children.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myReadOnlyShort, "My Read Only", "A test read only node.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(nullptr, &PropertyTreeEditorTester::m_mySubBlock, "My Sub Block", "sub block test")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Grouped")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myGroupedString, "My Grouped String", "A test grouped string.")
                        ;

                    editContext->Class<PropertyTreeEditorNestedTester>(
                        "PropertyTreeEditor Nested Tester", "SubClass Tester for the PropertyTreeEditor")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorNestedTester::m_myNestedString, "My Nested String", "A test string.")
                        ;
                }
            }
        }
    };

    class PropertyTreeEditorTests
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            m_app.Start(AzFramework::Application::Descriptor());
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        ToolsTestApplication m_app{ "PropertyTreeEditorTests" };
        AZ::SerializeContext* m_serializeContext = nullptr;
    };

    TEST_F(PropertyTreeEditorTests, ReadPropertyTreeValues)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());


        // Test existing properties of different types

        {
            PropertyTreeEditor::PropertyAccessOutcome boolOutcome = propertyTree.GetProperty("My Bool");
            EXPECT_TRUE(boolOutcome.IsSuccess());
            EXPECT_TRUE(AZStd::any_cast<bool>(boolOutcome.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.GetProperty("My Int");
            EXPECT_TRUE(intOutcome.IsSuccess());
            EXPECT_EQ(AZStd::any_cast<int>(intOutcome.GetValue()), propertyTreeEditorTester.m_myInt);
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome floatOutcome = propertyTree.GetProperty("My Float");
            EXPECT_TRUE(floatOutcome.IsSuccess());
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(floatOutcome.GetValue()), propertyTreeEditorTester.m_myFloat);
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcome = propertyTree.GetProperty("My String");
            EXPECT_TRUE(stringOutcome.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcome.GetValue()).data(), propertyTreeEditorTester.m_myString.data());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.GetProperty("Nested|My Nested String");
            EXPECT_TRUE(nestedOutcome.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(nestedOutcome.GetValue()).data(), propertyTreeEditorTester.m_nestedTester.m_myNestedString.data());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome groupedOutcome = propertyTree.GetProperty("Grouped|My Grouped String");
            EXPECT_TRUE(groupedOutcome.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(groupedOutcome.GetValue()).data(), propertyTreeEditorTester.m_myGroupedString.data());
        }


        // Test non-existing properties

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.GetProperty("Wrong Property");
            EXPECT_FALSE(intOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.GetProperty("Nested|Wrong Nested Property");
            EXPECT_FALSE(nestedOutcome.IsSuccess());
        }

        {
            // Addressing the grouped property by name directly without the group should fail
            PropertyTreeEditor::PropertyAccessOutcome groupedOutcome = propertyTree.GetProperty("My Grouped String");
            EXPECT_FALSE(groupedOutcome.IsSuccess());
        }

    }

    TEST_F(PropertyTreeEditorTests, WritePropertyTreeValues)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());


        // Test existing properties of different types

        {
            PropertyTreeEditor::PropertyAccessOutcome boolOutcomeSet = propertyTree.SetProperty("My Bool", AZStd::any(false));
            EXPECT_TRUE(boolOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome boolOutcomeGet = propertyTree.GetProperty("My Bool");
            EXPECT_TRUE(boolOutcomeGet.IsSuccess());
            EXPECT_FALSE(AZStd::any_cast<bool>(boolOutcomeGet.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeSet = propertyTree.SetProperty("My Int", AZStd::any(48));
            EXPECT_TRUE(intOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome intOutcomeGet = propertyTree.GetProperty("My Int");
            EXPECT_TRUE(intOutcomeGet.IsSuccess());
            EXPECT_EQ(AZStd::any_cast<int>(intOutcomeGet.GetValue()), AZStd::any_cast<int>(intOutcomeSet.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome floatOutcomeSet = propertyTree.SetProperty("My Float", AZStd::any(48.0f));
            EXPECT_TRUE(floatOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome floatOutcomeGet = propertyTree.GetProperty("My Float");
            EXPECT_TRUE(floatOutcomeGet.IsSuccess());
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(floatOutcomeGet.GetValue()), AZStd::any_cast<float>(floatOutcomeSet.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeSet = propertyTree.SetProperty("My String", AZStd::make_any<AZStd::string>("New Value"));
            EXPECT_TRUE(stringOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeGet = propertyTree.GetProperty("My String");
            EXPECT_TRUE(stringOutcomeGet.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcomeSet.GetValue()).data(), AZStd::any_cast<AZStd::string>(stringOutcomeGet.GetValue()).data());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeSet = propertyTree.SetProperty("Nested|My Nested String", AZStd::make_any<AZStd::string>("New Nested Value"));
            EXPECT_TRUE(stringOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeGet = propertyTree.GetProperty("Nested|My Nested String");
            EXPECT_TRUE(stringOutcomeGet.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcomeSet.GetValue()).data(), AZStd::any_cast<AZStd::string>(stringOutcomeGet.GetValue()).data());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeSet = propertyTree.SetProperty("Grouped|My Grouped String", AZStd::make_any<AZStd::string>("New Grouped Value"));
            EXPECT_TRUE(stringOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeGet = propertyTree.GetProperty("Grouped|My Grouped String");
            EXPECT_TRUE(stringOutcomeGet.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcomeSet.GetValue()).data(), AZStd::any_cast<AZStd::string>(stringOutcomeGet.GetValue()).data());
        }


        // Test non-existing properties

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.SetProperty("Wrong Property", AZStd::any(12));
            EXPECT_FALSE(intOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.SetProperty("Nested|Wrong Nested Property", AZStd::make_any<AZStd::string>("Some Value"));
            EXPECT_FALSE(nestedOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome groupedOutcome = propertyTree.SetProperty("Grouped|Wrong Grouped Property", AZStd::make_any<AZStd::string>("Some Value"));
            EXPECT_FALSE(groupedOutcome.IsSuccess());
        }

        {
            // Addressing the grouped property by name directly without the group should fail
            PropertyTreeEditor::PropertyAccessOutcome groupedOutcome = propertyTree.SetProperty("My Grouped String", AZStd::make_any<AZStd::string>("Some Value"));
            EXPECT_FALSE(groupedOutcome.IsSuccess());
        }


        // Test existing properties with wrong type

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.SetProperty("My Int", AZStd::any(12.0f));
            EXPECT_FALSE(intOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.SetProperty("Nested|My Nested String", AZStd::any(42.0f));
            EXPECT_FALSE(nestedOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome groupedOutcome = propertyTree.SetProperty("Grouped|My Grouped String", AZStd::any(42.0f));
            EXPECT_FALSE(groupedOutcome.IsSuccess());
        }
    }

    TEST_F(PropertyTreeEditorTests, PropertyTreeVectorContainerSupport)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // IsContainer
        {
            EXPECT_FALSE(propertyTree.IsContainer("My New Int"));
            EXPECT_TRUE(propertyTree.IsContainer("My New List"));
        }

        // AddContainerItem
        {
            AZStd::any key = AZStd::make_any<AZ::s32>(0);
            AZStd::any value = AZStd::make_any<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>();

            PropertyTreeEditor::PropertyAccessOutcome outcomeAdd0 = propertyTree.AddContainerItem("My New Int", key, value);
            EXPECT_FALSE(outcomeAdd0.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome outcomeAdd1 = propertyTree.AddContainerItem("My New List", key, value);
            EXPECT_TRUE(outcomeAdd1.IsSuccess());
        }

        // GetContainerCount
        {
            EXPECT_FALSE(propertyTree.GetContainerCount("My New Int").IsSuccess());
            EXPECT_EQ(1, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My New List").GetValue()));
        }

        // GetContainerItem
        {
            AZStd::any key = AZStd::make_any<AZ::s32>(0);
            AZStd::any keyString = AZStd::make_any<AZStd::string_view>("0");

            EXPECT_FALSE(propertyTree.GetContainerItem("My New Int", key).IsSuccess());
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(propertyTree.GetContainerItem("My New List", keyString).IsSuccess());
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            PropertyTreeEditor::PropertyAccessOutcome outcome = propertyTree.GetContainerItem("My New List", key);
            EXPECT_TRUE(outcome.IsSuccess());
            if (outcome.IsSuccess())
            {
                auto&& testerValue = AZStd::any_cast<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>(&outcome.GetValue());
                EXPECT_STREQ("NestedString", testerValue->m_myNestedString.c_str());
            }
        }

        // UpdateContainerItem
        {
            AZStd::any key = AZStd::make_any<AZ::s32>(0);
            AZStd::any keyString = AZStd::make_any<AZStd::string_view>("0");
            PropertyTreeEditorTester::PropertyTreeEditorNestedTester testUpdate;
            testUpdate.m_myNestedString = "a new value";
            AZStd::any value = AZStd::make_any<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>(testUpdate);

            EXPECT_FALSE(propertyTree.UpdateContainerItem("My New Int", key, value).IsSuccess());
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(propertyTree.UpdateContainerItem("My New List", keyString, value).IsSuccess());
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_TRUE(propertyTree.UpdateContainerItem("My New List", key, value).IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome outcome = propertyTree.GetContainerItem("My New List", key);
            EXPECT_TRUE(outcome.IsSuccess());
            if (outcome.IsSuccess())
            {
                auto&& testerValue = AZStd::any_cast<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>(&outcome.GetValue());
                EXPECT_STREQ(testUpdate.m_myNestedString.c_str(), testerValue->m_myNestedString.c_str());
            }
        }

        // RemoveContainerItem
        {
            AZStd::any key = AZStd::make_any<AZ::s32>(0);
            AZStd::any keyString = AZStd::make_any<AZStd::string_view>("0");

            EXPECT_FALSE(propertyTree.RemoveContainerItem("My New Int", key).IsSuccess());
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(propertyTree.RemoveContainerItem("My New List", keyString).IsSuccess());
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            PropertyTreeEditor::PropertyAccessOutcome outcomeAdd1 = propertyTree.RemoveContainerItem("My New List", key);
            EXPECT_TRUE(outcomeAdd1.IsSuccess());
            EXPECT_EQ(0, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My New List").GetValue()));
        }

        // ResetContainer
        {
            AZStd::any value = AZStd::make_any<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>();

            propertyTree.AddContainerItem("My New List", AZStd::make_any<AZ::s32>(0), value);
            propertyTree.AddContainerItem("My New List", AZStd::make_any<AZ::s32>(1), value);
            propertyTree.AddContainerItem("My New List", AZStd::make_any<AZ::s32>(2), value);

            EXPECT_EQ(3, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My New List").GetValue()));
            propertyTree.ResetContainer("My New List");
            EXPECT_EQ(0, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My New List").GetValue()));
        }

        // AppendContainerItem
        {
            AZStd::any value = AZStd::make_any<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>();

            EXPECT_TRUE(propertyTree.AppendContainerItem("My New List", value).IsSuccess());
            EXPECT_TRUE(propertyTree.AppendContainerItem("My New List", value).IsSuccess());
            EXPECT_TRUE(propertyTree.AppendContainerItem("My New List", value).IsSuccess());

            EXPECT_EQ(3, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My New List").GetValue()));
            propertyTree.ResetContainer("My New List");
        }
    }

    TEST_F(PropertyTreeEditorTests, PropertyTreeUnorderedMapContainerSupport)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        using TestData = PropertyTreeEditorTester::PropertyTreeEditorNestedTester;
        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);
        propertyTreeEditorTester.m_myMap.emplace(AZStd::make_pair("one", TestData()));
        const char* testDataString = "a test string";

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // AddContainerItem
        {
            AZStd::any key = AZStd::make_any<AZStd::string>("two");
            TestData testItem;
            testItem.m_myNestedString = testDataString;
            AZStd::any value = AZStd::make_any<TestData>(testItem);
            EXPECT_TRUE(propertyTree.AddContainerItem("My Map", key, value).IsSuccess());
        }

        // GetContainerCount
        {
            EXPECT_EQ(2, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My Map").GetValue()));
        }

        // GetContainerItem
        {
            AZStd::any key = AZStd::make_any<AZStd::string>("two");

            PropertyTreeEditor::PropertyAccessOutcome outcome = propertyTree.GetContainerItem("My Map", key);
            EXPECT_TRUE(outcome.IsSuccess());
            if (outcome.IsSuccess())
            {
                auto&& testerValue = AZStd::any_cast<TestData>(&outcome.GetValue());
                EXPECT_STREQ(testDataString, testerValue->m_myNestedString.c_str());
            }
        }

        // UpdateContainerItem
        {
            AZStd::any key = AZStd::make_any<AZStd::string>("two");
            TestData testUpdate;
            testUpdate.m_myNestedString = "a new value";
            AZStd::any value = AZStd::make_any<TestData>(testUpdate);

            EXPECT_TRUE(propertyTree.UpdateContainerItem("My Map", key, value).IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome outcome = propertyTree.GetContainerItem("My Map", key);
            if (outcome.IsSuccess())
            {
                auto&& testerValue = AZStd::any_cast<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>(&outcome.GetValue());
                EXPECT_STREQ(testUpdate.m_myNestedString.c_str(), testerValue->m_myNestedString.c_str());
            }
        }

        // RemoveContainerItem
        {
            AZStd::any key = AZStd::make_any<AZStd::string>("two");

            EXPECT_TRUE(propertyTree.RemoveContainerItem("My Map", key).IsSuccess());
            EXPECT_EQ(1, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My Map").GetValue()));
        }

        // ResetContainer
        {
            EXPECT_EQ(1, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My Map").GetValue()));
            propertyTree.ResetContainer("My Map");
            EXPECT_EQ(0, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My Map").GetValue()));
        }

        // AppendContainerItem
        {
            AZStd::any value = AZStd::make_any<PropertyTreeEditorTester::PropertyTreeEditorNestedTester>();
            EXPECT_FALSE(propertyTree.AppendContainerItem("My Map", value).IsSuccess());
            EXPECT_EQ(0, AZStd::any_cast<AZ::u64>(propertyTree.GetContainerCount("My Map").GetValue()));
        }
    }

    TEST_F(PropertyTreeEditorTests, PropertyTreeInspection)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // BuildPathsList
        {
            auto&& pathList = propertyTree.BuildPathsList();
            EXPECT_TRUE(!pathList.empty());
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Map"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My New List"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "Nested|My Nested String"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "Grouped|My Grouped String"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Hidden Double"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Sub Block|My Negative Short"; }));
        }

        // BuildPathsListWithTypes
        {
            [[maybe_unused]] static auto stringContains = [](const AZStd::string& data, const char* subString) -> bool
            {
                return data.find(subString) != AZStd::string::npos;
            };
            auto&& pathList = propertyTree.BuildPathsListWithTypes();
            EXPECT_TRUE(!pathList.empty());
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return stringContains(path,"NotVisible"); }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return stringContains(path,"Visible"); }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return stringContains(path,"ShowChildrenOnly"); }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return stringContains(path,"HideChildren"); }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return stringContains(path,"ReadOnly"); }));
        }

        // GetPropertyType
        {
            EXPECT_TRUE(propertyTree.GetPropertyType("My Map").starts_with("AZStd::unordered_map"));
            EXPECT_TRUE(propertyTree.GetPropertyType("My New List").starts_with("AZStd::vector"));
            EXPECT_EQ("AZStd::string", propertyTree.GetPropertyType("Nested|My Nested String"));
            EXPECT_EQ("double", propertyTree.GetPropertyType("My Hidden Double"));
            EXPECT_EQ("PropertyTreeEditorTester", propertyTree.GetPropertyType("Nested"));
        }

        // BuildPathsList after enforcement removes the "show children only" nodes from the paths
        {
            propertyTree.SetVisibleEnforcement(true);

            auto&& pathList = propertyTree.BuildPathsList();
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Map"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My New List"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "Nested|My Nested String"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "Grouped|My Grouped String"; }));
            EXPECT_FALSE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Hidden Double"; }));
            EXPECT_FALSE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Sub Block|My Negative Short"; }));
            EXPECT_TRUE(AZStd::any_of(pathList.begin(), pathList.end(), [](auto&& path) { return path == "My Negative Short"; }));
        }
    }

    TEST_F(PropertyTreeEditorTests, PropertyTreeAttributeInspection)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // HasAttribute
        {
            EXPECT_TRUE(propertyTree.HasAttribute("My Read Only", "ReadOnly"));
            EXPECT_TRUE(propertyTree.HasAttribute("My Hidden Double", "Visibility"));
            EXPECT_TRUE(propertyTree.HasAttribute("My Sub Block", "AutoExpand"));
        }
    }

    TEST_F(PropertyTreeEditorTests, HandlesVisibleEnforcement)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // can access a hidden value with 'visible enforcement' set to false
        {
            PropertyTreeEditor::PropertyAccessOutcome outcomeGet = propertyTree.GetProperty("My Hidden Double");
            EXPECT_TRUE(outcomeGet.IsSuccess());
            EXPECT_EQ(42.0, AZStd::any_cast<double>(outcomeGet.GetValue()));
        }

        // can mutate a hidden value with 'visible enforcement' set to false
        {
            PropertyTreeEditor::PropertyAccessOutcome outcomeSet = propertyTree.SetProperty("My Hidden Double", AZStd::any(12.0));
            EXPECT_TRUE(outcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome outcomeGet = propertyTree.GetProperty("My Hidden Double");
            EXPECT_TRUE(outcomeGet.IsSuccess());
            EXPECT_EQ(12.0, AZStd::any_cast<double>(outcomeGet.GetValue()));
        }

        propertyTree.SetVisibleEnforcement(true);

        // can NOT access hidden value with 'visible enforcement' set to true
        {
            PropertyTreeEditor::PropertyAccessOutcome outcomeGet = propertyTree.GetProperty("My Hidden Double");
            EXPECT_FALSE(outcomeGet.IsSuccess());
        }

        // can NOT mutate a hidden value with 'visible enforcement' set to false
        {
            PropertyTreeEditor::PropertyAccessOutcome outcomeSet = propertyTree.SetProperty("My Hidden Double", AZStd::any(42.0));
            EXPECT_FALSE(outcomeSet.IsSuccess());
        }
    }

    TEST_F(PropertyTreeEditorTests, PropertyTreeDeprecatedNamesSupport)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // Test that new and deprecated name both refer to the same property
        {
            int newIntValue = 0;

            // get current value of My New Int
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeGet = propertyTree.GetProperty("My New Int");
            EXPECT_TRUE(intOutcomeGet.IsSuccess());

            newIntValue = AZStd::any_cast<int>(intOutcomeGet.GetValue());

            // Set new value to My Old Int
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeSet = propertyTree.SetProperty("My Old Int", AZStd::any(12));
            EXPECT_TRUE(intOutcomeSet.IsSuccess());

            // Read value of My New Int again
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeGetAgain = propertyTree.GetProperty("My New Int");
            EXPECT_TRUE(intOutcomeGetAgain.IsSuccess());

            // Verify that My Old Int and My New Int refer to the same property
            EXPECT_TRUE(AZStd::any_cast<int>(intOutcomeGetAgain.GetValue()) != newIntValue);
        }

    }

    TEST_F(PropertyTreeEditorTests, ClearWithEmptyAny)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ::Data::AssetId mockAssetId = AZ::Data::AssetId::CreateString("{66CC8A20-DC4D-4856-95FE-5C75A47B6A21}:0");
        MockAssetData mockAssetData(mockAssetId);
        AZ::Data::Asset<MockAssetData> mockAsset(&mockAssetData, AZ::Data::AssetLoadBehavior::Default);

        AzFramework::SimpleAssetReference<TestSimpleAsset> mockSimpleAsset;
        mockSimpleAsset.SetAssetPath("path/to/42");

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);
        propertyTreeEditorTester.m_myInt = 42;
        propertyTreeEditorTester.m_mySubBlock.m_myNegativeShort = -42;
        propertyTreeEditorTester.m_myList.push_back({});
        propertyTreeEditorTester.m_myAssetData = AZStd::move(mockAsset);
        propertyTreeEditorTester.m_myTestSimpleAsset = mockSimpleAsset;

        PropertyTreeEditor propertyTree(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());
        propertyTree.SetVisibleEnforcement(true);

        // use an empty any<> to set properties back to a default value
        {
            AZStd::any anEmpty;
            EXPECT_TRUE(propertyTree.SetProperty("My Int", anEmpty).IsSuccess());
            EXPECT_TRUE(propertyTree.SetProperty("My Negative Short", anEmpty).IsSuccess());
            EXPECT_TRUE(propertyTree.SetProperty("My New List", anEmpty).IsSuccess());
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_TRUE(propertyTree.SetProperty("My Asset Data", anEmpty).IsSuccess());
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_TRUE(propertyTree.SetProperty("My Test Simple Asset", anEmpty).IsSuccess());
        }

        // check that the properties went back to default values
        {
            EXPECT_EQ(0, propertyTreeEditorTester.m_myInt);
            EXPECT_EQ(0, propertyTreeEditorTester.m_mySubBlock.m_myNegativeShort);
            EXPECT_TRUE(propertyTreeEditorTester.m_myList.empty());
            EXPECT_FALSE(propertyTreeEditorTester.m_myAssetData.GetId().IsValid());
            EXPECT_TRUE(propertyTreeEditorTester.m_myTestSimpleAsset.GetAssetPath().empty());
        }
    }

} // namespace UnitTest
