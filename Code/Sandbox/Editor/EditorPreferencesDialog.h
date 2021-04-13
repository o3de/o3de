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

#include <QDialog>
#include <QScopedPointer>
#include <QPixmap>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace Ui
{
    class EditorPreferencesDialog;
}

class EditorPreferencesTreeWidgetItem;

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

    void SetFilterText(const QString& filter);

protected:
    void showEvent(QShowEvent* event) override;

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
