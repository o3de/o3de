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
{
    Q_OBJECT

public:
    explicit QComponentEntityEditorInspectorWindow(QWidget* parent = 0);
    ~QComponentEntityEditorInspectorWindow();

    void Init();

    // Used to receive events from widgets where SIGNALS aren't available or implemented yet.
    // Required override.
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

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

    AzToolsFramework::EntityPropertyEditor* GetPropertyEditor() { return m_propertyEditor; }

private:

    AzToolsFramework::EntityPropertyEditor* m_propertyEditor;
};
