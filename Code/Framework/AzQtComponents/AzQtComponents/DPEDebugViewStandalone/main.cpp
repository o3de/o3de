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
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QTreeView>

#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>
#include <AzQtComponents/DPEDebugViewStandalone/ui_DPEDebugWindow.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugModel.h>

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
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Create();
        }

        virtual ~DPEDebugApplication()
        {
            AZ::AllocatorInstance<AZ::Dom::ValueAllocator>::Destroy();
        }
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

    // create a default cvar adapter to expose the local CVar settings to edit
    AZ::DocumentPropertyEditor::CvarAdapter cvarAdapter;
    AzToolsFramework::DPEDebugModel adapterModel(nullptr);
    adapterModel.SetAdapter(&cvarAdapter);

    QPointer<DPEDebugView::DPEDebugWindow> theWindow = new DPEDebugView::DPEDebugWindow(nullptr);
    theWindow->m_treeView->setModel(&adapterModel);
    theWindow->show();

    return qtApp.exec();
}
