/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QApplication>
#include <QPushButton>
#include <QMessageBox>

namespace StandaloneRPE
{
void Button1()
{
    QMessageBox::information(nullptr, "Button", "Button1 pressed");
}

void Button2()
{
    QMessageBox::information(nullptr, "Button", "Button2 pressed");
}

class TestContainer
{
public:
    AZ_TYPE_INFO(TestContainer, "{86586583-A58F-45FD-BB6E-C3E9C76DDA38}");
    AZ_CLASS_ALLOCATOR(TestContainer, AZ::SystemAllocator, 0);

    enum class EnumType : AZ::u8
    {
        Value1 = 1,
        Value2 = 2,
        ValueZ = 10,
        NotReflected = 0xFF
    };

    AZStd::map<AZStd::string, float> m_map;
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

    static void Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestContainer>()
                ->Field("map", &TestContainer::m_map)
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

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<EnumType>("EnumType", "")
                    ->Value("Value1", EnumType::Value1)
                    ->Value("Value2", EnumType::Value2)
                    ->Value("ValueZ", EnumType::ValueZ);

                editContext->Class<TestContainer>("TestContainer", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_map, "map<string, float>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_unorderedMap, "unordered_map<pair<int, double>, int>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleEnum, "unordered_map<enum, int>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_immutableEnum, "unordered_map<enum, double> (fixed contents)", "")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_set, "set<int>", "")
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::Slider)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_unorderedSet, "unordered_set<enum>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_multiMap, "unordered_multimap<int, string>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_nestedMap, "unordered_map<enum, unordered_map<int, int>>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_entityIdMap, "unordered_map<EntityId, Number>", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_enumValue, "enum (no multi-edit)", "")
                        ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_entityId, "entityId", "")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Button1)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Button1 (no multi-edit)")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Button2)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Button2 (multi-edit)")
                        ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, true);
            }
        }
    }
};

class SimpleKeyedContainer
{
public:
    AZ_TYPE_INFO(SimpleKeyedContainer, "{54E1AA90-1C09-47F4-8836-91600F225A34}");
    AZ_CLASS_ALLOCATOR(SimpleKeyedContainer, AZ::SystemAllocator, 0);

    AZStd::vector<AZStd::pair<int, int>> m_map;

    static void Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SimpleKeyedContainer>()
                ->Field("map", &SimpleKeyedContainer::m_map);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SimpleKeyedContainer>("SimpleKeyContainer", "")
                    ->DataElement(nullptr, &SimpleKeyedContainer::m_map, "map", "")
                        ->ElementAttribute(AZ::Edit::Attributes::ShowAsKeyValuePairs, true);
            }
        }
    }
};
} // StandaloneRPE

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(StandaloneRPE::TestContainer::EnumType, "{14B71D68-2CEE-445C-A359-CAA16FB647DF}");
}

namespace StandaloneRPE
{
class RPEApplication : public AzToolsFramework::ToolsApplication
{
public:
    RPEApplication(int* argc = nullptr, char*** argv = nullptr) : AzToolsFramework::ToolsApplication(argc, argv) {}

    void Reflect(AZ::ReflectContext* context) override
    {
        AzToolsFramework::ToolsApplication::Reflect(context);

        TestContainer::Reflect(context);
        SimpleKeyedContainer::Reflect(context);
    }
};
} // StandaloneRPE

int main(int argc, char** argv)
{
    AZ::IO::FixedMaxPath engineRootPath;
    {
        AZ::ComponentApplication componentApplication(argc, argv);
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

    StandaloneRPE::RPEApplication app(&argc, &argv);
    app.Start(AzFramework::Application::Descriptor());

    auto serializeContext = app.GetSerializeContext();

    AZStd::list<StandaloneRPE::TestContainer> testContainers;

    AzToolsFramework::ReflectedPropertyEditor* editor = new AzToolsFramework::ReflectedPropertyEditor(nullptr);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->Setup(serializeContext, nullptr, true);
    float newValue = 0.f;

    QPushButton* addInstanceButton = new QPushButton("Add Multi-Edit Instance");
    QPushButton* clearInstancesButton = new QPushButton("Clear Multi-Edit Instances");

    auto addInstance = [&]()
    {
        testContainers.emplace_back();
        StandaloneRPE::TestContainer& tc = testContainers.back();
        auto& m = tc.m_nestedMap[StandaloneRPE::TestContainer::EnumType::Value1];
        m[1] = 2;
        m[2] = 3;

        tc.m_immutableEnum[StandaloneRPE::TestContainer::EnumType::Value1] = 1.0;
        tc.m_immutableEnum[StandaloneRPE::TestContainer::EnumType::Value2] = 2.0;
        tc.m_immutableEnum[StandaloneRPE::TestContainer::EnumType::ValueZ] = 999.0;

        tc.m_map["A"] = 2.f + newValue;
        tc.m_map["B"] = 1.f;
        tc.m_map["C"] = -1.f;

        newValue += 1.f;

        void* aggregateInstance = nullptr;
        void* compareInstance = nullptr;
        if (testContainers.size() > 1)
        {
            aggregateInstance = &testContainers.front();
        }
        editor->AddInstance(&tc, azrtti_typeid<StandaloneRPE::TestContainer>(), aggregateInstance, compareInstance);
        editor->InvalidateAll();
        editor->ExpandAll();

        clearInstancesButton->setEnabled(testContainers.size() > 1);
    };
    addInstance();

    auto clearInstances = [&]()
    {
        auto& tc = testContainers.back();
        editor->ClearInstances();
        editor->AddInstance(&tc, azrtti_typeid<StandaloneRPE::TestContainer>());
        editor->InvalidateAll();
        editor->ExpandAll();
        while (testContainers.size() > 1)
        {
            testContainers.pop_front();
        }

        clearInstancesButton->setEnabled(false);
    };

    QObject::connect(addInstanceButton, &QPushButton::clicked, addInstance);
    QObject::connect(clearInstancesButton, &QPushButton::clicked, clearInstances);

    QWidget* containerWidget = new QWidget;
    QVBoxLayout* containerLayout = new QVBoxLayout(containerWidget);

    containerLayout->addWidget(editor);
    containerLayout->addWidget(addInstanceButton);
    containerLayout->addWidget(clearInstancesButton);

    containerWidget->resize(800, 600);
    containerWidget->show();

    return qtApp.exec();
}
