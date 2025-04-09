#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                void ResetScene();

                void SetInspectButtonVisibility(bool enableInspector);

                //! Finds this ManifestWidget if the given widget is it's child, otherwise returns null.
                static ManifestWidget* FindRoot(QWidget* child);
                //! Finds this ManifestWidget if the given widget is it's child, otherwise returns null.
                static const ManifestWidget* FindRoot(const QWidget* child);

            signals:
                void SaveClicked();
                void OnInspect();
                void OnSceneResetRequested();
                void OnClearUnsavedChangesRequested();
                void OnAssignScript();
                void AppendUnsavedChangesToTitle(bool hasUnsavedChanges);
                void EnableInspector(bool enableInspector);

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
