/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MorphTargetSelectionWindow.h"
#include "EMStudioManager.h"
#include <QPushButton>
#include <QVBoxLayout>


namespace EMStudio
{
    MorphTargetSelectionWindow::MorphTargetSelectionWindow(QWidget* parent, bool multiSelect)
        : QDialog(parent)
    {
        setWindowTitle("Morph target selection window");

        QVBoxLayout* layout = new QVBoxLayout();

        m_listWidget = new QListWidget();
        m_listWidget->setAlternatingRowColors(true);
        if (multiSelect)
        {
            m_listWidget->setSelectionMode(QListWidget::ExtendedSelection);
        }
        else
        {
            m_listWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        }

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        m_okButton       = new QPushButton("OK");
        m_cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        layout->addWidget(m_listWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(m_okButton, &QPushButton::clicked, this, &MorphTargetSelectionWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &MorphTargetSelectionWindow::reject);
        connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &MorphTargetSelectionWindow::OnSelectionChanged);
    }


    MorphTargetSelectionWindow::~MorphTargetSelectionWindow()
    {
    }


    const AZStd::vector<uint32>& MorphTargetSelectionWindow::GetMorphTargetIDs() const
    {
        return m_selection;
    }


    void MorphTargetSelectionWindow::OnSelectionChanged()
    {
        m_selection.clear();

        const int numItems = m_listWidget->count();
        m_selection.reserve(numItems);
        for (int i = 0; i < numItems; ++i)
        {
            QListWidgetItem* item = m_listWidget->item(i);
            if (!item->isSelected())
            {
                continue;
            }

            m_selection.emplace_back(item->data(Qt::UserRole).toInt());
        }
    }


    void MorphTargetSelectionWindow::Update(EMotionFX::MorphSetup* morphSetup, const AZStd::vector<uint32>& selection)
    {
        if (morphSetup == nullptr)
        {
            return;
        }

        m_listWidget->blockSignals(true);
        m_listWidget->clear();

        m_selection = selection;

        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
            const uint32 morphTargetID = morphTarget->GetID();

            QListWidgetItem* item = new QListWidgetItem();
            item->setText(morphTarget->GetName());
            item->setData(Qt::UserRole, morphTargetID);

            m_listWidget->addItem(item);

            if (AZStd::find(m_selection.begin(), m_selection.end(), morphTargetID) != m_selection.end())
            {
                item->setSelected(true);
            }
        }

        m_listWidget->blockSignals(false);
        m_selection = selection;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_MorphTargetSelectionWindow.cpp>
