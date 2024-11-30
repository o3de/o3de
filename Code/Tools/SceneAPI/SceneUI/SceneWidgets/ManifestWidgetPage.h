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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Uuid.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#endif

namespace AZ
{
    class SerializeContext;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IManifestObject;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class ManifestWidgetPage;
            }
            class ManifestWidgetPage 
                : public QWidget
                , public AzToolsFramework::IPropertyEditorNotify
                , public Events::ManifestMetaInfoBus::Handler
            {
                Q_OBJECT
            public:
                ManifestWidgetPage(SerializeContext* context, AZStd::vector<AZ::Uuid>&& classTypeIds);
                ~ManifestWidgetPage() override;

                // Sets the number of entries the user can add through this widget. It doesn't limit
                //      the amount of entries that can be stored.
                virtual void SetCapSize(size_t size);
                virtual size_t GetCapSize() const;

                virtual bool SupportsType(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);
                virtual bool AddObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);
                virtual bool RemoveObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);

                virtual size_t ObjectCount() const;
                virtual void Clear();

                virtual void ScrollToBottom();

                void RefreshPage(); // Called when a scene is initially loaded, after all objects are populated.

            signals:
                void SaveClicked();
                void ResetSettings();
                void ClearChanges();
                void AssignScript();
                void InspectClicked();

            public slots:
                void AppendUnsavedChangesToTitle(bool hasUnsavedChanges);
                void EnableInspector(bool enableInspector);

            protected slots:
                //! Callback that's triggered when the add button only has 1 entry.
                void OnSingleGroupAdd();
                void AddEditMenu();

            protected:
                //! Callback that's triggered when the add button has multiple entries.
                virtual void OnMultiGroupAdd(const Uuid& id);
                virtual void OnHelpButtonClicked();

                virtual void BuildAndConnectAddButton();
                virtual void BuildHelpButton();
                
                virtual AZStd::string ClassIdToName(const Uuid& id) const;
                virtual void AddNewObject(const Uuid& id);
                //! Report that an object on this page has been updated.
                //! @param object Pointer to the changed object. If the manifest itself has been update
                //! for instance after adding or removing a group use null to update the entire manifest.
                virtual void EmitObjectChanged(const DataTypes::IManifestObject* object = nullptr);

                // IPropertyEditorNotify Interface Methods
                void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
                void SealUndoStack() override;

                // ManifestMetaInfoBus
                void ObjectUpdated(const Containers::Scene& scene, const DataTypes::IManifestObject* target, void* sender) override;
                void AddObjects(AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>& objects) override;

                void UpdateAddButtonStatus();

                bool SetNodeReadOnlyStatus(const AzToolsFramework::InstanceDataNode* node);

                AZStd::vector<AZ::Uuid> m_classTypeIds;
                AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> m_objects;
                QScopedPointer<Ui::ManifestWidgetPage> ui;
                AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
                SerializeContext* m_context;
                size_t m_capSize;
                QString m_helpUrl;
                QMenu* m_editMenu;
                bool m_scrollToBottomQueued = false;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
