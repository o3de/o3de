/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemDependenciesDialog.h>
#include <GemCatalog/GemModel.h>
#include <AzQtComponents/Components/FlowLayout.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>

namespace O3DE::ProjectManager
{
    GemDependenciesDialog::GemDependenciesDialog(GemModel* gemModel, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Dependent Gems"));
        setObjectName("GemDependenciesDialog");
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);

        QVBoxLayout* layout = new QVBoxLayout();
        // layout margin/alignment cannot be set with qss
        layout->setMargin(15);
        layout->setAlignment(Qt::AlignTop);
        setLayout(layout);

        // message
        QLabel* instructionLabel = new QLabel(
            tr("The following gem dependencies are no longer needed and will be deactivated.<br><br>"
                "To keep these Gems enabled, select the checkbox next to it."));
        layout->addWidget(instructionLabel);

        // checkboxes
        FlowLayout* flowLayout = new FlowLayout();
        QVector<QModelIndex> gemsToRemove = gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true);
        for (const QModelIndex& gem : gemsToRemove)
        {
            if (GemModel::WasPreviouslyAddedDependency(gem))
            {
                QCheckBox* checkBox = new QCheckBox(GemModel::GetName(gem));
                connect(checkBox, &QCheckBox::stateChanged, this,
                    [=](int state)
                    {
                        GemModel::SetIsAdded(*gemModel, gem, /*isAdded=*/state == Qt::Checked);
                    });
                flowLayout->addWidget(checkBox);
            }
        }
        layout->addLayout(flowLayout);

        layout->addSpacing(10);
        layout->addStretch(1);

        // buttons
        QDialogButtonBox* dialogButtons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(dialogButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(dialogButtons, &QDialogButtonBox::rejected, this,
            [=]()
            {
                // de-select any Gems the user selected because they're canceling
                for (const QModelIndex& gem : gemsToRemove)
                {
                    if (GemModel::WasPreviouslyAddedDependency(gem) && GemModel::IsAdded(gem))
                    {
                        GemModel::SetIsAdded(*gemModel, gem, /*isAdded=*/false);
                    }
                }

                reject();
            });
        layout->addWidget(dialogButtons);
    }
} // namespace O3DE::ProjectManager
