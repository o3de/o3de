/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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

#include <QObject>

#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace GraphCanvas
{
    class AutomationUtils
    {
    public:
        // These have some issues with custom types due to dll boundaries and how the dynamic typing system in qt works.
        // So for the most part these will only return native qt types cleanly.
        template<typename T = QObject>
        static T* FindObjectById(GraphCanvas::EditorId editorId, AZ::Crc32 id)
        {
            QObject* object = nullptr;
            GraphCanvas::AssetEditorAutomationRequestBus::EventResult(object, editorId, &GraphCanvas::AssetEditorAutomationRequests::FindObject, id);

            return qobject_cast<T*>(object);
        }

        template<typename T = QObject>
        static T* FindObjectByName(GraphCanvas::EditorId editorId, QString name)
        {
            T* object = nullptr;
            GraphCanvas::AssetEditorAutomationRequestBus::EventResult(object, editorId, &GraphCanvas::AssetEditorAutomationRequests::FindElementByName, name);
            return qobject_cast<T*>(object);
        }
    };
}
