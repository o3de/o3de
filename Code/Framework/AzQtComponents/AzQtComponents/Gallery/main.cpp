/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/Asset/CustomAssetTypeComponent.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include "ComponentDemoWidget.h"

#include <AzCore/Memory/SystemAllocator.h>

#include <QApplication>
#include <QMainWindow>
#include <QSettings>

#include <iostream>

const QString g_ui_1_0_SettingKey = QStringLiteral("useUI_1_0");

static void LogToDebug([[maybe_unused]] QtMsgType Type, [[maybe_unused]] const QMessageLogContext& Context, const QString& message)
{
    AZ::Debug::Platform::OutputToDebugger("Qt", message.toStdString().c_str());
    AZ::Debug::Platform::OutputToDebugger({}, "\n");
}

/*
 * Sets up and tears down everything we need for an AZ::ComponentApplication.
 * This is required for the ReflectedPropertyEditorPage.
 */
class ComponentApplicationWrapper
{
public:
    explicit ComponentApplicationWrapper()
    {
        AZ::ComponentApplication::Descriptor appDesc;
        m_systemEntity = m_componentApp.Create(appDesc);

        m_componentApp.RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
        m_componentApp.RegisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
        m_componentApp.RegisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());
        m_componentApp.RegisterComponentDescriptor(AZ::UserSettingsComponent::CreateDescriptor());
        m_componentApp.RegisterComponentDescriptor(AzFramework::CustomAssetTypeComponent::CreateDescriptor());
        m_componentApp.RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());

        m_systemEntity->CreateComponent<AZ::AssetManagerComponent>();
        m_systemEntity->CreateComponent<AZ::JobManagerComponent>();
        m_systemEntity->CreateComponent<AZ::StreamerComponent>();
        m_systemEntity->CreateComponent<AZ::UserSettingsComponent>();
        m_systemEntity->CreateComponent<AzFramework::CustomAssetTypeComponent>();
        m_systemEntity->CreateComponent<AzToolsFramework::Components::PropertyManagerComponent>();

        m_systemEntity->Init();
        m_systemEntity->Activate();

        m_serializeContext = m_componentApp.GetSerializeContext();
        m_serializeContext->CreateEditContext();
    }

    ~ComponentApplicationWrapper()
    {
        m_serializeContext->DestroyEditContext();
        m_systemEntity->Deactivate();

        std::vector<AZ::Component*> components =
        {
            m_systemEntity->FindComponent<AZ::AssetManagerComponent>(),
            m_systemEntity->FindComponent<AZ::JobManagerComponent>(),
            m_systemEntity->FindComponent<AZ::StreamerComponent>(),
            m_systemEntity->FindComponent<AZ::UserSettingsComponent>(),
            m_systemEntity->FindComponent<AzFramework::CustomAssetTypeComponent>(),
            m_systemEntity->FindComponent<AzToolsFramework::Components::PropertyManagerComponent>()
        };

        for (auto component : components)
        {
            m_systemEntity->RemoveComponent(component);
            delete component;
        }

        m_componentApp.UnregisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
        m_componentApp.UnregisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
        m_componentApp.UnregisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());
        m_componentApp.UnregisterComponentDescriptor(AZ::UserSettingsComponent::CreateDescriptor());
        m_componentApp.UnregisterComponentDescriptor(AzFramework::CustomAssetTypeComponent::CreateDescriptor());
        m_componentApp.UnregisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());

        m_componentApp.Destroy();
    }

private:
    // ComponentApplication must not be a pointer - it cannot be dynamically allocated
    AZ::ComponentApplication m_componentApp;
    AZ::Entity* m_systemEntity = nullptr;
    AZ::SerializeContext* m_serializeContext = nullptr;
};

int main(int argc, char **argv)
{
    const AZ::Debug::Trace tracer;
    ComponentApplicationWrapper componentApplicationWrapper;

    QApplication::setOrganizationName("O3DE");
    QApplication::setOrganizationDomain("o3de.org");
    QApplication::setApplicationName("O3DEWidgetGallery");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    qInstallMessageHandler(LogToDebug);

    AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::PerScreenDpiAware);
    QApplication app(argc, argv);

    auto globalEventFilter = new AzQtComponents::GlobalEventFilter(&app);
    app.installEventFilter(globalEventFilter);

    AzQtComponents::StyleManager styleManager(&app);
    AZ::IO::FixedMaxPath engineRootPath;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    styleManager.initialize(&app, engineRootPath);

    auto wrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionNone);
    auto widget = new ComponentDemoWidget(wrapper);
    wrapper->setGuest(widget);
    widget->resize(550, 900);
    widget->show();

    wrapper->enableSaveRestoreGeometry("windowGeometry");
    wrapper->restoreGeometryFromSettings();

    QObject::connect(widget, &ComponentDemoWidget::refreshStyle, &styleManager, [&styleManager]() {
        styleManager.Refresh();
    });

    app.setQuitOnLastWindowClosed(true);

    app.exec();
}
