/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/AssetWidget.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <QHBoxLayout>
#include <QLabel>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

namespace OpenParticleSystemEditor
{
    AssetWidget::AssetWidget(const AZStd::string& label, const AZ::Data::AssetType& assetType, QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(20, 0, 0, 0);
        layout->setSpacing(2);
        QLabel* name = new QLabel(tr(label.c_str()));
        name->setAlignment(Qt::AlignLeft);
        name->setMinimumWidth(0);
        m_assetCtrl = new AzToolsFramework::PropertyAssetCtrl(this);
        m_assetCtrl->SetCurrentAssetType(assetType);
        m_assetCtrl->SetClearButtonEnabled(false);
        m_assetCtrl->SetShowThumbnail(true);
        m_assetCtrl->SetShowProductAssetName(false);
        m_assetCtrl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        connect(
            m_assetCtrl,
            &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged,
            this,
            [this](const AZ::Data::AssetId& assetId)
            {
                if (m_AssetSelectionChanged)
                {
                    m_AssetSelectionChanged(assetId);
                }
            });

        QToolButton* button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        button->setToolTip(tr("Edit asset"));
        button->setVisible(false);

        layout->addWidget(name, 2, Qt::AlignLeft);
        layout->addWidget(m_assetCtrl, 5);
        layout->addWidget(button, Qt::AlignRight);

        setLayout(layout);
        setVisible(false);
    }

    void AssetWidget::SetOnAssetSelectionChangedCallback(AssetChangeCB funcPtr)
    {
        m_AssetSelectionChanged = funcPtr;
    }

    void AssetWidget::SetAssetId(const AZ::Data::AssetId& newId)
    {
        m_assetCtrl->blockSignals(true);
        m_assetCtrl->SetSelectedAssetID(newId);
        m_assetCtrl->blockSignals(false);
    }

    void AssetWidget::Clear()
    {
        m_assetCtrl->SetSelectedAssetID(AZ::Data::AssetId());
    }
}
