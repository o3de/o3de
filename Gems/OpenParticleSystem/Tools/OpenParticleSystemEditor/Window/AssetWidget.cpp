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
#include <QToolButton>
#include <QLineEdit>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace OpenParticleSystemEditor
{
    AssetWidget::AssetWidget(const AZStd::string& label, const AZ::Data::AssetType& assetType, const AZStd::string& extention, QWidget* parent)
        : QWidget(parent)
        , m_title(label)
        , m_assetType(assetType)
        , m_extention(extention)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(20, 0, 0, 0);
        layout->setSpacing(2);
        QLabel* name = new QLabel(tr(m_title.c_str()));
        name->setAlignment(Qt::AlignLeft);
        name->setMinimumWidth(0);
        m_assetPathBrowseEdit = new AzQtComponents::BrowseEdit(this);
        m_assetPathBrowseEdit->lineEdit()->setFocusPolicy(Qt::StrongFocus);
        m_assetPathBrowseEdit->lineEdit()->installEventFilter(this);
        m_assetPathBrowseEdit->setClearButtonEnabled(false);
        m_assetPathBrowseEdit->setLineEditReadOnly(true);
        m_assetPathBrowseEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        connect(m_assetPathBrowseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &AssetWidget::PopupAssetPicker);

        QToolButton* button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        button->setToolTip(tr("Edit asset"));
        button->setVisible(false);

        layout->addWidget(name, 2, Qt::AlignLeft);
        layout->addWidget(m_assetPathBrowseEdit, 5);
        layout->addWidget(button, Qt::AlignRight);

        setLayout(layout);
        setVisible(false);
    }

    void AssetWidget::SetAssetPath(const AZStd::string& path)
    {
        m_assetPathBrowseEdit->setText(QString::fromStdString(path.c_str()));
    }

    void AssetWidget::Clear()
    {
        m_assetPathBrowseEdit->lineEdit()->clear();
    }

    void AssetWidget::PopupAssetPicker()
    {
        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(m_assetType);
        selection.SetTitle(m_title.c_str());

        AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, parentWidget());
        if (!selection.IsValid())
        {
            AZ_TracePrintf("AssetWidget", "PopupAssetPicker failed\n");
            return;
        }
        auto path = selection.GetResult()->GetRelativePath();

        AZ::StringFunc::Path::ReplaceExtension(path, m_extention.c_str());

        m_assetPathBrowseEdit->setText(path.c_str());

        if (m_AssetSelectionChanged)
        {
            m_AssetSelectionChanged(path.c_str());
        }
    }
}
