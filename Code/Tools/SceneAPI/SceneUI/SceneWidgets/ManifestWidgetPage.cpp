/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QDesktopServices>
#include <QMenu>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QMessageBox>
#include <QUrl>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <SceneWidgets/ui_ManifestWidgetPage.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IUnmodifiableRule.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidgetPage.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            ManifestWidgetPage::ManifestWidgetPage(SerializeContext* context, AZStd::vector<AZ::Uuid>&& classTypeIds)
                : m_classTypeIds(AZStd::move(classTypeIds))
                , ui(new Ui::ManifestWidgetPage())
                , m_propertyEditor(nullptr)
                , m_context(context)
                , m_capSize(100)
            {
                ui->setupUi(this);

                m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(nullptr);
                m_propertyEditor->Setup(context, this, true, 250);

                m_propertyEditor->SetReadOnlyQueryFunction(
                    [this](const AzToolsFramework::InstanceDataNode* node)
                    {
                        return SetNodeReadOnlyStatus(node);
                    });

                ui->m_mainLayout->insertWidget(0, m_propertyEditor);

                BuildAndConnectAddButton();

                BuildHelpButton();

                m_editMenu = new QMenu("Edit Scene Settings Menu", ui->m_editButton);
                ui->m_editButton->setMenu(m_editMenu);

                connect(m_editMenu, &QMenu::aboutToShow, this, &ManifestWidgetPage::AddEditMenu);

                connect(ui->m_saveButton, &QPushButton::clicked, this, &ManifestWidgetPage::SaveClicked);
                connect(ui->m_inspectButton, &QPushButton::clicked, this, &ManifestWidgetPage::InspectClicked);

                BusConnect();
            }

            ManifestWidgetPage::~ManifestWidgetPage()
            {
                BusDisconnect();
            }

            void ManifestWidgetPage::SetCapSize(size_t size)
            {
                m_capSize = size;
            }

            size_t ManifestWidgetPage::GetCapSize() const
            {
                return m_capSize;
            }

            bool ManifestWidgetPage::SupportsType(const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
            {
                for (Uuid& id : m_classTypeIds)
                {
                    if (object->RTTI_IsTypeOf(id))
                    {
                        return true;
                    }
                }
                return false;
            }

            bool ManifestWidgetPage::AddObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
            {
                AZ_PROFILE_FUNCTION(Editor);
                if (!SupportsType(object))
                {
                    return false;
                }
                if (!m_propertyEditor->AddInstance(object.get(), object->RTTI_GetType()))
                {
                    AZ_Assert(false, "Failed to add manifest object to Reflected Property Editor.");
                    return false;
                }

                // Add new object to the list so it's ready for updating later on.
                m_objects.push_back(object);

                UpdateAddButtonStatus();

                // Guard against adding lots of ScrollToBottom calls in the case where we're performing bulk adds in a single frame.
                if (!m_scrollToBottomQueued)
                {
                    m_scrollToBottomQueued = true;
                    QTimer::singleShot(0, this,
                        [this]()
                        {
                            ScrollToBottom();
                        });
                }

                return true;
            }

            void ManifestWidgetPage::UpdateAddButtonStatus()
            {
                if (m_objects.size() >= m_capSize)
                {
                    QString entryString(tr(m_capSize == 1 ? "entry" : "entries"));
                    ui->m_addButton->setToolTip(tr("Maximum number of entries reached. This page can contain up to %1 %2.").arg(m_capSize).arg(entryString));
                    ui->m_addButton->setEnabled(false);
                }
                else
                {
                    ui->m_addButton->setToolTip(QString());
                    ui->m_addButton->setEnabled(true);
                }
            }

            bool ManifestWidgetPage::RemoveObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
            {
                if (SupportsType(object))
                {
                    // Explicitly keep a copy of the shared pointer to guarantee that the manifest object isn't
                    //      deleted before it can be queued for the delete deletion.
                    AZStd::shared_ptr<DataTypes::IManifestObject> temp = object;
                    (void)temp;

                    auto it = AZStd::find(m_objects.begin(), m_objects.end(), object);
                    if (it == m_objects.end())
                    {
                        AZ_Assert(false, "Manifest object not part of manifest page.");
                        return false;
                    }

                    m_objects.erase(it);

                    UpdateAddButtonStatus();

                    if (m_objects.size() == 0)
                    {
                        // We won't get property modified event  if it's the last element removed
                        EmitObjectChanged();
                    }

                    // If the property editor is immediately updated here QT will do some processing in an unexpected order,
                    //      leading to heap corruption. To avoid this, keep a cached version of the deleted object and
                    //      delay the rebuilding of the property editor to the end of the update cycle.
                    QTimer::singleShot(0, this,
                        [this, object]()
                        {
                            // Explicitly keep a copy of the shared pointer to guarantee that the manifest object isn't
                            //      deleted between updates of QT.
                            (void)object;

                            m_propertyEditor->ClearInstances();
                            for (auto& instance : m_objects)
                            {
                                if (!m_propertyEditor->AddInstance(instance.get(), instance->RTTI_GetType()))
                                {
                                    AZ_Assert(false, "Failed to add manifest object to Reflected Property Editor.");
                                }
                            }
                            RefreshPage();
                        }
                    );

                    return true;
                }
                else
                {
                    return false;
                }
            }

            size_t ManifestWidgetPage::ObjectCount() const
            {
                return m_objects.size();
            }


            void ManifestWidgetPage::Clear()
            {
                m_objects.clear();
                m_propertyEditor->ClearInstances();
                UpdateAddButtonStatus();
            }

            void ManifestWidgetPage::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*pNode*/)
            {
            }

            void ManifestWidgetPage::AfterPropertyModified(AzToolsFramework::InstanceDataNode* node)
            {
                if (node)
                {
                    node = node->GetParent();
                    while (node)
                    {
                        if (const AZ::SerializeContext::ClassData* classData = node->GetClassMetadata(); classData && classData->m_azRtti)
                        {
                            if (const DataTypes::IManifestObject* cast = classData->m_azRtti->Cast<DataTypes::IManifestObject>(node->FirstInstance()); cast)
                            {
                                AZ_Assert(AZStd::find_if(m_objects.begin(), m_objects.end(), 
                                    [cast](const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
                                    {
                                        return object.get() == cast;
                                    }) != m_objects.end(), "ManifestWidgetPage detected an update of a field it doesn't own.");
                                EmitObjectChanged(cast);
                                break;
                            }
                        }
                        node = node->GetParent();
                    }
                }
            }

            void ManifestWidgetPage::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*pNode*/)
            {
            }
            
            void ManifestWidgetPage::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*pNode*/)
            {
            }

            void ManifestWidgetPage::SealUndoStack()
            {
            }

            void ManifestWidgetPage::ScrollToBottom()
            {
                m_scrollToBottomQueued = false;

                QScrollArea* propertyGridScrollArea = m_propertyEditor->findChild<QScrollArea*>();
                if (propertyGridScrollArea)
                {
                    propertyGridScrollArea->verticalScrollBar()->setSliderPosition(propertyGridScrollArea->verticalScrollBar()->maximum());
                }
            }

            void ManifestWidgetPage::RefreshPage()
            {
                AZ_PROFILE_FUNCTION(Editor);
                m_propertyEditor->InvalidateAll();
                m_propertyEditor->ExpandAll();
            }

            void ManifestWidgetPage::OnSingleGroupAdd()
            {
                if (m_classTypeIds.size() > 0)
                {
                    if (m_objects.size() >= m_capSize)
                    {
                        QMessageBox::warning(this, "Cap reached", QString("The group container reached its cap of %1 entries.\nPlease remove groups to free up space.").
                            arg(m_capSize));
                        return;
                    }

                    AddNewObject(m_classTypeIds[0]);

                    UpdateAddButtonStatus();
                }
            }

            void ManifestWidgetPage::OnMultiGroupAdd(const Uuid& id)
            {
                if (m_objects.size() >= m_capSize)
                {
                    QMessageBox::warning(this, "Cap reached", QString("The group container reached its cap of %1 entries.\nPlease remove groups to free up space.").
                        arg(m_capSize));
                    return;
                }
                AddNewObject(id);

                UpdateAddButtonStatus();
            }

            void ManifestWidgetPage::BuildAndConnectAddButton()
            {
                if (m_classTypeIds.size() == 0)
                {
                    ui->m_addButton->setText("No types for this group");
                }
                else if (m_classTypeIds.size() == 1)
                {
                    AZStd::string className = ClassIdToName(m_classTypeIds[0]);

                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(
                        serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    AZ_Assert(serializeContext, "No serialize context");
                    auto classData = serializeContext->FindClassData(m_classTypeIds[0]);
                    if (classData && classData->m_editData)
                    {
                        const AZ::Edit::ElementData* editorElementData =
                            classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Max))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<int>*>(categoryAttribute))
                            {
                                m_capSize = categoryAttributeData->Get(nullptr);
                            }
                        }
                    }

                    ui->m_addButton->setText(QString::fromLatin1("Add %1").arg(className.c_str()));
                    connect(ui->m_addButton, &QPushButton::clicked, this, &ManifestWidgetPage::OnSingleGroupAdd);
                }
                else
                {
                    QMenu* menu = new QMenu();
                    AZStd::vector<AZStd::string> classNames;
                    for (Uuid& id : m_classTypeIds)
                    {
                        AZStd::string className = ClassIdToName(id);
                        
                        menu->addAction(className.c_str(), 
                            [this, id]()
                            {
                                OnMultiGroupAdd(id); 
                            }
                        );

                        AZStd::to_lower(className.begin(), className.end());
                        classNames.push_back(className);
                    }

                    connect(menu, &QMenu::aboutToShow, this,
                        [this, menu]() 
                        {
                            menu->setFixedWidth(ui->m_addButton->width());
                        }
                    );

                    ui->m_addButton->setMenu(menu);

                    AZStd::string buttonText = "Add ";
                    AzFramework::StringFunc::Join(buttonText, classNames.begin(), classNames.end(), " or ");
                    ui->m_addButton->setText(buttonText.c_str());
                }
            }

            void ManifestWidgetPage::BuildHelpButton()
            {
                // Default to the root scene settings page, this is used when:
                //  * There are no groups available to add.
                //  * There are multiple groups available to add.
                //  * The group to add does not have a help URL set.
                //  * There is an issue retrieving the help URL from the group.
                m_helpUrl = "https://www.o3de.org/docs/user-guide/assets/scene-settings/";

                if (m_classTypeIds.size() == 1)
                {
                    const SerializeContext::ClassData* classData = m_context->FindClassData(m_classTypeIds[0]);
                    if (classData && classData->m_editData)
                    {
                        const AZ::Edit::ElementData* editorElementData =
                            classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::HelpPageURL))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                            {
                                const DataTypes::IGroup* sceneNodeGroup = nullptr;
                                AZStd::string urlValue = categoryAttributeData->Get(&sceneNodeGroup);
                                if (!urlValue.empty())
                                {
                                    m_helpUrl = urlValue.c_str();
                                }
                            }
                        }
                    }
                }

                connect(ui->m_helpButton, &QPushButton::clicked, this, &ManifestWidgetPage::OnHelpButtonClicked);
            }

            void ManifestWidgetPage::OnHelpButtonClicked()
            {
                QDesktopServices::openUrl(QUrl(m_helpUrl));
            }

            AZStd::string ManifestWidgetPage::ClassIdToName(const Uuid& id) const
            {
                static const AZStd::string s_groupSuffix = "group";

                const SerializeContext::ClassData* classData = m_context->FindClassData(id);
                if (!classData)
                {
                    return "<type not registered>";
                }

                AZStd::string className;
                if (classData->m_editData)
                {
                    className = classData->m_editData->m_name;
                }
                else
                {
                    className = classData->m_name;
                }

                // Get rid of "Group" suffix and all trailing whitespace (e.g. "mesh  group" -> "mesh")
                if (className.length() > s_groupSuffix.length())
                {
                    size_t potentialSuffixOffset = className.length() - s_groupSuffix.length();
                    if (AzFramework::StringFunc::Equal(className.c_str() + potentialSuffixOffset, s_groupSuffix.c_str()))
                    {
                        AzFramework::StringFunc::LKeep(className, potentialSuffixOffset - 1);
                        AzFramework::StringFunc::Strip(className, ' ');
                    }
                }

                return className;
            }

            void ManifestWidgetPage::AddNewObject(const Uuid& id)
            {
                AZ_TraceContext("Instance id", id);
                
                const SerializeContext::ClassData* classData = m_context->FindClassData(id);
                AZ_Assert(classData, "Type not registered.");
                if (classData)
                {
                    AZ_TraceContext("Object Type", classData->m_name);

                    AZ_Assert(classData->m_factory, "Registered type has no factory to create a new instance with.");
                    if (classData->m_factory)
                    {
                        ManifestWidget* parent = ManifestWidget::FindRoot(this);
                        AZ_Assert(parent, "ManifestWidgetPage isn't docked in a ManifestWidget.");
                        if (!parent)
                        {
                            return;
                        }

                        AZStd::shared_ptr<Containers::Scene> scene = parent->GetScene();
                        if (!scene)
                        {
                            return;
                        }
                        Containers::SceneManifest& manifest = scene->GetManifest();

                        void* rawInstance = classData->m_factory->Create(classData->m_name);
                        AZ_Assert(rawInstance, "Serialization factory failed to construct new instance.");
                        if (!rawInstance)
                        {
                            return;
                        }

                        AZStd::shared_ptr<DataTypes::IManifestObject> instance(reinterpret_cast<DataTypes::IManifestObject*>(rawInstance));
                        EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, *scene, *instance);
                        
                        if (!manifest.AddEntry(instance))
                        {
                            AZ_Assert(false, "Unable to add new object to manifest.");
                        }

                        if (!AddObject(instance))
                        {
                            AZ_Assert(false, "Unable to add new object to Reflected Property Editor.");
                        }
                        // Refresh the page after adding this new object.
                        RefreshPage();

                        EmitObjectChanged();
                    }
                }
            }

            void ManifestWidgetPage::EmitObjectChanged(const DataTypes::IManifestObject* object)
            {
                ManifestWidget* parent = ManifestWidget::FindRoot(this);
                AZ_Assert(parent, "ManifestWidgetPage isn't docked in a ManifestWidget.");
                if (!parent)
                {
                    return;
                }

                AZStd::shared_ptr<Containers::Scene> scene = parent->GetScene();
                if (!scene)
                {
                    return;
                }

                Events::ManifestMetaInfoBus::Broadcast(&Events::ManifestMetaInfoBus::Events::ObjectUpdated, *scene, object, this);
            }

            void ManifestWidgetPage::ObjectUpdated([[maybe_unused]] const Containers::Scene& scene, const DataTypes::IManifestObject* target, void* sender)
            {
                if (sender != this && target != nullptr && m_propertyEditor)
                {
                    if (AZStd::find_if(m_objects.begin(), m_objects.end(),
                        [target](const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
                        {
                            return object.get() == target;
                        }) != m_objects.end())
                    {
                        m_propertyEditor->InvalidateAttributesAndValues();
                    }
                }
            }

            void ManifestWidgetPage::AddObjects(AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>& objects)
            {
                ManifestWidget* parent = ManifestWidget::FindRoot(this);
                AZ_Error(SceneAPI::Utilities::ErrorWindow, parent, "ManifestWidgetPage isn't docked in a ManifestWidget.");
                if (!parent)
                {
                    return;
                }
                AZStd::shared_ptr<Containers::Scene> scene = parent->GetScene();
                if (!scene)
                {
                    return;
                }
                Containers::SceneManifest& manifest = scene->GetManifest();
                for (auto& object : objects)
                {
                    if (!SupportsType(object))
                    {
                        continue;
                    }
                    if (!manifest.AddEntry(object))
                    {
                        AZ_Error(SceneAPI::Utilities::ErrorWindow, false, "Unable to add new object to manifest.");
                    }
                    else
                    {
                        AddObject(object);
                    }
                }
                RefreshPage();
            }

            bool ManifestWidgetPage::SetNodeReadOnlyStatus(const AzToolsFramework::InstanceDataNode* node)
            {
                if (AzToolsFramework::InstanceDataNode* parentNode = node ? node->GetRoot() : nullptr)
                {
                    AZ::SceneAPI::DataTypes::IGroup* group = m_context->Cast<AZ::SceneAPI::DataTypes::IGroup*>(
                        parentNode->FirstInstance(), parentNode->GetClassMetadata()->m_typeId);
                    // If this group is unmodifiable, that means it's read only.
                    if (group && group->GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::DataTypes::IUnmodifiableRule>())
                    {
                        return true;
                    }
                }
                return false;
            }

            void ManifestWidgetPage::AppendUnsavedChangesToTitle(bool hasUnsavedChanges)
            {
                QString title(ui->m_saveButton->text());

                if (hasUnsavedChanges && title.back() != "*")
                {
                    title.push_back("*");
                }
                else if (!hasUnsavedChanges && title.back() == "*")
                {
                    title.remove(title.size() - 1, 1);
                }
                ui->m_saveButton->setText(title);
            }

            void ManifestWidgetPage::AddEditMenu()
            {
                m_editMenu->clear();

                m_editMenu->addAction(
                    QObject::tr("Reset settings to default..."),
                    [this]()
                    {
                        emit ResetSettings();
                    });

                m_editMenu->addAction(
                    QObject::tr("Clear unsaved changes..."),
                    [this]()
                    {
                        emit ClearChanges();
                    });

                m_editMenu->addAction(
                    QObject::tr("Assign build script..."),
                    [this]()
                    {
                        emit AssignScript();
                    });
            }

            void ManifestWidgetPage::EnableInspector(bool enableInspector)
            {
                ui->m_inspectButton->setVisible(enableInspector);
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <SceneWidgets/moc_ManifestWidgetPage.cpp>
