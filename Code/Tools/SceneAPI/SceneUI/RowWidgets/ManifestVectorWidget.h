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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#endif

namespace AZStd
{
    template<class T>
    class shared_ptr;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class InstanceDataNode;
}
namespace SerializeContext
{
    class IObjectFactory;
}
namespace AZ
{
    class SerializeContext;
    
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IManifestObject;
            class IGroup;
        }

        namespace SceneData
        {
            template<class T>
            class VectorWrapper;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class ManifestVectorWidget;
            }
            class ManifestVectorWidget
                : public QWidget
                , public AzToolsFramework::IPropertyEditorNotify
                , public Events::ManifestMetaInfoBus::Handler
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                using ManifestVectorType = AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject> >;

                ManifestVectorWidget(SerializeContext* serializeContext, QWidget* parent);
                ~ManifestVectorWidget() override;

                template<typename InputIterator>
                void SetManifestVector(InputIterator first, InputIterator last, DataTypes::IManifestObject* ownerObject)
                {
                    AZ_Assert(ownerObject, "ManifestVectorWidgets must be initialized with a non-null owner object.");
                    m_manifestVector.assign(first, last);
                    m_ownerObject = ownerObject;
                    UpdatePropertyGrid();
                }
                void SetManifestVector(const ManifestVectorType& manifestVector, DataTypes::IManifestObject* ownerObject);
                ManifestVectorType GetManifestVector();

                void SetCollectionName(const AZStd::string& name);
                void SetCollectionTypeName(const AZStd::string& typeName);
                // Sets the number of entries the user can add through this widget. It doesn't limit
                //      the amount of entries that can be stored.
                void SetCapSize(size_t cap);

                bool ContainsManifestObject(const DataTypes::IManifestObject* object) const;
                bool RemoveManifestObject(const DataTypes::IManifestObject* object);

            signals:
                void valueChanged();

            protected:
                void DisplayAddPrompt();
                void AddNewObject(SerializeContext::IObjectFactory* factory, const AZStd::string& typeName);
                void UpdatePropertyGrid();
                void UpdatePropertyGridSize();
                void EmitObjectChanged(const DataTypes::IManifestObject* object);

                // IPropertyEditorNotify
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
                void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* /*node*/, const QPoint& /*point*/) override;
                void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
                void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
                void SealUndoStack() override;

                // ManifestMetaInfoBus
                void ObjectUpdated(const Containers::Scene& scene, const DataTypes::IManifestObject* target, void* sender) override;

                SerializeContext* m_serializeContext;
                AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
                QScopedPointer<Ui::ManifestVectorWidget> m_ui;
                DataTypes::IManifestObject* m_ownerObject;
                ManifestVectorType m_manifestVector;
                size_t m_capSize;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
