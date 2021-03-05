#pragma once

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

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace AZ
{
    class SerializeContext;

    namespace SceneAPI
    {   
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGraphObject;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class SceneGraphInspectWidget;
            }

            class SceneGraphWidget;

            class SCENE_UI_API SceneGraphInspectWidget
                : public QWidget
            {
            public:
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL;

                explicit SceneGraphInspectWidget(const Containers::Scene& scene, QWidget* parent = nullptr, SerializeContext* context = nullptr);
                ~SceneGraphInspectWidget() override;

            protected:
                void OnSelectionChanged(AZStd::shared_ptr<const DataTypes::IGraphObject> item);

                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                QScopedPointer<Ui::SceneGraphInspectWidget> ui;
                QScopedPointer<SceneGraphWidget> m_graphView;
                QScopedPointer<AzToolsFramework::ReflectedPropertyEditor> m_propertyEditor;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

                SerializeContext* m_context;
            };
        } // UI
    } // SceneAPI
} // AZ
