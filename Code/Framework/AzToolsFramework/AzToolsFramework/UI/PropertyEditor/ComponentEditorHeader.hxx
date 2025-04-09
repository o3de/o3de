/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QtWidgets/QFrame>
#include <AzCore/std/string/string.h>

#include <AzQtComponents/Components/Widgets/CardHeader.h>
#endif

class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace AzToolsFramework
{
    /**
     * Header bar for component editor info and indicators.
     * The widgets are hidden by default and will show once they're configured
     * via the appropriate setter (ex: SetIcon causes the icon widget to appear).
     */
    class ComponentEditorHeader
        : public AzQtComponents::CardHeader
    {
        Q_OBJECT;
    public:
        ComponentEditorHeader(QWidget* parent = nullptr);

        /// Set a title. Passing an empty string will hide the widget.
        void SetTitle(const QString& title);

        /// Set a title style option.
        void setTitleProperty(const char *name, const QVariant &value);

        /// Redraw title to get new style to work
        void RefreshTitle();

        /// Set an icon. Passing a null icon will hide the widget.
        void SetIcon(const QIcon& icon);

        /// Set a secondary icon to be drawn on top of the main icon.
        void SetIconOverlay(const QIcon& icon);

        // Set whether the component icon is clickable
        void SetComponentIconClickable(bool clickable);

        /// Set whether the header has an expand/contract button.
        /// Note that the header itself will not change size or hide, it
        /// simply causes the OnExpanderChanged signal to fire.
        void SetExpandable(bool expandable);
        bool IsExpandable() const;

        void SetExpanded(bool expanded);
        bool IsExpanded() const;

        void SetWarningIcon(const QIcon& icon);
        void SetWarning(bool warning);
        bool IsWarning() const;

        void SetReadOnly(bool readOnly);
        bool IsReadOnly() const;

        /// Set whether the header has a context menu widget.
        /// This widget can fire the OnContextMenuClicked signal.
        /// This widget will also display any menu set via SetContextMenu(QMenu*).
        void SetHasContextMenu(bool showContextMenu);

        void SetHelpURL(AZStd::string& url);
        void ClearHelpURL();

        void SetFilterString(const AZStd::string& str);
        bool TitleMatchesFilter() const;
    Q_SIGNALS:
        void OnContextMenuClicked(const QPoint& position);
        void OnExpanderChanged(bool expanded);

    private:
        void TriggerContextMenuUnderButton();
        void UpdateStyleSheets();
        void TriggerHelpButton();

        // Widgets in header
        QVBoxLayout* m_mainLayout = nullptr;
        QHBoxLayout* m_backgroundLayout = nullptr;
        QFrame* m_backgroundFrame = nullptr;
        QPushButton* m_expanderButton = nullptr;
        QLabel* m_iconLabel = nullptr;
        QLabel* m_titleLabel = nullptr;
        QLabel* m_warningLabel = nullptr;
        QPushButton* m_contextMenuButton = nullptr;
        QPushButton* m_helpButton = nullptr;
        QString m_title;
        QString m_currentFilterString;
        bool m_warning = false;
        bool m_readOnly = false;

        AZStd::string m_helpUrl;
    };
}
