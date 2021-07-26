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
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/RowWidgets/HeaderWidget.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>

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
            AZ_CLASS_ALLOCATOR_IMPL(HeaderWidget, SystemAllocator, 0)

            HeaderWidget::HeaderWidget(QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::HeaderWidget())
                , m_target(nullptr)
                , m_nameIsEditable(false)
                , m_sceneManifest(nullptr)
            {
                InitSceneUIHeaderWidgetResources();

                ui->setupUi(this);

                ui->m_icon->hide();
                
                ui->m_deleteButton->setIcon(QIcon(":/PropertyEditor/Resources/cross-small.png"));
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
                SetIcon(target);
            }

            const DataTypes::IManifestObject* HeaderWidget::GetManifestObject() const
            {
                return m_target;
            }

            void HeaderWidget::DeleteObject()
            {
                AZ_TraceContext("Delete target", GetSerializedName(m_target));

                if (m_sceneManifest)
                {
                    Containers::SceneManifest::Index index = m_sceneManifest->FindIndex(m_target);
                    if (index != Containers::SceneManifest::s_invalidIndex)
                    {
                        AZ_TraceContext("Manifest index", static_cast<int>(index));
                        ManifestWidget* root = ManifestWidget::FindRoot(this);
                        // The manifest object could be a root element at the manifest page level so it needs to be
                        //      removed from there as well in that case.
                        if (root->RemoveObject(m_sceneManifest->GetValue(index)) && m_sceneManifest->RemoveEntry(m_target))
                        {
                            m_target = nullptr;
                            // Hide and disable the button so when users spam the delete button only a single click is recorded.
                            ui->m_deleteButton->hide();
                            ui->m_deleteButton->setEnabled(false);
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

            void HeaderWidget::SetIcon(const DataTypes::IManifestObject* target)
            {
                if (!target)
                {
                    return;
                }

                AZStd::string iconPath;
                EBUS_EVENT(Events::ManifestMetaInfoBus, GetIconPath, iconPath, *target);
                if (iconPath.empty())
                {
                    ui->m_icon->hide();
                }
                else
                {
                    ui->m_icon->setPixmap(QPixmap(iconPath.c_str()));
                    ui->m_icon->show();
                }
            }
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_HeaderWidget.cpp>
