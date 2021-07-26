/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QDialog>
#include <QPixmap>
#include <QScopedPointer>

namespace Ui
{
    class EditorSettingsDialog;
}

namespace AzToolsFramework
{
    class EditorSettingsTreeWidgetItem;

    class EditorSettingsDialog
        : public QDialog
        , public AzToolsFramework::IPropertyEditorNotify
    {
    public:
        EditorSettingsDialog(QWidget* pParent = nullptr);
        ~EditorSettingsDialog();

        // IPropertyEditorNotify
        void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
        {
        }
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* node) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
        {
        }
        void SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
        {
        }
        void SealUndoStack() override
        {
        }

    protected:
        void showEvent(QShowEvent* event) override;

    private:
        void CreateImages();
        void CreatePages();
        // void SetActivePage(EditorSettingsTreeWidgetItem* pageItem);
        void SetFilter(const QString& filter);

        void OnTreeCurrentItemChanged();
        void OnReject();
        void OnAccept();

        struct SAutoBackup
        {
            AZ_TYPE_INFO(SAutoBackup, "{547A0B74-B513-4A74-A27B-28256BE730E7}")

            SAutoBackup()
                : bEnabled(false)
                , nTime(0)
                , nRemindTime(0)
            {
            }

            bool bEnabled;
            int nTime;
            int nRemindTime;
        };
        SAutoBackup origAutoBackup;

        QScopedPointer<Ui::EditorSettingsDialog> ui;

        QPixmap m_selectedPixmap;
        QPixmap m_unSelectedPixmap;
        EditorSettingsTreeWidgetItem* m_currentPageItem;
        QString m_filter;
    };

} // namespace AzToolsFramework
