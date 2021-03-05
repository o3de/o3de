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
#ifndef NEWENTITYDIALOG_H
#define NEWENTITYDIALOG_H

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QCompleter>
#include <QDirIterator>
#include <QStringListModel>
#include <QValidator>
#include <QEvent>
#include <QLineEdit>
#endif

namespace Ui {
    class NewEntityDialog;
}

class NewEntityDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit NewEntityDialog(QWidget* parent = 0);
    ~NewEntityDialog();

private:
    Ui::NewEntityDialog* ui;
    QString baseDir = "";
    QString nameBaseDir = "";
    QCompleter* folderNameCompleter = NULL;

    void SetCategoryCompleterPath(CryStringT<char> path);
    void SetNameValidatorPath(CryStringT<char> path);

    virtual void accept();

    class EntityNameValidator
        : public QValidator
    {
    public:
        explicit EntityNameValidator(NewEntityDialog* parent = 0)
            : QValidator(parent)
            , m_Parent(parent)
        {
        }
        virtual State validate(QString& input, int& pos) const;

        NewEntityDialog* m_Parent;
    };

    EntityNameValidator* entityNameValidator;

public slots:
    void ValidateInput();
};

#endif // NEWENTITYDIALOG_H
