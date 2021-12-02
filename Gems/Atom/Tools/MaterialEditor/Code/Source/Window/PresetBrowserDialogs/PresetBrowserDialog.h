/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <QDialog>
#endif

#include <Window/PresetBrowserDialogs/ui_PresetBrowserDialog.h>

class QImage;
class QListWidgetItem;
class QString;

namespace MaterialEditor
{
    //! Widget for managing and selecting from a library of preset assets
    class PresetBrowserDialog : public QDialog
    {
        Q_OBJECT
    public:
        PresetBrowserDialog(QWidget* parent = nullptr);
        ~PresetBrowserDialog() = default;

protected:
        void SetupPresetList();
        QListWidgetItem* CreateListItem(const QString& title, const AZ::Data::AssetId& assetId, const QSize& size);

        void SetupSearchWidget();
        void SetupDialogButtons();
        void ApplySearchFilter();
        void ShowSearchMenu(const QPoint& pos);
        virtual void SelectCurrentPreset() = 0;
        virtual void SelectInitialPreset() = 0;

        QScopedPointer<Ui::PresetBrowserDialog> m_ui;
    };
} // namespace MaterialEditor
