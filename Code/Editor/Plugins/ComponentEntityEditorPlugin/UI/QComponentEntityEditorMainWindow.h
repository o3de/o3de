/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QStackedWidget>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <CryCommon/ISystem.h>
#endif

class QObjectPropertyModel;
class PropertyInfo;

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class EntityPropertyEditor;
}

// This is the shell class to interface between Qt and the Sandbox.  All Sandbox implementation is retained in an inherited class.
class QComponentEntityEditorInspectorWindow
    : public QMainWindow
    , public ISystemEventListener
    , public AzToolsFramework::ToolsApplicationNotificationBus::Handler
    , public AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler
{
    Q_OBJECT

public:
    explicit QComponentEntityEditorInspectorWindow(QWidget* parent = 0);
    ~QComponentEntityEditorInspectorWindow();

    void Init();

    // Used to receive events from widgets where SIGNALS aren't available or implemented yet.
    // Required override.
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    // ToolsApplicationNotificationBus overrides...
    void AfterEntitySelectionChanged(
        [[maybe_unused]]const AzToolsFramework::EntityIdList& newlySelectedEntities,
        [[maybe_unused]]const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;

    // AssetBrowserPreviewRequestBus overrides...
    void PreviewAsset([[maybe_unused]]const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) override;

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {D7FEC1E3-8898-4D1F-8A9C-F8A161AF6746}
        static const GUID guid =
        {
            0xD7FEC1E3, 0x8898, 0x4D1F, { 0x8a, 0x9c, 0xf8, 0xa1, 0x61, 0xaf, 0x67, 0x46 }
        };
        return guid;
    }

    void closeEvent(QCloseEvent* ev) override;

    AzToolsFramework::EntityPropertyEditor* GetPropertyEditor() { return m_propertyEditor; }
    AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget* GetAssetBrowserInspector() { return m_assetBrowserInspector; }

private:

    AzToolsFramework::EntityPropertyEditor* m_propertyEditor;
    AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget* m_assetBrowserInspector;

    QStackedWidget* m_inspectorWidgetStack;
};
