/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
