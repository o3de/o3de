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

#ifndef CRYINCLUDE_EDITORCOMMON_LISTSELECTIONDIALOG_H
#define CRYINCLUDE_EDITORCOMMON_LISTSELECTIONDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommonAPI.h"
#include <QDialog>
#include <QMap>
#endif

class DeepFilterProxyModel;
class QLineEdit;
class QModelIndex;
class QStandardItemModel;
class QStandardItem;
class QString;
class QTreeView;
class QWidget;
class QByteArray;

class EDITOR_COMMON_API ListSelectionDialog
    : public QDialog
{
    Q_OBJECT

public:
    ListSelectionDialog(QWidget* parent);
    void SetColumnText(int column, const char* text);
    void SetColumnWidth(int column, int width);

    void AddRow(const char* firstColumnValue);
    void AddRow(const char* firstColumnValue, const QIcon& icon);
    void AddRowColumn(const char* value);

    QString ChooseItem(const QString& currentValue);

    QSize sizeHint() const override;

protected slots:
    void onActivated(const QModelIndex& index);
    void onFilterChanged(const QString&);

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    QTreeView* m_tree;
    QStandardItemModel* m_model;
    DeepFilterProxyModel* m_filterModel;
    typedef QMap<QString, QStandardItem*> StringToItem;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    StringToItem m_firstColumnToItem;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QLineEdit* m_filterEdit;
    QByteArray m_chosenItem;
    int m_currentColumn;
};


#endif // CRYINCLUDE_EDITORCOMMON_LISTSELECTIONDIALOG_H
