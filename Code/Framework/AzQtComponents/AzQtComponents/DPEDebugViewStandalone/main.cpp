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

    class TestContainer
    {
    public:
        AZ_TYPE_INFO(TestContainer, "{86586583-A58F-45FD-BB6E-C3E9C76DDA38}");
        AZ_CLASS_ALLOCATOR(TestContainer, AZ::SystemAllocator, 0);

        enum class EnumType : AZ::s16
        {
            Value1 = 1,
            Value2 = 2,
            ValueZ = -10,
            NotReflected = 0xFF
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

        int m_simpleInt = 5;
        int m_readOnlyInt = 33;
        double m_doubleSlider = 3.25;
        AZStd::vector<AZStd::string> m_vector;
        AZStd::map<AZStd::string, float> m_map;
        AZStd::map<AZStd::string, float> m_readOnlyMap;
        AZStd::unordered_map<AZStd::pair<int, double>, int> m_unorderedMap;
        AZStd::unordered_map<EnumType, int> m_simpleEnum;
        AZStd::unordered_map<EnumType, double> m_immutableEnum;
        AZStd::set<int> m_set;
        AZStd::unordered_set<EnumType> m_unorderedSet;
        AZStd::unordered_multimap<int, AZStd::string> m_multiMap;
        AZStd::unordered_map<EnumType, AZStd::unordered_map<int, int>> m_nestedMap;
        AZStd::unordered_map<AZ::EntityId, int> m_entityIdMap;
        EnumType m_enumValue = EnumType::Value1;
        AZ::EntityId m_entityId;

        // For testing invocable ReadOnly attributes
        bool IsDataReadOnly()
        {
            return true;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestContainer>()
                    ->Field("simpleInt", &TestContainer::m_simpleInt)
                    ->Field("readonlyInt", &TestContainer::m_readOnlyInt)
                    ->Field("doubleSlider", &TestContainer::m_doubleSlider)
                    ->Field("vector", &TestContainer::m_vector)
                    ->Field("map", &TestContainer::m_map)
                    ->Field("map", &TestContainer::m_readOnlyMap)
                    ->Field("unorderedMap", &TestContainer::m_unorderedMap)
                    ->Field("simpleEnum", &TestContainer::m_simpleEnum)
                    ->Field("immutableEnum", &TestContainer::m_immutableEnum)
                    ->Field("set", &TestContainer::m_set)
                    ->Field("unorderedSet", &TestContainer::m_unorderedSet)
                    ->Field("multiMap", &TestContainer::m_multiMap)
                    ->Field("nestedMap", &TestContainer::m_nestedMap)
                    ->Field("entityIdMap", &TestContainer::m_entityIdMap)
                    ->Field("enumValue", &TestContainer::m_enumValue)
                    ->Field("entityId", &TestContainer::m_entityId);

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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleInt, "simple int", "")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &TestContainer::m_doubleSlider, "double slider", "")
                        ->Attribute(AZ::Edit::Attributes::Min, -10.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Containers")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_vector, "vector<string>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_map, "map<string, float>", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &TestContainer::m_unorderedMap, "unordered_map<pair<int, double>, int>", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleEnum, "unordered_map<enum, int>", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &TestContainer::m_immutableEnum, "unordered_map<enum, double> (fixed contents)",
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_enumValue, "enum (no multi-edit)", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &TestContainer::GetEnumValues)
                        ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_entityId, "entityId", "")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Button2)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Button 2 (should be at bottom)")
                        ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, true)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "ReadOnly")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_readOnlyInt, "readonly int", "")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TestContainer::IsDataReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_readOnlyMap, "readonly map<string, float>", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true);
                }
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

    testContainer.m_map["One"] = 1.f;
    testContainer.m_map["Two"] = 2.f;
    testContainer.m_map["million"] = 1000000.f;

    testContainer.m_readOnlyMap["A"] = 1.f;
    testContainer.m_readOnlyMap["B"] = 2.f;
    testContainer.m_readOnlyMap["C"] = 3.f;

    testContainer.m_unorderedMap[{1, 2.}] = 3;
    testContainer.m_unorderedMap[{ 4, 5. }] = 6;

    testContainer.m_simpleEnum[DPEDebugView::TestContainer::EnumType::Value1] = 1;
    testContainer.m_simpleEnum[DPEDebugView::TestContainer::EnumType::Value2] = 2;
    testContainer.m_simpleEnum[DPEDebugView::TestContainer::EnumType::ValueZ] = 10;

    testContainer.m_immutableEnum[DPEDebugView::TestContainer::EnumType::Value1] = 1.;
    testContainer.m_immutableEnum[DPEDebugView::TestContainer::EnumType::Value2] = 2.;
    testContainer.m_immutableEnum[DPEDebugView::TestContainer::EnumType::ValueZ] = 10.;

    testContainer.m_set.insert(1);
    testContainer.m_set.insert(3);
    testContainer.m_set.insert(5);

    testContainer.m_unorderedSet.insert(DPEDebugView::TestContainer::EnumType::Value1);
    testContainer.m_unorderedSet.insert(DPEDebugView::TestContainer::EnumType::ValueZ);

    testContainer.m_multiMap.insert({1, "one"});
    testContainer.m_multiMap.insert({ 2, "two" });
    testContainer.m_multiMap.insert({ 1, "also one" });

    QPointer<AzToolsFramework::DPEDebugWindow> debugViewer = new AzToolsFramework::DPEDebugWindow(nullptr);

    // create a real DPE to track the same adapter selected for the debug tool
    QPointer<AzToolsFramework::FilteredDPE> filteredDPE = new AzToolsFramework::FilteredDPE(nullptr);
    QObject::connect(
        debugViewer.data(), &AzToolsFramework::DPEDebugWindow::AdapterChanged, filteredDPE, &AzToolsFramework::FilteredDPE::SetAdapter);

    auto cvarAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::CvarAdapter>();
    auto sortFilter = AZStd::make_shared<AZ::DocumentPropertyEditor::ValueStringSort>();

    sortFilter->SetSourceAdapter(cvarAdapter);

    debugViewer->AddAdapterToList("CVar Adapter", sortFilter);
    debugViewer->AddAdapterToList("Reflection Adapter", AZStd::make_shared<AZ::DocumentPropertyEditor::ReflectionAdapter>(&testContainer, azrtti_typeid<DPEDebugView::TestContainer>()));
    debugViewer->AddAdapterToList("Example Adapter", AZStd::make_shared<AZ::DocumentPropertyEditor::ExampleAdapter>());
    debugViewer->AddAdapterToList("Settings Registry Adapter", AZStd::make_shared<AZ::DocumentPropertyEditor::SettingsRegistryAdapter>());

    debugViewer->show();
    filteredDPE->show();

    return qtApp.exec();
}
