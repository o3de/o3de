/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Editor/JointSelectionDialog.h>
#include <Editor/JointSelectionWidget.h>
#include <QAbstractItemView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>


namespace EMotionFX
{
    JointSelectionDialog::JointSelectionDialog(bool singleSelection, const QString& title, const QString& descriptionLabelText, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(title);

        QVBoxLayout* layout = new QVBoxLayout(this);
        setLayout(layout);

        QLabel* textLabel = new QLabel(descriptionLabelText, this);
        layout->addWidget(textLabel, 0, Qt::AlignLeft);

        m_jointSelectionWidget = new JointSelectionWidget(singleSelection ? QAbstractItemView::SingleSelection : QAbstractItemView::ExtendedSelection, this);
        layout->addWidget(m_jointSelectionWidget);

        QHBoxLayout* buttonLayout = new QHBoxLayout(this);
        layout->addLayout(buttonLayout);

        QPushButton* okButton = new QPushButton("OK");
        buttonLayout->addWidget(okButton);
        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(cancelButton);

        if (singleSelection)
        {
            connect(m_jointSelectionWidget, &JointSelectionWidget::ItemDoubleClicked, this, &JointSelectionDialog::OnItemDoubleClicked);
        }

        connect(okButton, &QPushButton::clicked, this, &JointSelectionDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &JointSelectionDialog::reject);

        setMinimumSize(QSize(500, 400));
        resize(700, 800);
    }

    JointSelectionDialog::~JointSelectionDialog()
    {
    }

    void JointSelectionDialog::OnItemDoubleClicked(const QModelIndex& modelIndex)
    {
        AZ_UNUSED(modelIndex);
        accept();
    }

    void JointSelectionDialog::SelectByJointNames(const AZStd::vector<AZStd::string>& jointNames)
    {
        m_jointSelectionWidget->SelectByJointNames(jointNames);
    }

    AZStd::vector<AZStd::string> JointSelectionDialog::GetSelectedJointNames() const
    {
        return m_jointSelectionWidget->GetSelectedJointNames();
    }

    void JointSelectionDialog::SetFilterState(const QString& category, const QString& displayName, bool enabled)
    {
        m_jointSelectionWidget->SetFilterState(category, displayName, enabled);
    }

    void JointSelectionDialog::HideIcons()
    {
        m_jointSelectionWidget->HideIcons();
    }
} // namespace EMotionFX
