/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    SelectDestinationDialog(QString message, QWidget* parent = nullptr, QString suggestedDestination = QString());
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
    void SetPreviousOrSuggestedDestinationDirectory(QString suggestedDestination);
    QString DestinationDirectory() const;

    QScopedPointer<Ui::SelectDestinationDialog> m_ui;
    DestinationDialogValidator* m_validator;
};
