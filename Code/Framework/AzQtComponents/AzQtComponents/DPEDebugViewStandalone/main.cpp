/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QMessageBox>
#include <QTreeView>
#include <QScrollArea>
#include <QComboBox>

#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzFramework/DocumentPropertyEditor/AggregateAdapter.h>
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzFramework/DocumentPropertyEditor/SettingsRegistryAdapter.h>
#include <AzFramework/DocumentPropertyEditor/ValueStringSort.h>
#include <AzQtComponents/DPEDebugViewStandalone/ExampleAdapter.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugWindow.h>

#include <AzToolsFramework/UI/DocumentPropertyEditor/FilteredDPE.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>

#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

namespace DPEDebugView
{
    void Button1()
    {
        QMessageBox::information(nullptr, "Button", "Button1 pressed");
    }

    AZ::Crc32 Button2()
    {
        QMessageBox::information(nullptr, "Button", "Button2 pressed");
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZStd::string GetButtonText()
    {
        return "Button 2 (should be at bottom)";
    }

    class TestContainer
    {
    public:
        AZ_TYPE_INFO(TestContainer, "{86586583-A58F-45FD-BB6E-C3E9C76DDA38}");
        AZ_CLASS_ALLOCATOR(TestContainer, AZ::SystemAllocator);

        enum class EnumType : AZ::s16
        {
            Value1 = 1,
            Value2 = 2,
            ValueZ = -10,
            NotReflected = 0xFF
        };

        // This enum is being used to test parsing of AZ::Edit::EnumConstant attributes from the reflection context
        enum class EnumValueType : char
        {
            AValue,
            BValue,
            CValue
        };

        AZStd::vector<AZ::Edit::EnumConstant<EnumType>> GetEnumValues() const
        {
            AZStd::vector<AZ::Edit::EnumConstant<EnumType>> values;
            values.emplace_back(EnumType::Value1, "Value 1");
            values.emplace_back(EnumType::Value2, "Value 2");
            values.emplace_back(EnumType::ValueZ, "Value Z");
            values.emplace_back(EnumType::NotReflected, "Not Reflected (set from EnumValues)");
            return values;
        }

        struct GenericValueTestType
        {
            AZ_RTTI(GenericValueTestType, "{9EBA5A84-02A3-46EB-8C10-22246FD0EFDF}");

            GenericValueTestType() = default;
            GenericValueTestType(AZStd::string_view id)
            {
                m_id = id;
            }
            virtual ~GenericValueTestType() = default;

            static void Reflect(AZ::ReflectContext* context)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<GenericValueTestType>()->Version(1)
                        ->Field("m_id", &GenericValueTestType::m_id);
                }
            }

            bool operator==(const GenericValueTestType& other) const
            {
                return this->m_id == other.m_id;
            }

            bool operator!=(const GenericValueTestType& other) const
            {
                return !operator==(other);
            }

            AZStd::string m_id;
        };

        using GenericValueListType = AZStd::vector<AZStd::pair<GenericValueTestType, AZStd::string>>;
        GenericValueListType GetGenericValueList()
        {
            static GenericValueListType genericValueList;

            if (genericValueList.empty())
            {
                genericValueList.push_back(AZStd::make_pair(GenericValueTestType{ "id1" }, "FirstId"));
                genericValueList.push_back(AZStd::make_pair(GenericValueTestType{ "id2" }, "SecondId"));
                genericValueList.push_back(AZStd::make_pair(GenericValueTestType{ "id3" }, "ThirdId"));
            }

            return genericValueList;
        }

        using EnumValuesAsPairsObjToStringType = AZStd::vector<AZStd::pair<GenericValueTestType, AZStd::string>>;
        EnumValuesAsPairsObjToStringType GetEnumValuesInGenericValueListFormat()
        {
            static EnumValuesAsPairsObjToStringType enumValuesList;

            if (enumValuesList.empty())
            {
                enumValuesList.push_back(AZStd::make_pair(GenericValueTestType{ "enumValues1" }, "enumValues1"));
                enumValuesList.push_back(AZStd::make_pair(GenericValueTestType{ "enumValues2" }, "enumValues2"));
                enumValuesList.push_back(AZStd::make_pair(GenericValueTestType{ "enumValues3" }, "enumValues3"));
            }

            return enumValuesList;
        }

        bool m_groupToggle = false;
        int m_subToggleInt = 16;
        bool m_toggle = false;
        int m_simpleInt = 5;
        int m_readOnlyInt = 33;
        double m_doubleSlider = 3.25;
        AZStd::string m_simpleString = "test";
        AZStd::vector<AZStd::string> m_vector;
        AZStd::vector<AZStd::vector<int>> m_vectorOfVectors;
        AZStd::map<AZStd::string, float> m_map;
        AZStd::map<int, float> m_readOnlyMap;
        AZStd::unordered_map<AZStd::pair<int, double>, int> m_unorderedMap;
        AZStd::unordered_map<EnumType, int> m_simpleEnumMap;
        AZStd::unordered_map<EnumType, double> m_immutableEnumMap;
        AZStd::set<int> m_set;
        AZStd::unordered_set<EnumType> m_unorderedSet;
        AZStd::unordered_multimap<int, AZStd::string> m_multiMap;
        AZStd::unordered_map<EnumType, AZStd::unordered_map<int, int>> m_nestedMap;
        AZStd::unordered_map<AZ::EntityId, int> m_entityIdMap;
        EnumType m_enumValue = EnumType::Value1;
        EnumType m_enumValueInlineDeclaration = EnumType::ValueZ;
        EnumValueType m_enumValueType = EnumValueType::BValue;
        GenericValueTestType m_genericValueList = { "id3" };
        GenericValueTestType m_enumValuesInGenericValueListFormat = { "enumValues2" };
        AZ::EntityId m_entityId;

        // For testing invocable ReadOnly attributes
        bool IsDataReadOnly()
        {
            return true;
        }

        bool IsGroupToggleOff() const
        {
            return !m_groupToggle;
        }

        AZ::Crc32 IsToggleEnabled() const
        {
            return m_toggle ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        double DoubleMin() const
        {
            return -9.0;
        }

        double DoubleMax() const
        {
            return 9.0;
        }

        int IntMin() const
        {
            return -8;
        }

        int IntMax() const
        {
            return 8;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestContainer>()
                    ->Field("toggle", &TestContainer::m_toggle)
                    ->Field("simpleInt", &TestContainer::m_simpleInt)
                    ->Field("doubleSlider", &TestContainer::m_doubleSlider)
                    ->Field("simpleString", &TestContainer::m_simpleString)
                    ->Field("vector", &TestContainer::m_vector)
                    ->Field("vectorOfVectors", &TestContainer::m_vectorOfVectors)
                    ->Field("map", &TestContainer::m_map)
                    ->Field("unorderedMap", &TestContainer::m_unorderedMap)
                    ->Field("simpleEnum", &TestContainer::m_simpleEnumMap)
                    ->Field("immutableEnum", &TestContainer::m_immutableEnumMap)
                    ->Field("set", &TestContainer::m_set)
                    ->Field("unorderedSet", &TestContainer::m_unorderedSet)
                    ->Field("multiMap", &TestContainer::m_multiMap)
                    ->Field("nestedMap", &TestContainer::m_nestedMap)
                    ->Field("entityIdMap", &TestContainer::m_entityIdMap)
                    ->Field("enumValue", &TestContainer::m_enumValue)
                    ->Field("enumValueInlineDeclaration", &TestContainer::m_enumValueInlineDeclaration)
                    ->Field("enumValueType", &TestContainer::m_enumValueType)
                    ->Field("genericValueList", &TestContainer::m_genericValueList)
                    ->Field("enumValuesInGenericValueListFormat", &TestContainer::m_enumValuesInGenericValueListFormat)
                    ->Field("entityId", &TestContainer::m_entityId)
                    ->Field("readonlyInt", &TestContainer::m_readOnlyInt)
                    ->Field("map", &TestContainer::m_readOnlyMap)
                    ->Field("groupToggle", &TestContainer::m_groupToggle)
                    ->Field("subToggleInt", &TestContainer::m_subToggleInt);

                serializeContext->Enum<EnumType>()
                    ->Value("Value1", EnumType::Value1)
                    ->Value("Value2", EnumType::Value2)
                    ->Value("ValueZ", EnumType::ValueZ);

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<TestContainer>("TestContainer", "")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Button1)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Button 1 (should be at top)")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Simple Types")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_toggle, "toggle switch", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleInt, "simple int", "")
                        ->Attribute(AZ::Edit::Attributes::Min, &TestContainer::IntMin)
                        ->Attribute(AZ::Edit::Attributes::Max, &TestContainer::IntMax)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &TestContainer::m_doubleSlider, "double slider", "")
                        ->Attribute(AZ::Edit::Attributes::Min, &TestContainer::DoubleMin)
                        ->Attribute(AZ::Edit::Attributes::Max, &TestContainer::DoubleMax)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleString, "simple string", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &TestContainer::IsToggleEnabled)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Containers")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_vector, "vector<string>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_vectorOfVectors, "vector<vector<int>>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_map, "map<string, float>", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &TestContainer::m_unorderedMap, "unordered_map<pair<int, double>, int>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleEnumMap, "unordered_map<enum, int>", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default,
                            &TestContainer::m_immutableEnumMap,
                            "unordered_map<enum, double> (fixed contents)",
                            "")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_set, "set<int>", "")
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::Slider)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_unorderedSet, "unordered_set<enum>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_multiMap, "unordered_multimap<int, string>", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &TestContainer::m_nestedMap, "unordered_map<enum, unordered_map<int, int>>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_entityIdMap, "unordered_map<EntityId, Number>", "")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TestContainer::m_enumValue, "EnumValues ComboBox", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &TestContainer::GetEnumValues)
                        ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, false)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox,
                            &TestContainer::m_enumValueInlineDeclaration,
                            "EnumValues ComboBox (inline decl)", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues,
                            AZStd::vector<AZ::Edit::EnumConstant<TestContainer::EnumType>>{
                                AZ::Edit::EnumConstant<TestContainer::EnumType>(TestContainer::EnumType::Value1, "Value1"),
                                AZ::Edit::EnumConstant<TestContainer::EnumType>(TestContainer::EnumType::Value2, "Value2"),
                                AZ::Edit::EnumConstant<TestContainer::EnumType>(TestContainer::EnumType::ValueZ, "ValueZ") })
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TestContainer::m_enumValueType, "EnumValue ComboBox", "")
                        ->EnumAttribute(EnumValueType::AValue, "AValueDescription")
                        ->EnumAttribute(EnumValueType::BValue, "BValueDescription")
                        ->EnumAttribute(EnumValueType::CValue, "CValueDescription")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TestContainer::m_genericValueList, "GenericValueList ComboBox", "")
                        ->Attribute(AZ::Edit::Attributes::GenericValueList, &TestContainer::GetGenericValueList)
                        ->DataElement(
                            AZ::Edit::UIHandlers::ComboBox,
                            &TestContainer::m_enumValuesInGenericValueListFormat,
                            "EnumValues in GenericValueList format ComboBox",
                            "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &TestContainer::GetEnumValuesInGenericValueListFormat)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_entityId, "entityId", "")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Button2)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, &GetButtonText)
                        ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, true)
                        ->UIElement(AZ::Edit::UIHandlers::Label, "")
                        ->Attribute(
                            AZ::Edit::Attributes::ValueText,
                            "<h2>Label UIElement</h2>This is a test of a UIElement label<br>that can handle multiple lines of text that "
                            "also can be wrapped onto newlines<br>and can also support <a "
                            "href='https://doc.qt.io/qt-5/richtext-html-subset.html'>rich text</a>")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "ReadOnly")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_readOnlyInt, "readonly int", "")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TestContainer::IsDataReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_readOnlyMap, "readonly map<string, float>", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->GroupElementToggle("Group Toggle", &TestContainer::m_groupToggle)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_subToggleInt, "sub-toggle int", "")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TestContainer::IsGroupToggleOff);
                }

                GenericValueTestType::Reflect(context);
            }
        }
    };
} // namespace DPEDebugView

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(DPEDebugView::TestContainer::EnumType, "{14B71D68-2CEE-445C-A359-CAA16FB647DF}");
}

namespace DPEDebugView
{
    class DPEDebugApplication
        : public AzToolsFramework::ToolsApplication
    {
    public:
        using AzToolsFramework::ToolsApplication::ToolsApplication;

        void Reflect(AZ::ReflectContext* context) override
        {
            AzToolsFramework::ToolsApplication::Reflect(context);

            TestContainer::Reflect(context);
        }
    };
} // namespace DPEDebugView

int main(int argc, char** argv)
{
    const AZ::Debug::Trace tracer;
    DPEDebugView::DPEDebugApplication app(&argc, &argv);
    AZ::IO::FixedMaxPath engineRootPath;
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    AzQtComponents::PrepareQtPaths();
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication qtApp(argc, argv);
    AzQtComponents::StyleManager styleManager(&qtApp);
    styleManager.initialize(&qtApp, engineRootPath);

    app.Start(AzFramework::Application::Descriptor());

    // store a list of selectable adapters to switch between
    DPEDebugView::TestContainer testContainer;

    testContainer.m_vector.push_back("one");
    testContainer.m_vector.push_back("two");
    testContainer.m_vector.push_back("the third");

    testContainer.m_vectorOfVectors.push_back({ 0, 1 });
    testContainer.m_vectorOfVectors.push_back({ 2, 3 });

    testContainer.m_map["One"] = 1.f;
    testContainer.m_map["Two"] = 2.f;
    testContainer.m_map["million"] = 1000000.f;

    testContainer.m_readOnlyMap[2] = 1.f;
    testContainer.m_readOnlyMap[4] = 2.f;
    testContainer.m_readOnlyMap[6] = 3.f;

    testContainer.m_unorderedMap[{1, 2.}] = 3;
    testContainer.m_unorderedMap[{ 4, 5. }] = 6;

    testContainer.m_simpleEnumMap[DPEDebugView::TestContainer::EnumType::Value1] = 1;
    testContainer.m_simpleEnumMap[DPEDebugView::TestContainer::EnumType::Value2] = 2;
    testContainer.m_simpleEnumMap[DPEDebugView::TestContainer::EnumType::ValueZ] = 10;

    testContainer.m_immutableEnumMap[DPEDebugView::TestContainer::EnumType::Value1] = 1.;
    testContainer.m_immutableEnumMap[DPEDebugView::TestContainer::EnumType::Value2] = 2.;
    testContainer.m_immutableEnumMap[DPEDebugView::TestContainer::EnumType::ValueZ] = 10.;

    testContainer.m_set.insert(1);
    testContainer.m_set.insert(3);
    testContainer.m_set.insert(5);

    testContainer.m_unorderedSet.insert(DPEDebugView::TestContainer::EnumType::Value1);
    testContainer.m_unorderedSet.insert(DPEDebugView::TestContainer::EnumType::ValueZ);

    testContainer.m_multiMap.insert({ 1, "one"});
    testContainer.m_multiMap.insert({ 2, "two" });
    testContainer.m_multiMap.insert({ 1, "also one" });

    QPointer<AzToolsFramework::DPEDebugWindow> debugViewer = new AzToolsFramework::DPEDebugWindow(nullptr);

    AzToolsFramework::RegisterGenericComboBoxHandler<DPEDebugView::TestContainer::GenericValueTestType>();

    // create a real DPE to track the same adapter selected for the debug tool
    QPointer<AzToolsFramework::FilteredDPE> filteredDPE = new AzToolsFramework::FilteredDPE(nullptr);
    QObject::connect(
        debugViewer.data(), &AzToolsFramework::DPEDebugWindow::AdapterChanged, filteredDPE, &AzToolsFramework::FilteredDPE::SetAdapter);

    auto cvarAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::CvarAdapter>();
    auto sortFilter = AZStd::make_shared<AZ::DocumentPropertyEditor::ValueStringSort>();

    sortFilter->SetSourceAdapter(cvarAdapter);

    debugViewer->AddAdapterToList(
        "Reflection Adapter",
        AZStd::make_shared<AZ::DocumentPropertyEditor::ReflectionAdapter>(&testContainer, azrtti_typeid<DPEDebugView::TestContainer>()));
    debugViewer->AddAdapterToList("CVar Adapter", sortFilter);
    debugViewer->AddAdapterToList("Example Adapter", AZStd::make_shared<AZ::DocumentPropertyEditor::ExampleAdapter>());
    debugViewer->AddAdapterToList("Settings Registry Adapter", AZStd::make_shared<AZ::DocumentPropertyEditor::SettingsRegistryAdapter>());

    // Important! Note that the following type must already be exposed to the reflection system, so we will re-use AZStd::map<int, float>,
    // which was previously used for m_readOnlyMap
    AZStd::map<int, float> firstVector = { 
        { 1, 1. },
        { 2, 2. },
        { 3, 3. },
        { 4, 4. },
    };

    auto firstContainerAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::ReflectionAdapter>(&firstVector, azrtti_typeid<AZStd::map<int, float>>());
    // Note: uncomment to generate DPE to view the the firstVector adapter
    /*AzToolsFramework::DocumentPropertyEditor firstDPE;
    firstDPE.SetAdapter(firstContainerAdapter);
    firstDPE.show();*/

    AZStd::map<int, float> secondVector = {
        { 2, 2. },
        { 3, 3.5 },
        { 4, 4. },
        { 5, 5. },
    };
    auto secondContainerAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::ReflectionAdapter>(&secondVector, azrtti_typeid<AZStd::map<int, float>>());
    // Note: uncomment to generate DPE to view the the secondVector adapter
    /*AzToolsFramework::DocumentPropertyEditor secondDPE;
    secondDPE.SetAdapter(secondContainerAdapter);
    secondDPE.show();*/

    auto aggregateAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::LabeledRowAggregateAdapter>();
    aggregateAdapter->AddAdapter(firstContainerAdapter);
    aggregateAdapter->AddAdapter(secondContainerAdapter);

    debugViewer->AddAdapterToList("Vector AggregateAdapter", aggregateAdapter);
    debugViewer->show();
    filteredDPE->show();

    return qtApp.exec();
}
