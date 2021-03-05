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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "UnsavedChangesDialog.h"

#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QScreen>

CUnsavedChangedDialog::CUnsavedChangedDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Unsaved Changes");
    setModal(true);

    auto layout = new QBoxLayout(QBoxLayout::TopToBottom);
    auto label = new QLabel("The following files were modified.\n\nWould you like to save them before closing?");
    layout->addWidget(label, 0);

    m_list = new QListWidget();
    layout->addWidget(m_list, 1);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel, Qt::Horizontal);
    layout->addWidget(buttonBox, 0);

    connect(buttonBox, &QDialogButtonBox::clicked, this, [this, buttonBox](QAbstractButton* button)
        {
            done(buttonBox->buttonRole(button));
        });

    setLayout(layout);
    resize(500, 350);

    if (parent)
    {
        QPoint center = parent->mapToGlobal(parent->geometry().center());
        QDesktopWidget* desktop = QApplication::desktop();
        const QRect screenDimensions = QApplication::screenAt(center)->geometry();
        if (screenDimensions.contains(center))
        {
            QPoint dialogPosition = center - QPoint(width() / 2, height() / 2);
            if (screenDimensions.contains(dialogPosition))
            {
                move(dialogPosition);
            }
            else
            {
                move(center);
            }
        }
        else
        {
            move((screenDimensions.width() - width()) / 2, (screenDimensions.height() - height()) / 2);
        }
    }
}

bool CUnsavedChangedDialog::Exec(DynArray<string>* selectedFiles, const DynArray<string>& files)
{
    m_list->clear();

    std::vector<QListWidgetItem*> items(files.size(), nullptr);

    for (size_t i = 0; i < files.size(); ++i)
    {
        auto item = new QListWidgetItem(files[i].c_str(), m_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        items[i] = item;
    }

    selectedFiles->clear();

    int result = exec();
    if (result == QDialogButtonBox::YesRole)
    {
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (items[i]->checkState() == Qt::Checked)
            {
                selectedFiles->push_back(files[i].c_str());
            }
        }
        return true;
    }
    else if (result == QDialogButtonBox::NoRole)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool EDITOR_COMMON_API UnsavedChangesDialog(QWidget* parent, DynArray<string>* selectedFiles, const DynArray<string>& files)
{
    CUnsavedChangedDialog dialog(parent);
    return dialog.Exec(selectedFiles, files);
}

#include <moc_UnsavedChangesDialog.cpp>
