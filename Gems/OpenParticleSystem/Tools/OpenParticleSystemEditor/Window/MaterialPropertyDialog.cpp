/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/MaterialPropertyDialog.h>
#include <Window/MaterialPropertyWidget.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/string/string.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QLabel>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace OpenParticleSystemEditor
{
    MaterialPropertyDialog::MaterialPropertyDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Emitter Material Properties"));

        // Modeless with the standard min/max buttons so it can be resized and left open next to the viewport
        // while iterating. Qt::Window rather than the default dialog flags is what gives it the maximise box.
        setWindowFlags(Qt::Window);
        setModal(false);
        setSizeGripEnabled(true);
        resize(380, 560);

        m_heading = new QLabel(this);
        m_heading->setWordWrap(true);
        m_heading->setTextInteractionFlags(Qt::TextSelectableByMouse);

        m_propertyWidget = new MaterialPropertyWidget(this);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_heading);
        layout->addWidget(m_propertyWidget, 1);
        setLayout(layout);

        connect(
            m_propertyWidget, &MaterialPropertyWidget::OnMaterialPropertyChanged, this,
            [this](bool editingFinished)
            {
                Q_EMIT OnMaterialPropertyChanged(editingFinished);
            });
    }

    void MaterialPropertyDialog::SetDetail(OpenParticle::ParticleSourceData::DetailInfo* detail)
    {
        if (detail == nullptr || !detail->m_material.GetId().IsValid())
        {
            m_propertyWidget->SetDetail(nullptr);
            close();
            return;
        }

        m_propertyWidget->SetDetail(detail);
        UpdateHeading(detail);
    }

    void MaterialPropertyDialog::UpdateHeading(OpenParticle::ParticleSourceData::DetailInfo* detail)
    {
        AZStd::string materialPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            materialPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, detail->m_material.GetId());

        if (materialPath.empty())
        {
            materialPath = detail->m_material.GetHint();
        }

        setWindowTitle(tr("Material Properties - %1").arg(QString::fromUtf8(detail->m_name.c_str())));

        // Spelling out that edits are scoped to this emitter is worth the line: the same .material can be
        // assigned to several emitters, and nothing else on screen says the values are not shared.
        m_heading->setText(
            tr("<b>%1</b><br/>%2<br/><i>Overrides apply to this emitter only. Other emitters using the same "
               "material are unaffected.</i>")
                .arg(QString::fromUtf8(detail->m_name.c_str()))
                .arg(QString::fromUtf8(materialPath.c_str())));
    }
} // namespace OpenParticleSystemEditor
