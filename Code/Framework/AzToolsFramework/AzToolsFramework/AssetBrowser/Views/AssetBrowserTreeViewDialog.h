/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetPicker/AssetPickerDialog.h>
#endif

namespace Ui
{
    class AssetBrowserTreeViewDialogClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserTreeViewDialog
            : public AzToolsFramework::AssetBrowser::AssetPickerDialog
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTreeViewDialog, AZ::SystemAllocator);
            explicit AssetBrowserTreeViewDialog(AssetSelectionModel& selection, QWidget* parent = nullptr);

        protected:
            bool EvaluateSelection() const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
