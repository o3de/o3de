#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
