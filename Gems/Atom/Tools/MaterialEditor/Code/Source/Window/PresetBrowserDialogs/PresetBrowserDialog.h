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

#if !defined(Q_MOC_RUN)
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
        QListWidgetItem* CreateListItem(const QString& title, const QImage& image);

        void SetupSearchWidget();
        void SetupDialogButtons();
        void ApplySearchFilter();
        void ShowSearchMenu(const QPoint& pos);
        virtual void SelectCurrentPreset() = 0;
        virtual void SelectInitialPreset() = 0;

        QScopedPointer<Ui::PresetBrowserDialog> m_ui;
    };
} // namespace MaterialEditor
