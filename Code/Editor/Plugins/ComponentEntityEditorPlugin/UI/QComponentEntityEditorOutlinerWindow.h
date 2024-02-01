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

#include <Legacy/CryCommon/ISystem.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

class QObjectPropertyModel;
class PropertyInfo;

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    class EntityOutlinerWidget;
}

// This is the shell class to interface between Qt and the Sandbox.  All Sandbox implementation is retained in an inherited class.
class QEntityOutlinerWindow
    : public QMainWindow
    , public ISystemEventListener
{
    Q_OBJECT

public:
    explicit QEntityOutlinerWindow(QWidget* parent = 0);
    ~QEntityOutlinerWindow();

    void Init();

    // Used to receive events from widgets where SIGNALS aren't available or implemented yet.
    // Required override.
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {CEE50D0E-46A8-4CB4-9C5F-DCD374A78032}
        static const GUID guid =
        {
            0xcee50d0e, 0x46a8, 0x4cb4, { 0x9c, 0x5f, 0xdc, 0xd3, 0x74, 0xa7, 0x80, 0x32 }
        };
        return guid;
    }

private:
    AzToolsFramework::EntityOutlinerWidget* m_outlinerWidget;
};
