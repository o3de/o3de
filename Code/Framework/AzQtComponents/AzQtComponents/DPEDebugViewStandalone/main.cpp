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

#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzQtComponents/DPEDebugViewStandalone/ui_DPEDebugWindow.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugModel.h>

namespace DPEDebugView
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

        int m_simpleInt = 5;
        double m_doubleSlider = 3.25;
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
                    ->Field("simpleInt", &TestContainer::m_simpleInt)
                    ->Field("doubleSlider", &TestContainer::m_doubleSlider)
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_simpleInt, "simple int", "")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &TestContainer::m_doubleSlider, "double slider", "")
                        ->Attribute(AZ::Edit::Attributes::Min, -10.0)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TestContainer::m_map, "map<string, float>", "")
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
} // namespace DPEDebugView

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(DPEDebugView::TestContainer::EnumType, "{14B71D68-2CEE-445C-A359-CAA16FB647DF}");
}

namespace DPEDebugView
{
    class DPEDebugWindow
        : public QMainWindow
        , public Ui::DPEDebugWindow
    {
    public:
        DPEDebugWindow(QWidget* parentWidget)
            : QMainWindow(parentWidget)
        {
            setupUi(this);
        }
    };

    class DPEDebugApplication : public AzToolsFramework::ToolsApplication
    {
    public:
        DPEDebugApplication(int* argc = nullptr, char*** argv = nullptr)
            : AzToolsFramework::ToolsApplication(argc, argv)
        {
            AZ::NameDictionary::Create();
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Create();

            m_propertyEditorSystem = AZStd::make_unique<AZ::DocumentPropertyEditor::PropertyEditorSystem>();
        }

        virtual ~DPEDebugApplication()
        {
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Destroy();

            m_propertyEditorSystem.reset();
        }

        void Reflect(AZ::ReflectContext* context) override
        {
            AzToolsFramework::ToolsApplication::Reflect(context);

            TestContainer::Reflect(context);
        }

    private:
        AZStd::unique_ptr<AZ::DocumentPropertyEditor::PropertyEditorSystem> m_propertyEditorSystem;
    };
} // namespace DPEDebugView

int main(int argc, char** argv)
{
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

#if 0 // change this to test with a reflection adapter instead
    // create a default cvar adapter to expose the local CVar settings to edit
    AZStd::shared_ptr<AZ::DocumentPropertyEditor::CvarAdapter> adapter = AZStd::make_shared<AZ::DocumentPropertyEditor::CvarAdapter>();
#else
    // create a reflection adapter for a TestContainer
    AZStd::shared_ptr<AZ::DocumentPropertyEditor::ReflectionAdapter> adapter =
        AZStd::make_shared<AZ::DocumentPropertyEditor::ReflectionAdapter>();
    DPEDebugView::TestContainer testContainer;
    adapter->SetValue(&testContainer, azrtti_typeid<DPEDebugView::TestContainer>());
#endif

    AzToolsFramework::DPEDebugModel adapterModel(nullptr);
    adapterModel.SetAdapter(adapter.get());

    QPointer<DPEDebugView::DPEDebugWindow> theWindow = new DPEDebugView::DPEDebugWindow(nullptr);
    theWindow->m_textView->SetAdapter(adapter);
    theWindow->m_treeView->setModel(&adapterModel);

    for (int columnIndex = 0, maxColumns = adapterModel.GetMaxColumns(); columnIndex < maxColumns; ++columnIndex)
    {
        // resize the columns to accommodate the displayed data
        theWindow->m_treeView->resizeColumnToContents(columnIndex);
    }
    theWindow->show();

    return qtApp.exec();
}
