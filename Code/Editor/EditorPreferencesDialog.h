/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QDialog>
#include <QScopedPointer>
#include <QPixmap>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace Ui
{
    class EditorPreferencesDialog;
}

class EditorPreferencesTreeWidgetItem;
void WidgetHandleKeyPressEvent(QWidget* widget, QKeyEvent* event)
{
    // If the enter key is pressed during any text input, the dialog box will close
    // making it inconvenient to do multiple edits. This routine captures the
    // Key_Enter or Key_Return and clears the focus to give a visible cue that
    // editing of that field has finished and then doesn't propogate it.
    if (event->key() != Qt::Key::Key_Enter && event->key() != Qt::Key::Key_Return)
    {
        QApplication::sendEvent(widget, event);
    }
    else
    {
        if (QWidget* editWidget = QApplication::focusWidget())
        {
            editWidget->clearFocus();
        }
    }
}



class EditorPreferencesDialog
    : public QDialog
    , public AzToolsFramework::IPropertyEditorNotify
{
public:
    EditorPreferencesDialog(QWidget* pParent = nullptr);
    ~EditorPreferencesDialog();

    // IPropertyEditorNotify
    void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override {}
    void AfterPropertyModified(AzToolsFramework::InstanceDataNode* node) override;
    void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override {}
    void SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override {}
    void SealUndoStack() override {}

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void CreateImages();
    void CreatePages();
    void SetActivePage(EditorPreferencesTreeWidgetItem* pageItem);
    void SetFilter(const QString& filter);

    void OnTreeCurrentItemChanged();
    void OnReject();
    void OnAccept();
    void OnManage();

    struct SAutoBackup
    {
        AZ_TYPE_INFO(SAutoBackup, "{547A0B74-B513-4A74-A27B-28256BE730E7}")

        SAutoBackup()
            : bEnabled(false)
            , nTime(0)
            , nRemindTime(0) { }

        bool bEnabled;
        int nTime;
        int nRemindTime;
    };
    SAutoBackup origAutoBackup;

    QScopedPointer<Ui::EditorPreferencesDialog> ui;

    QPixmap m_selectedPixmap;
    QPixmap m_unSelectedPixmap;
    EditorPreferencesTreeWidgetItem* m_currentPageItem;
    QString m_filter;
};
