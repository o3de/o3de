/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        mListWidget = new QListWidget();
        mListWidget->setAlternatingRowColors(true);
        if (multiSelect)
        {
            mListWidget->setSelectionMode(QListWidget::ExtendedSelection);
        }
        else
        {
            mListWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        }

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mListWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, &QPushButton::clicked, this, &MorphTargetSelectionWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &MorphTargetSelectionWindow::reject);
        connect(mListWidget, &QListWidget::itemSelectionChanged, this, &MorphTargetSelectionWindow::OnSelectionChanged);
    }


    MorphTargetSelectionWindow::~MorphTargetSelectionWindow()
    {
    }


    const AZStd::vector<uint32>& MorphTargetSelectionWindow::GetMorphTargetIDs() const
    {
        return mSelection;
    }


    void MorphTargetSelectionWindow::OnSelectionChanged()
    {
        mSelection.clear();

        const int numItems = mListWidget->count();
        mSelection.reserve(numItems);
        for (int i = 0; i < numItems; ++i)
        {
            QListWidgetItem* item = mListWidget->item(i);
            if (!item->isSelected())
            {
                continue;
            }

            mSelection.emplace_back(item->data(Qt::UserRole).toInt());
        }
    }


    void MorphTargetSelectionWindow::Update(EMotionFX::MorphSetup* morphSetup, const AZStd::vector<uint32>& selection)
    {
        if (morphSetup == nullptr)
        {
            return;
        }

        mListWidget->blockSignals(true);
        mListWidget->clear();

        mSelection = selection;

        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
            const uint32 morphTargetID = morphTarget->GetID();

            QListWidgetItem* item = new QListWidgetItem();
            item->setText(morphTarget->GetName());
            item->setData(Qt::UserRole, morphTargetID);

            mListWidget->addItem(item);

            if (AZStd::find(mSelection.begin(), mSelection.end(), morphTargetID) != mSelection.end())
            {
                item->setSelected(true);
            }
        }

        mListWidget->blockSignals(false);
        mSelection = selection;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_MorphTargetSelectionWindow.cpp>
