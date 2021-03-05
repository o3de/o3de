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
#include <QTabWidget>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

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
            class IManifestObject;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class ManifestWidget;
            }

            class ManifestWidgetPage;

            class SCENE_UI_API ManifestWidget : public QWidget
            {
                Q_OBJECT
            public:
                using PageList = AZStd::vector<ManifestWidgetPage*>;

                explicit ManifestWidget(SerializeContext* serializeContext, QWidget* parent = nullptr);
                ~ManifestWidget() override;

                void BuildFromScene(const AZStd::shared_ptr<Containers::Scene>& scene);
                bool AddObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);
                bool RemoveObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);
                AZStd::shared_ptr<Containers::Scene> GetScene();
                AZStd::shared_ptr<const Containers::Scene> GetScene() const;

                //! Finds this ManifestWidget if the given widget is it's child, otherwise returns null.
                static ManifestWidget* FindRoot(QWidget* child);
                //! Finds this ManifestWidget if the given widget is it's child, otherwise returns null.
                static const ManifestWidget* FindRoot(const QWidget* child);

            protected:
                void BuildPages();
                void AddPage(const QString& category, ManifestWidgetPage* page);

                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                PageList m_pages;
                QScopedPointer<Ui::ManifestWidget> ui;
                AZStd::shared_ptr<Containers::Scene> m_scene;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
                SerializeContext* m_serializeContext;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
