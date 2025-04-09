/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentEditorHeader.hxx"
#include "PropertyRowWidget.hxx"

#include <QDesktopServices>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>

namespace AzToolsFramework
{
    ComponentEditorHeader::ComponentEditorHeader(QWidget* parent /*= nullptr*/)
        : AzQtComponents::CardHeader(parent)
    {
        connect(this, &AzQtComponents::CardHeader::contextMenuRequested, this, &ComponentEditorHeader::OnContextMenuClicked);
        connect(this, &AzQtComponents::CardHeader::expanderChanged, this, &ComponentEditorHeader::OnExpanderChanged);
    }

    void ComponentEditorHeader::SetTitle(const QString& title)
    {
        m_title = title;
        RefreshTitle();
    }

    void ComponentEditorHeader::setTitleProperty(const char *name, const QVariant &value)
    {
        AzQtComponents::CardHeader::setTitleProperty(name, value);
    }

    void ComponentEditorHeader::RefreshTitle()
    {
        AzQtComponents::CardHeader::setTitle(m_title);
        AzQtComponents::CardHeader::setFilter(m_currentFilterString);

        AzQtComponents::CardHeader::refreshTitle();
    }

    void ComponentEditorHeader::SetIcon(const QIcon& icon)
    {
        AzQtComponents::CardHeader::setIcon(icon);
    }

    void ComponentEditorHeader::SetIconOverlay(const QIcon& icon)
    {
        AzQtComponents::CardHeader::setIconOverlay(icon);
    }

    void ComponentEditorHeader::SetComponentIconClickable(bool clickable)
    {
        AzQtComponents::CardHeader::setIconClickable(clickable);
    }

    void ComponentEditorHeader::SetExpandable(bool expandable)
    {
        AzQtComponents::CardHeader::setExpandable(expandable);
    }

    bool ComponentEditorHeader::IsExpandable() const
    {
        return AzQtComponents::CardHeader::isExpandable();
    }

    void ComponentEditorHeader::SetExpanded(bool expanded)
    {
        AzQtComponents::CardHeader::setExpanded(expanded);
    }

    bool ComponentEditorHeader::IsExpanded() const
    {
        return AzQtComponents::CardHeader::isExpanded();
    }

    void ComponentEditorHeader::SetWarningIcon(const QIcon& icon)
    {
        AzQtComponents::CardHeader::setWarningIcon(icon);
    }

    void ComponentEditorHeader::SetWarning(bool warning)
    {
        AzQtComponents::CardHeader::setWarning(warning);
    }

    bool ComponentEditorHeader::IsWarning() const
    {
        return AzQtComponents::CardHeader::isWarning();
    }

    void ComponentEditorHeader::SetReadOnly(bool readOnly)
    {
        AzQtComponents::CardHeader::setReadOnly(readOnly);
    }

    bool ComponentEditorHeader::IsReadOnly() const
    {
        return AzQtComponents::CardHeader::isReadOnly();
    }

    void ComponentEditorHeader::SetHasContextMenu(bool showContextMenu)
    {
        AzQtComponents::CardHeader::setHasContextMenu(showContextMenu);
    }

    void ComponentEditorHeader::SetHelpURL(AZStd::string& url)
    {
        AzQtComponents::CardHeader::setHelpURL(QString(url.c_str()));
    }

    void ComponentEditorHeader::ClearHelpURL()
    {
        AzQtComponents::CardHeader::clearHelpURL();
    }

    void ComponentEditorHeader::SetFilterString(const AZStd::string& str)
    {
        m_currentFilterString = str.c_str();
    }

    bool ComponentEditorHeader::TitleMatchesFilter() const
    {
        return AzQtComponents::CardHeader::m_titleLabel->TextMatchesFilter();
    }
}

#include "UI/PropertyEditor/moc_ComponentEditorHeader.cpp"
