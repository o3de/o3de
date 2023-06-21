/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QEvent>
#include <RowWidgets/ui_HeaderWidget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IUnmodifiableRule.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/RowWidgets/HeaderWidget.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>


#include <QFile>

static void InitSceneUIHeaderWidgetResources()
{
    Q_INIT_RESOURCE(Icons);
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(HeaderWidget, SystemAllocator);

            HeaderWidget::HeaderWidget(QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::HeaderWidget())
                , m_target(nullptr)
                , m_sceneManifest(nullptr)
            {
                InitSceneUIHeaderWidgetResources();

                ui->setupUi(this);

                ui->m_icon->hide();
                
                ui->m_deleteButton->setIcon(QIcon(":/stylesheet/img/close_small.svg"));
                connect(ui->m_deleteButton, &QToolButton::clicked, this, &HeaderWidget::DeleteObject);
                ui->m_deleteButton->hide();

                ManifestWidget* root = ManifestWidget::FindRoot(this);
                AZ_Assert(root, "HeaderWidget is not a child of the ManifestWidget");
                if (root)
                {
                    m_sceneManifest = &root->GetScene()->GetManifest();
                }

            }

            void HeaderWidget::SetManifestObject(const DataTypes::IManifestObject* target)
            {
                AZ_TraceContext("New target", GetSerializedName(target));
                m_target = target;
                ui->m_nameLabel->setText(GetSerializedName(target));
                
                UpdateDeletable();
                UpdateUIForManifestObject(target);
            }

            const DataTypes::IManifestObject* HeaderWidget::GetManifestObject() const
            {
                return m_target;
            }

            bool HeaderWidget::ModifyTooltip(QString& toolTipString)
            {
                if (!m_target)
                {
                    return false;
                }
                if (m_target->RTTI_IsTypeOf(DataTypes::IGroup::TYPEINFO_Uuid()))
                {
                    const DataTypes::IGroup* group = azrtti_cast<const DataTypes::IGroup*>(m_target);

                    const Containers::RuleContainer& rules = group->GetRuleContainerConst();
                    // Multiple rules might change the tooltip, so loop through all rules.
                    bool ruleChangedTooltip = false;
                    // Rules don't all have access to Qt
                    AZStd::string ruleTooltip;
                    for (size_t ruleIndex = 0; ruleIndex < rules.GetRuleCount(); ++ruleIndex)
                    {
                        if (rules.GetRule(ruleIndex)->ModifyTooltip(ruleTooltip))
                        {
                            ruleChangedTooltip = true;
                        }
                    }
                    if (ruleChangedTooltip)
                    {
                        toolTipString = QString("%1%2").arg(ruleTooltip.c_str()).arg(toolTipString);
                    }

                    return ruleChangedTooltip;
                }

                return false;
            }

            void HeaderWidget::DeleteObject()
            {
                AZ_TraceContext("Delete target", GetSerializedName(m_target));

                if (m_sceneManifest)
                {
                    Containers::SceneManifest::Index index = m_sceneManifest->FindIndex(m_target);
                    if (index != Containers::SceneManifest::s_invalidIndex)
                    {
                        ManifestWidget* root = ManifestWidget::FindRoot(this);

                        AZStd::vector<const AZ::SceneAPI::DataTypes::IManifestObject*> otherObjectsToRemove;
                        m_target->GetManifestObjectsToRemoveOnRemoved(otherObjectsToRemove, *m_sceneManifest);
                        // The manifest object could be a root element at the manifest page level so it needs to be
                        //      removed from there as well in that case.
                        if (root->RemoveObject(m_sceneManifest->GetValue(index)) && m_sceneManifest->RemoveEntry(m_target))
                        {
                            m_target = nullptr;
                            // Hide and disable the button so when users spam the delete button only a single click is recorded.
                            ui->m_deleteButton->hide();
                            ui->m_deleteButton->setEnabled(false);

                            for (auto* toRemove : otherObjectsToRemove)
                            {
                                index = m_sceneManifest->FindIndex(toRemove);
                                root->RemoveObject(m_sceneManifest->GetValue(index));
                                m_sceneManifest->RemoveEntry(toRemove);
                            }
                            return;
                        }
                        else
                        {
                            AZ_TracePrintf(Utilities::LogWindow, "Unable to delete manifest object from manifest.");
                        }
                    }
                }

                QObject* widget = this->parent();
                while (widget != nullptr)
                {
                    ManifestVectorWidget* manifestVectorWidget = qobject_cast<ManifestVectorWidget*>(widget);
                    if (manifestVectorWidget)
                    {
                        if (manifestVectorWidget->RemoveManifestObject(m_target))
                        {
                            m_target = nullptr;
                            // Hide and disable the button so when users spam the delete button only a single click is recorded.
                            ui->m_deleteButton->hide();
                            ui->m_deleteButton->setEnabled(false);
                        }
                        else
                        {
                            AZ_TracePrintf(Utilities::WarningWindow, "Parent collection did not contain this ManifestObject");
                        }

                        return;
                    }
                    widget = widget->parent();
                }

                AZ_TracePrintf(Utilities::ErrorWindow, "No parent valid parent collection found.");
            }

            void HeaderWidget::UpdateDeletable()
            {
                ui->m_deleteButton->hide();

                // If this widget has the unmodifiable rule, then this can't be deleted.
                // Even though the delete button would be disabled, it's even more clear it can't be deleted if it's not visible.
                if (m_target->RTTI_IsTypeOf(DataTypes::IGroup::TYPEINFO_Uuid()))
                {
                    const DataTypes::IGroup* sceneNodeGroup = azrtti_cast<const DataTypes::IGroup*>(m_target);
                    const Containers::RuleContainer& rules = sceneNodeGroup->GetRuleContainerConst();
                    if (rules.FindFirstByType<AZ::SceneAPI::DataTypes::IUnmodifiableRule>())
                    {
                        // This header is unmodifiable, so leave the delete button hidden.
                        return;
                    }
                }

                if (m_sceneManifest)
                {
                    Containers::SceneManifest::Index index = m_sceneManifest->FindIndex(m_target);
                    if (index != Containers::SceneManifest::s_invalidIndex)
                    {
                        ui->m_deleteButton->show();
                        return;
                    }
                }

                QObject* widget = this->parent();
                while(widget != nullptr)
                {
                    ManifestVectorWidget* manifestVectorWidget = qobject_cast<ManifestVectorWidget*>(widget);
                    if (manifestVectorWidget && manifestVectorWidget->ContainsManifestObject(m_target))
                    {
                        ui->m_deleteButton->show();
                        break;
                    }
                    widget = widget->parent();
                }
            }

            const char* HeaderWidget::GetSerializedName(const DataTypes::IManifestObject* target) const
            {
                SerializeContext* context = nullptr;
                EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
                if (context)
                {
                    const SerializeContext::ClassData* classData = context->FindClassData(target->RTTI_GetType());
                    if (classData)
                    {
                        if (classData->m_editData)
                        {
                            return classData->m_editData->m_name;
                        }
                        return classData->m_name;
                    }
                }
                return "<type not registered>";
            }

            void HeaderWidget::UpdateUIForManifestObject(const DataTypes::IManifestObject* target)
            {
                if (!target)
                {
                    return;
                }

                const AZ::Edit::ElementData* editorElementData = nullptr;
                const DataTypes::IGroup* sceneNodeGroup = nullptr;

                // Retrieve information from the edit context to figure out if this HeaderWidget
                // show have a visual divider, and potentially the icon.
                if (target->RTTI_IsTypeOf(DataTypes::IGroup::TYPEINFO_Uuid()))
                {
                    sceneNodeGroup = azrtti_cast<const DataTypes::IGroup*>(target);
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    AZ_Assert(serializeContext, "No serialize context");

                    auto classData = serializeContext->FindClassData(target->RTTI_GetType());
                    if (classData && classData->m_editData)
                    {
                        editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);


                        if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                            {
                                AZStd::string categoryAttributeValue = categoryAttributeData->Get(&sceneNodeGroup);
                                if (categoryAttributeValue.compare("display divider") == 0)
                                {
                                    setStyleSheet("QFrame, QLabel {margin-top: 0px; font: bold;}");
                                }
                            }
                        }
                    }
                }

                // First, see if there's an icon registered on the ManifestMetaInfoBus.
                AZStd::string iconPath;
                EBUS_EVENT(Events::ManifestMetaInfoBus, GetIconPath, iconPath, *target);

                // If there isn't, then attempt to retrieve it from the edit context.
                if (iconPath.empty() && editorElementData)
                {
                    // Search the edit context for an icon.
                    // It will have been reflected like:
                    //  ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/MeshCollider.svg")
                    if (auto iconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Icon))
                    {
                        if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
                        {
                            AZStd::string iconAttributeValue = iconAttributeData->Get(&sceneNodeGroup);
                            if (!iconAttributeValue.empty())
                            {
                                iconPath = AZStd::move(iconAttributeValue);
                                        
                                // The path is probably going to be relative to a scan directory,
                                // especially if this node was defined in a Gem.
                                // If a first check doesn't find the file, then see if the path can
                                // be resolved via the asset system, and pull an absolute path from there.
                                if (!QFile::exists(QString(iconPath.c_str())))
                                {
                                    AZStd::string iconFullPath;
                                    bool pathFound = false;
                                    using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
                                    AssetSysReqBus::BroadcastResult(
                                        pathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath,
                                        iconPath, iconFullPath);
                                    if (pathFound)
                                    {
                                        iconPath = AZStd::move(iconFullPath);
                                    }
                                }
                            }
                        }
                    }
                }

                if (iconPath.empty())
                {
                    ui->m_icon->hide();
                }
                else
                {
                    ui->m_icon->setPixmap(QIcon(iconPath.c_str()).pixmap(QSize(ui->m_icon->size())));
                    
                    ui->m_icon->show();
                }
            }
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_HeaderWidget.cpp>
