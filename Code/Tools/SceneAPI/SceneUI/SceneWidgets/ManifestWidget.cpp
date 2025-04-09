/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QLabel>
#include <QPushButton>
#include <SceneWidgets/ui_ManifestWidget.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidgetPage.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphWidget.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            ManifestWidget::ManifestWidget(SerializeContext* serializeContext, QWidget* parent)
                : QWidget(parent)
                , ui(new Ui::ManifestWidget())
                , m_serializeContext(serializeContext)
            {
                ui->setupUi(this);
                ui->m_tabs->setOverflowButtonSpacing(true);
            }
            
            ManifestWidget::~ManifestWidget()
            {
            }

            void ManifestWidget::ResetScene()
            {
                ui->m_tabs->clear();
                m_pages.clear();
                m_scene.reset();
            }

            void ManifestWidget::BuildFromScene(const AZStd::shared_ptr<Containers::Scene>& scene)
            {
                AZ_PROFILE_FUNCTION(Editor);
                ui->m_tabs->clear();
                m_pages.clear();
                
                m_scene = scene;
                if (!scene)
                {
                    return;
                }

                BuildPages();

                Containers::SceneManifest& manifest = scene->GetManifest();
                for (auto& value : manifest.GetValueStorage())
                {
                    AddObject(value);
                }

                for (ManifestWidgetPage* page : m_pages)
                {
                    page->RefreshPage();
                }

                // Make sure to reset the active tab if the active tab is now empty
                ManifestWidgetPage* currentPage = qobject_cast<ManifestWidgetPage*>(ui->m_tabs->currentWidget());
                if (currentPage == nullptr || currentPage->ObjectCount() == 0)
                {
                    for (ManifestWidgetPage* page : m_pages)
                    {
                        if (page->ObjectCount() > 0)
                        {
                            ui->m_tabs->setCurrentWidget(page);
                            break;
                        }
                    }
                }
            }

            bool ManifestWidget::AddObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
            {
                AZ_PROFILE_FUNCTION(Editor);
                for (ManifestWidgetPage* page : m_pages)
                {
                    if (page->SupportsType(object))
                    {
                        return page->AddObject(object);
                    }
                }
                return false;
            }

            bool ManifestWidget::RemoveObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object)
            {
                for (ManifestWidgetPage* page : m_pages)
                {
                    if (page->SupportsType(object))
                    {
                        return page->RemoveObject(object);
                    }
                }
                return false;
            }

            AZStd::shared_ptr<Containers::Scene> ManifestWidget::GetScene()
            {
                return m_scene;
            }

            AZStd::shared_ptr<const Containers::Scene> ManifestWidget::GetScene() const
            {
                return m_scene;
            }

            ManifestWidget* ManifestWidget::FindRoot(QWidget* child)
            {
                while (child != nullptr)
                {
                    ManifestWidget* manifestWidget = qobject_cast<ManifestWidget*>(child);
                    if (manifestWidget)
                    {
                        return manifestWidget;
                    }
                    else
                    {
                        child = child->parentWidget();
                    }
                }
                return nullptr;
            }

            const ManifestWidget* ManifestWidget::FindRoot(const QWidget* child)
            {
                while (child != nullptr)
                {
                    const ManifestWidget* manifestWidget = qobject_cast<const ManifestWidget*>(child);
                    if (manifestWidget)
                    {
                        return manifestWidget;
                    }
                    else
                    {
                        child = child->parentWidget();
                    }
                }
                return nullptr;
            }

            void ManifestWidget::BuildPages()
            {
                if (!m_scene)
                {
                    return;
                }

                Events::ManifestMetaInfo::CategoryRegistrationList categories;
                EBUS_EVENT(Events::ManifestMetaInfoBus, GetCategoryAssignments, categories, *m_scene);

                AZStd::sort(categories.begin(), categories.end(), 
                    [](const Events::ManifestMetaInfo::CategoryRegistration& lhs, const Events::ManifestMetaInfo::CategoryRegistration& rhs)
                    {
                        return rhs.m_preferredOrder > lhs.m_preferredOrder;
                    }
                );

                AZStd::string currentCategory;
                AZStd::vector<AZ::Uuid> types;
                for (auto& category : categories)
                {
                    if (category.m_categoryName != currentCategory)
                    {
                        // Skip first occurrence.
                        if (!currentCategory.empty())
                        {
                            ManifestWidgetPage* page = new ManifestWidgetPage(m_serializeContext, AZStd::move(types));
                            AddPage(currentCategory.c_str(), page);
                        }
                        currentCategory = category.m_categoryName;
                        AZ_Assert(types.empty(), "Expecting vectors to be empty after being moved.");
                    }
                    types.push_back(category.m_categoryTargetGroupId);
                }
                // Add final page
                if (!currentCategory.empty())
                {
                    ManifestWidgetPage* page = new ManifestWidgetPage(m_serializeContext, AZStd::move(types));
                    AddPage(currentCategory.c_str(), page);
                }
            }

            void ManifestWidget::AddPage(const QString& category, ManifestWidgetPage* page)
            {
                m_pages.push_back(page);
                ui->m_tabs->addTab(page, category);
                connect(page, &ManifestWidgetPage::SaveClicked, this, &ManifestWidget::SaveClicked);
                connect(page, &ManifestWidgetPage::InspectClicked, this, &ManifestWidget::OnInspect);
                connect(page, &ManifestWidgetPage::ResetSettings, this, &ManifestWidget::OnSceneResetRequested);
                connect(page, &ManifestWidgetPage::ClearChanges, this, &ManifestWidget::OnClearUnsavedChangesRequested);
                connect(page, &ManifestWidgetPage::AssignScript, this, &ManifestWidget::OnAssignScript);
                connect(this, &ManifestWidget::AppendUnsavedChangesToTitle, page, &ManifestWidgetPage::AppendUnsavedChangesToTitle);
                connect(this, &ManifestWidget::EnableInspector, page, &ManifestWidgetPage::EnableInspector);
            }

            void ManifestWidget::SetInspectButtonVisibility(bool enableInspector)
            {
                emit EnableInspector(enableInspector);
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <SceneWidgets/moc_ManifestWidget.cpp>
