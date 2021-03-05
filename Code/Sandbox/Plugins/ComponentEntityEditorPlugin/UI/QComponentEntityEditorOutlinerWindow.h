/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

class QObjectPropertyModel;
class PropertyInfo;

namespace AZ
{
    class Entity;
}

class OutlinerWidget;

// This is the shell class to interface between Qt and the Sandbox.  All Sandbox implementation is retained in an inherited class.
class QComponentEntityEditorOutlinerWindow
    : public QMainWindow
    , public ISystemEventListener
{
    Q_OBJECT

public:
    explicit QComponentEntityEditorOutlinerWindow(QWidget* parent = 0);
    ~QComponentEntityEditorOutlinerWindow();

    void Init();

    // Used to receive events from widgets where SIGNALS aren't available or implemented yet.
    // Required override.
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {A2B58C0B-811A-4773-A057-A02D4BB9A293}
        static const GUID guid =
        {
            0xA2B58C0B, 0x811A, 0x4773, { 0xa0, 0x57, 0xa0, 0x2d, 0x4b, 0xb9, 0xa2, 0x93 }
        };
        return guid;
    }

private:
    OutlinerWidget* m_outlinerWidget;
};
