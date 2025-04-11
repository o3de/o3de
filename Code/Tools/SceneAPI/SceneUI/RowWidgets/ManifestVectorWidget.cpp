/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QEvent>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>
#include <RowWidgets/ui_ManifestVectorWidget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ManifestVectorWidget, SystemAllocator);

            ManifestVectorWidget::ManifestVectorWidget(SerializeContext* serializeContext, QWidget* parent)
                : QWidget(parent)
                , m_serializeContext(serializeContext)
                , m_propertyEditor(nullptr)
                , m_ui(new Ui::ManifestVectorWidget())
                , m_capSize(50)
            {
                m_ui->setupUi(this);

                m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
                m_propertyEditor->Setup(m_serializeContext, this, false, 175);
                m_propertyEditor->show();
                m_ui->m_mainLayout->insertWidget(1, m_propertyEditor);

                m_ui->m_addObjectButton->setProperty("class", "FixedMenu");
                connect(m_ui->m_addObjectButton, &QPushButton::pressed, this, &ManifestVectorWidget::DisplayAddPrompt);

                // Add empty menu for visual consistency.
                m_ui->m_addObjectButton->setMenu(new QMenu(this));

                BusConnect();
            }

            ManifestVectorWidget::~ManifestVectorWidget()
            {
                BusDisconnect();
            }

            void ManifestVectorWidget::SetManifestVector(const ManifestVectorType& manifestVector, DataTypes::IManifestObject* ownerObject)
            {
                AZ_Assert(ownerObject, "ManifestVectorWidgets must be initialized with a non-null owner object.");
                m_manifestVector = manifestVector;
                m_ownerObject = ownerObject;
                UpdatePropertyGrid();
            }

            ManifestVectorWidget::ManifestVectorType ManifestVectorWidget::GetManifestVector()
            {
                return m_manifestVector;
            }

            void ManifestVectorWidget::SetCollectionName(const AZStd::string& name)
            {
                m_ui->m_containerTitle->setText(name.c_str());
            }

            void ManifestVectorWidget::SetCapSize(size_t cap)
            {
                m_capSize = cap;
            }

            void ManifestVectorWidget::SetCollectionTypeName(const AZStd::string& typeName)
            {
                QString addString = QString("Add ") + typeName.c_str();
                m_ui->m_addObjectButton->setText(addString);
            }

            bool ManifestVectorWidget::ContainsManifestObject(const DataTypes::IManifestObject* object) const
            {
                for (auto& containedObject : m_manifestVector)
                {
                    if (containedObject.get() == object)
                    {
                        return true;
                    }
                }

                return false;
            }

            bool ManifestVectorWidget::RemoveManifestObject(const DataTypes::IManifestObject* object)
            {
                AZ_TraceContext("Remove object type", object->RTTI_GetTypeName());

                for (auto it = m_manifestVector.begin(); it < m_manifestVector.end(); ++it)
                {
                    if ((*it).get() == object)
                    {
                        object->OnUserRemoved();
                        m_manifestVector.erase(it);
                        QTimer::singleShot(0, this, 
                            [this]()
                            {
                                UpdatePropertyGrid();
                                EmitObjectChanged(m_ownerObject);
                            });
                        return true;
                    }
                }

                AZ_TracePrintf(Utilities::WarningWindow, "Tried to remove an object that was not contained in the vector.");

                return false;
            }

            void ManifestVectorWidget::DisplayAddPrompt()
            {
                Events::ManifestMetaInfo::ModifiersList availableManifestUUIDs;
                ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "ManifestVectorWidget is not a child of a ManifestWidget.");
                if (!root)
                {
                    return;
                }
                AZStd::shared_ptr<Containers::Scene> scene = root->GetScene();
                if (!scene)
                {
                    return;
                }

                EBUS_EVENT(Events::ManifestMetaInfoBus, GetAvailableModifiers, availableManifestUUIDs, *scene, *m_ownerObject);

                AZ_TraceContext("Parent manifest object type", m_ownerObject->RTTI_GetTypeName());

                QMenu* objectMenu = m_ui->m_addObjectButton->menu();
                objectMenu->clear();
                
                for (auto& manifestUUID : availableManifestUUIDs)
                {
                    const AZ::SerializeContext::ClassData* manifestClassData = m_serializeContext->FindClassData(manifestUUID);

                    AZ_TraceContext("Child manifest object UUID", manifestUUID.ToString<AZStd::string>());

                    if (!manifestClassData)
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Class data was not registered for class, it will not be available as an option");
                        continue;
                    }

                    AZStd::string displayName;
                    if (manifestClassData->m_editData && manifestClassData->m_editData->m_name[0] != '\0')
                    {
                        displayName = manifestClassData->m_editData->m_name;
                    }
                    else if(manifestClassData->m_name[0] != '\0')
                    {
                        displayName = manifestClassData->m_name;
                    }
                    else
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Class data did not contain a human readable name for class, it will not be available as an option");
                        continue;
                    }

                    QAction* objectCreateAction = new QAction(displayName.c_str(), m_ui->m_addObjectButton);
                    connect(objectCreateAction, &QAction::triggered, this,
                        [this, manifestClassData, displayName]() 
                        {
                            this->AddNewObject(manifestClassData->m_factory, displayName); 
                        });
                    objectMenu->addAction(objectCreateAction);
                }
            }

            void ManifestVectorWidget::AddNewObject(SerializeContext::IObjectFactory* factory, const AZStd::string& objectName)
            {
                if (m_manifestVector.size() >= m_capSize)
                {
                    QMessageBox::warning(this, "Cap reached", QString("The %1 container reached its cap of %2 entries.\nPlease remove entries to free up space.").
                        arg(m_ui->m_containerTitle->text()).arg(m_capSize));
                    return;
                }
                
                AZ_TraceContext("Object name", objectName);
                AZStd::shared_ptr<DataTypes::IManifestObject> newObject(static_cast<DataTypes::IManifestObject*>(factory->Create(objectName.c_str())));
                if (newObject)
                {
                    newObject->OnUserAdded();
                }
                ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "ManifestVectorWidget is not a child of a ManifestWidget.");
                if (!root)
                {
                    return;
                }
                
                AZ_TraceContext("Object type", newObject->RTTI_GetTypeName());
                AZStd::shared_ptr<Containers::Scene> scene = root->GetScene();
                EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, *scene, *newObject);
                
                m_manifestVector.push_back(newObject);
                UpdatePropertyGrid();
                EmitObjectChanged(m_ownerObject);
            }

            void ManifestVectorWidget::UpdatePropertyGrid()
            {
                QSignalBlocker(this);
                m_propertyEditor->ClearInstances();
                for (auto &object : m_manifestVector)
                {
                    if(object)
                    {
                        m_propertyEditor->AddInstance(object.get(), object->RTTI_GetType());
                    }
                }
                m_propertyEditor->InvalidateAll();
                m_propertyEditor->ExpandAll();
            }

            void ManifestVectorWidget::EmitObjectChanged(const DataTypes::IManifestObject* object)
            {
                emit valueChanged();

                ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "ManifestVectorWidget is not a child of a ManifestWidget.");
                if (!root)
                {
                    return;
                }
                AZStd::shared_ptr<Containers::Scene> scene = root->GetScene();
                if (!scene)
                {
                    return;
                }

                Events::ManifestMetaInfoBus::Broadcast(&Events::ManifestMetaInfoBus::Events::ObjectUpdated, *scene, object, this);
            }

            void ManifestVectorWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* node)
            {
                // The immediate parent may not have the manifest object, so check the full ancestry.
                while (node && node->GetParent())
                {
                    AzToolsFramework::InstanceDataNode* owner = node->GetParent();
                    const AZ::SerializeContext::ClassData* classData = owner->GetClassMetadata();
                    if (classData && classData->m_azRtti)
                    {
                        const DataTypes::IManifestObject* cast = classData->m_azRtti->Cast<DataTypes::IManifestObject>(owner->FirstInstance());
                        if (cast)
                        {
                            AZ_Assert(AZStd::find_if(m_manifestVector.begin(), m_manifestVector.end(),
                                [cast](const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
                                {
                                    return object.get() == cast;
                                }) != m_manifestVector.end(), "ManifestVectorWidget detected an update of a field it doesn't own.");
                            EmitObjectChanged(cast);
                            return;
                        }
                    }
                    node = node->GetParent();
                }
            }

            void ManifestVectorWidget::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* /*node*/, const QPoint& /*point*/)
            {
            }

            void ManifestVectorWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
            {
            }

            void ManifestVectorWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
            {
            }

            void ManifestVectorWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/)
            {
            }

            void ManifestVectorWidget::SealUndoStack()
            {
            }

            void ManifestVectorWidget::ObjectUpdated([[maybe_unused]] const Containers::Scene& scene, const DataTypes::IManifestObject* target, void* sender)
            {
                if (sender != this && target != nullptr && m_propertyEditor)
                {
                    if (AZStd::find_if(m_manifestVector.begin(), m_manifestVector.end(),
                        [target](const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
                        {
                            return object.get() == target;
                        }) != m_manifestVector.end())
                    {
                        m_propertyEditor->InvalidateAttributesAndValues();
                    }
                }
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <RowWidgets/moc_ManifestVectorWidget.cpp>
