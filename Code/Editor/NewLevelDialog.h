/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <QApplication>
#include <QPainter>
#include <QScopedPointer>
#include <QAbstractButton>
#include <QDialog>
#include <QStyledItemDelegate>

#include "LevelRoots.h"

// QListWidget icon-mode items render icons in white when selected in Qt6 because
// QIcon::Selected mode tints the pixmap. This delegate draws the selection background
// manually and strips State_Selected before Qt renders, keeping the icon readable.
class DrawIconWithBackgroundDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

        const bool selected = opt.state & QStyle::State_Selected;
        if (selected)
        {
            painter->fillRect(opt.rect, opt.palette.highlight());
        }

        opt.state &= ~QStyle::State_Selected;
        opt.state &= ~QStyle::State_HasFocus;
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    }
};

namespace Ui {
    class CNewLevelDialog;
}

class CNewLevelDialog
    : public QDialog
{
    Q_OBJECT

public:
    CNewLevelDialog(QWidget* pParent = nullptr);
    ~CNewLevelDialog();

    QString GetLevel() const;
    QString GetLevelsFolder() const; // Absolute path of the active root's Levels folder
    bool ValidateLevel();
    QString GetTemplateName() const;

protected:
    void UpdateData(bool fromUi = true);
    void OnInitDialog();
    void ReloadLevelFolder();
    void showEvent(QShowEvent* event) override;
    void InitTemplateListWidget() const;

protected slots:
    void OnLevelNameChange();
    void OnClearButtonClicked();
    void PopupAssetPicker();
    void OnStartup();
    void OnRootSelected(int index);

private:
    void PopulateRootSelector();
    const LevelRoots::Root* CurrentRoot() const;

public:
    QString         m_level;
    QString         m_levelFolders;
    QScopedPointer<Ui::CNewLevelDialog> ui;
    bool m_initialized;

private:
    // Cached list of "Levels" roots (project + active gems with a Levels
    // folder). Drives the "Root" combo box and GetLevelsFolder().
    QVector<LevelRoots::Root> m_roots;
};
