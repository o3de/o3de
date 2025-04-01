/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeViewDialog.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetPicker/ui_AssetPickerDialog.h>
#include <QDialog>
#include <QHBoxLayout>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTreeViewDialog::AssetBrowserTreeViewDialog( [[maybe_unused]] AssetSelectionModel& selection, [[maybe_unused]]QWidget* parent)
            : AzToolsFramework::AssetBrowser::AssetPickerDialog(selection, parent)
        {
        }

        bool AssetBrowserTreeViewDialog::EvaluateSelection() const
        {
            auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->isVisible()
                ? m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets()
                : m_ui->m_assetBrowserListViewWidget->GetSelectedAssets();
            // exactly one item must be selected, even if multi-select option is disabled, still good practice to check
            if (selectedAssets.empty())
            {
                return false;
            }

            m_selection.GetResults().clear();
            m_selection.SetSelectedFilePath(selectedAssets.front()->GetFullPath());

            return true;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
