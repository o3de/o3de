/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/ComboBoxWidget.h>
#include <QLabel>
#include <QHBoxLayout>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Window/EffectorInspector.h>
#include <Document/ParticleDocumentBus.h>
#include <OpenParticleSystemEditor/Window/AssetWidget.h>
#include <Window/MaterialPropertyDialog.h>
#include <QMessageBox>

namespace OpenParticleSystemEditor
{
    ComboBoxWidget::ComboBoxWidget(
        const AZStd::string& className,
        OpenParticle::ParticleSourceData::DetailInfo* detail,
        AZ::SerializeContext* serializeContext,
        AzToolsFramework::IPropertyEditorNotify* pnotify,
        QWidget* parent)
        : QWidget(parent)
        , m_className(className)
        , m_parent(parent)
    {
        m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->SetHideRootProperties(false);
        m_propertyEditor->SetAutoResizeLabels(true);
        m_propertyEditor->SetValueComparisonFunction({});
        m_propertyEditor->SetSavedStateKey({});
        m_propertyEditor->Setup(serializeContext, pnotify, false);
        m_propertyEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        for (const auto& iterModule : detail->m_modules[m_className.data()])
        {
            if (iterModule.second.first == true)
            {
                m_propertyEditor->AddInstance(
                    AZStd::any_cast<void>(iterModule.second.second), iterModule.second.second->get_type_info().m_id, nullptr, nullptr);
                m_lastModuleName = iterModule.first;
                m_moduleName = iterModule.first;
                break;
            }
        }
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);

        m_locationClasses = { { "NA", tr("NA") }, { "Point", tr("Point") },
                              { "Box", tr("Box") }, { "Sphere", tr("Sphere") }, { "Torus", tr("Torus") },
                              { "Cylinder", tr("Cylinder") }, { "Mesh", tr("Mesh") } };
        m_rendererClasses = { { "NA", tr("NA") }, { "Sprite Renderer", tr("Sprite Renderer") },
                              { "Mesh Renderer", tr("Mesh Renderer") }, { "Ribbon Renderer", tr("Ribbon Renderer") } };

        InitComboBox();

        m_detail = detail;

        m_materialAssetWidget = new AssetWidget(MATERIAL_DESCRIPTION.toUtf8().data(), azrtti_typeid<AZ::RPI::MaterialAsset>(), this);
        m_materialPropertiesButton = new QPushButton(tr("Edit Material Properties..."), this);
        m_materialPropertiesButton->setToolTip(
            tr("Override material property values for this emitter only. Other emitters using the same "
               "material are unaffected."));
        connect(m_materialPropertiesButton, &QPushButton::clicked, this, &ComboBoxWidget::OpenMaterialPropertyDialog);
        m_clearOverridesButton = new QPushButton(tr("Clear Overrides"), this);
        m_clearOverridesButton->setToolTip(tr("Clear all material property overrides for this emitter."));
        connect(m_clearOverridesButton, &QPushButton::clicked, this, &ComboBoxWidget::ClearMaterialOverrides);
        m_modelAssetWidget = new AssetWidget(MESH_DESCRIPTION.toUtf8().data(), azrtti_typeid<AZ::RPI::ModelAsset>(), this);
        m_skeletonModelAssetWidget = new AssetWidget(MESH_DESCRIPTION.toUtf8().data(), azrtti_typeid<AZ::RPI::ModelAsset>(), this);
        
        SetAssetWidget(&detail->m_material, &detail->m_model, &detail->m_skeletonModel);
        SetAssetWidgetVisible();

        SetUI();
    }

    void ComboBoxWidget::InitComboBox()
    {
        m_comboBox = new QComboBox(this);
        m_comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        if (m_className == PARTICLE_LINE_NAMES[WIDGET_LINE_RENDERER])
        {
            m_nameLabel = new QLabel(RENDERER_LABEL);
            for (auto iter: m_rendererClasses)
            {
                m_comboBox->addItem(iter.second);
                if (m_moduleName == iter.first)
                {
                    m_comboBox->setCurrentText(iter.second);
                }
            }
        }
        else if (m_className == PARTICLE_LINE_NAMES[WIDGET_LINE_LOCATION])
        {
            m_nameLabel = new QLabel(SHAPE_LABEL);
            for (auto iter : m_locationClasses)
            {
                m_comboBox->addItem(iter.second);
                if (m_moduleName == iter.first)
                {
                    m_comboBox->setCurrentText(iter.second);
                }
            }
        }

        m_nameLabel->setAlignment(Qt::AlignLeft);
        m_nameLabel->setMinimumWidth(0);
        connect(m_comboBox, &QComboBox::currentTextChanged, this, &ComboBoxWidget::OnIndexChanged);
    }

    void ComboBoxWidget::SetUI()
    {
        m_layout = new QVBoxLayout(this);
        QHBoxLayout* hLayout = new QHBoxLayout(this);
        hLayout->addWidget(m_nameLabel);
        hLayout->addWidget(m_comboBox);
        m_layout->addLayout(hLayout);
        m_layout->addWidget(m_propertyEditor);
        m_layout->addWidget(m_materialAssetWidget);
        QHBoxLayout* materialButtonLayout = new QHBoxLayout();
        materialButtonLayout->addWidget(m_materialPropertiesButton, 1);
        materialButtonLayout->addWidget(m_clearOverridesButton);
        m_layout->addLayout(materialButtonLayout);
        m_layout->addWidget(m_modelAssetWidget);
        m_layout->addWidget(m_skeletonModelAssetWidget);

        setLayout(m_layout);
    }

    void ComboBoxWidget::OnIndexChanged([[maybe_unused]] const QString& curString)
    {
        EffectorInspector* inspector = dynamic_cast<EffectorInspector*>(m_parent);
        if (inspector != nullptr)
        {
            m_lastModuleName = m_moduleName;

            if (m_className == PARTICLE_LINE_NAMES[WIDGET_LINE_LOCATION])
            {
                m_moduleName = m_locationClasses[m_comboBox->currentIndex()].first;
            }
            else if (m_className == PARTICLE_LINE_NAMES[WIDGET_LINE_RENDERER])
            {
                m_moduleName = m_rendererClasses[m_comboBox->currentIndex()].first;
            }
            else
            {
                m_moduleName = NOT_AVAILABLE;
            }
            
            inspector->UpdataComboBoxWidget(m_moduleName, m_className, m_lastModuleName);
        }
    }

    void ComboBoxWidget::Clear()
    {
        m_layout->removeWidget(m_propertyEditor);
        m_layout->removeWidget(m_nameLabel);
        m_layout->removeWidget(m_comboBox);
        m_layout->removeWidget(m_materialAssetWidget);
        // The two material buttons live in the nested materialButtonLayout, not in m_layout, so there is
        // nothing to remove here - deleting them below detaches them from their own layout.
        m_layout->removeWidget(m_modelAssetWidget);
        m_layout->removeWidget(m_skeletonModelAssetWidget);
        m_propertyEditor->deleteLater();

        delete m_nameLabel;
        m_nameLabel = nullptr;

        delete m_comboBox;
        m_comboBox = nullptr;

        m_propertyEditor->ClearInstances();
        delete m_propertyEditor;
        m_propertyEditor = nullptr;

        delete m_materialAssetWidget;
        m_materialAssetWidget = nullptr;

        delete m_materialPropertiesButton;
        m_materialPropertiesButton = nullptr;

        delete m_clearOverridesButton;
        m_clearOverridesButton = nullptr;

        // The dialog holds a DetailInfo* owned by the source data, so it must not outlive this widget.
        if (m_materialPropertyDialog != nullptr)
        {
            m_materialPropertyDialog->close();
            delete m_materialPropertyDialog;
            m_materialPropertyDialog = nullptr;
        }

        m_detail = nullptr;

        delete m_modelAssetWidget;
        m_modelAssetWidget = nullptr;

        delete m_skeletonModelAssetWidget;
        m_skeletonModelAssetWidget = nullptr;
    }

    void ComboBoxWidget::Refresh(AZStd::any* instance)
    {
        m_propertyEditor->ClearInstances();
        m_propertyEditor->AddInstance(AZStd::any_cast<void>(instance), instance->get_type_info().m_id, nullptr, nullptr);
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
        SetAssetWidgetVisible();
    }

    void ComboBoxWidget::Refresh()
    {
        m_propertyEditor->ClearInstances();
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
        SetAssetWidgetVisible();
    }

    void ComboBoxWidget::SetAssetWidget(
        AZ::Data::Asset<AZ::RPI::MaterialAsset>* materialAsset,
        AZ::Data::Asset<AZ::RPI::ModelAsset>* modelAsset,
        AZ::Data::Asset<AZ::RPI::ModelAsset>* skeletonModelAsset)
    {
        EffectorInspector* inspector = dynamic_cast<EffectorInspector*>(m_parent);
        AZStd::string busWidgetName = "";
        if (inspector != nullptr)
        {
            busWidgetName = inspector->m_widgetName;
        }

        
        m_materialAssetWidget->SetAssetId(materialAsset->GetId());
        m_modelAssetWidget->SetAssetId(modelAsset->GetId());
        m_skeletonModelAssetWidget->SetAssetId(skeletonModelAsset->GetId());

        m_materialAssetWidget->SetOnAssetSelectionChangedCallback(
            [this, busWidgetName, materialAsset](AZ::Data::AssetId newId)
            {
                *materialAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(newId, AZ::Data::AssetLoadBehavior::PreLoad);
                // OnMaterialChanged syncs the detail onto the emitter and prunes overrides that the new
                // material does not have, so the panel is rebuilt after that rather than before.
                Q_EMIT(OnMaterialChanged());
                RefreshMaterialProperties();
                EBUS_EVENT_ID(busWidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            });

         m_modelAssetWidget->SetOnAssetSelectionChangedCallback(
            [this, busWidgetName, modelAsset](AZ::Data::AssetId newId)
            {
                *modelAsset =
                    AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(newId, AZ::Data::AssetLoadBehavior::PreLoad);
                Q_EMIT(OnModelChanged());
                EBUS_EVENT_ID(busWidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            });

         m_skeletonModelAssetWidget->SetOnAssetSelectionChangedCallback(
             [this, busWidgetName, skeletonModelAsset](AZ::Data::AssetId newId)
            {
                 *skeletonModelAsset =
                     AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(newId, AZ::Data::AssetLoadBehavior::PreLoad);
                 Q_EMIT(OnSkeletonModelChanged());
                 EBUS_EVENT_ID(busWidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            });
    }

    void ComboBoxWidget::RefreshMaterialProperties()
    {
        // Guard every pointer this touches, not just one of them: Clear() tears the widgets down one at a
        // time, so checking a single member would leave a window where it is still set and the others are not.
        if (m_materialPropertiesButton == nullptr || m_clearOverridesButton == nullptr || m_materialAssetWidget == nullptr)
        {
            return;
        }

        // Only offer property overrides where a material actually applies. isHidden rather than isVisible:
        // this runs during construction, before the widget has a parent layout, and isVisible would report
        // false for everything at that point.
        const bool hasMaterial =
            m_detail != nullptr && !m_materialAssetWidget->isHidden() && m_detail->m_material.GetId().IsValid();

        m_materialPropertiesButton->setVisible(hasMaterial);

        m_clearOverridesButton->setVisible(hasMaterial);
        UpdateClearOverridesButtonState();

        // Only touch the dialog if one is already open; this must not pop a window on its own.
        if (m_materialPropertyDialog != nullptr)
        {
            m_materialPropertyDialog->SetDetail(hasMaterial ? m_detail : nullptr);
        }
    }

    void ComboBoxWidget::OpenMaterialPropertyDialog()
    {
        if (m_detail == nullptr || !m_detail->m_material.GetId().IsValid())
        {
            return;
        }

        if (m_materialPropertyDialog == nullptr)
        {
            m_materialPropertyDialog = new MaterialPropertyDialog(this);
            connect(
                m_materialPropertyDialog, &MaterialPropertyDialog::OnMaterialPropertyChanged, this,
                [this](bool editingFinished)
                {
                    UpdateClearOverridesButtonState();
                    Q_EMIT OnMaterialPropertiesChanged(editingFinished);
                });
        }

        m_materialPropertyDialog->SetDetail(m_detail);
        m_materialPropertyDialog->show();
        m_materialPropertyDialog->raise();
        m_materialPropertyDialog->activateWindow();
    }

    void ComboBoxWidget::UpdateClearOverridesButtonState()
    {
        if (m_clearOverridesButton != nullptr)
        {
            m_clearOverridesButton->setEnabled(m_detail != nullptr && !m_detail->m_materialOverrides.empty());
        }
    }

    void ComboBoxWidget::ClearMaterialOverrides()
    {
        if (m_detail == nullptr || m_detail->m_materialOverrides.empty())
        {
            return;
        }

        const int count = static_cast<int>(m_detail->m_materialOverrides.size());
        if (QMessageBox::question(
                this, tr("Clear Overrides"),
                tr("Remove all %1 material property override(s) from this emitter?").arg(count),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        // QMessageBox spins a nested event loop, so the emitter panel could have been torn down while it was
        // up. Re-check rather than trusting the pointer we validated before the prompt.
        if (m_detail == nullptr)
        {
            return;
        }

        m_detail->m_materialOverrides.clear();

        // Refresh the open inspector before notifying: the notification rebuilds the particle asset, and the
        // dialog holds a DetailInfo* we want re-read while we still know it is valid.
        if (m_materialPropertyDialog != nullptr)
        {
            m_materialPropertyDialog->SetDetail(m_detail);
        }
        UpdateClearOverridesButtonState();

        // Same path a property edit takes: syncs the (now empty) map onto the emitter and marks the doc modified.
        Q_EMIT OnMaterialPropertiesChanged(true);
    }

    void ComboBoxWidget::SetAssetWidgetVisible()
    {
        m_materialAssetWidget->setVisible(false);
        m_modelAssetWidget->setVisible(false);
        m_skeletonModelAssetWidget->setVisible(false);
        if (m_moduleName.c_str() == m_locationClasses[SKELETON_INDEX].first)
        {
            m_skeletonModelAssetWidget->setVisible(true);
        }
        else if (m_moduleName.c_str() == m_rendererClasses[SPRITE_RENDERER_INDEX].first
            || m_moduleName.c_str() == m_rendererClasses[RIBBON_RENDERER_INDEX].first)
        {
            m_materialAssetWidget->setVisible(true);
        }
        else if (IsMeshRenderer())
        {
            m_materialAssetWidget->setVisible(true);
            m_modelAssetWidget->setVisible(true);
        }

        RefreshMaterialProperties();
    }

    bool ComboBoxWidget::IsMeshRenderer() const
    {
        return m_moduleName.c_str() == m_rendererClasses[MESH_RENDERER_INDEX].first;
    }
} // namespace OpenParticleSystemEditor
