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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEWFBXIMPORTPREVIEWDIALOG_H
#define CRYINCLUDE_EDITOR_TRACKVIEWFBXIMPORTPREVIEWDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class TrackViewFBXImportPreviewDialog;
}

class CTrackViewFBXImportPreviewDialog
    : public QDialog
{
    Q_OBJECT
public:
    CTrackViewFBXImportPreviewDialog(QWidget* parent = nullptr);
    virtual ~CTrackViewFBXImportPreviewDialog();

    void AddTreeItem(const QString& objectName);
    bool IsObjectSelected(const QString& objectName) const;

    int exec() override;

protected:
    void OnBnSelectAllClicked();
    void OnBnUnselectAllClicked();

private:
    friend class FBXImportModel;
    struct Item
    {
        QString name;
        bool checked;
    };
    typedef QVector<Item> TItemsMap;
    TItemsMap m_fBXItemNames;
    QScopedPointer<Ui::TrackViewFBXImportPreviewDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEWFBXIMPORTPREVIEWDIALOG_H
