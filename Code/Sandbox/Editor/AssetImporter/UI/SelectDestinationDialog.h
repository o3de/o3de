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
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/StyledLineEdit.h>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#endif

class StyledLineEdit;
class QValidator;

class DestinationDialogValidator;

namespace Ui {
    class SelectDestinationDialog;
}

class SelectDestinationDialog
    : public QDialog
{
    Q_OBJECT

public:
    SelectDestinationDialog(QString message, QWidget* parent = nullptr);
    ~SelectDestinationDialog();
    
Q_SIGNALS:
    void GoBack();
    void DoCopyFiles();
    void DoMoveFiles();
    void BrowseDestinationPath(QLineEdit* destinationLineEdit);
    void Cancel();
    void UpdateImportButtonState(bool enabled);
    void SetDestinationDirectory(QString destinationDirectory);

public Q_SLOTS:
    void accept();
    void reject();
    void ShowMessage();
    void OnBrowseDestinationFilePath();
    void ValidatePath();

private:
    void UpdateMessage(QString message);
    void InitializeButtons();
    void SetPreviousDestinationDirectory();
    QString DestinationDirectory() const;

    QScopedPointer<Ui::SelectDestinationDialog> m_ui;
    DestinationDialogValidator* m_validator;
};
