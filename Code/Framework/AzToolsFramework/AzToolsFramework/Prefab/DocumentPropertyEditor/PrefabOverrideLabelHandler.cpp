/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabOverrideLabelHandler.h>

namespace AzToolsFramework::Prefab
{
    PrefabOverrideLabelHandler::PrefabOverrideLabelHandler()
        : m_overridden(false)
        , m_iconButton(new QToolButton())
        , m_textLabel(new AzQtComponents::ElidingLabel())
    {
        m_textLabel->setProperty(OverriddenPropertyName, false);
        m_textLabel->setStyleSheet(QString("[%1=\"true\"] { font-weight: bold }").arg(OverriddenPropertyName));

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(m_iconButton);
        layout->addWidget(m_textLabel);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &PrefabOverrideLabelHandler::customContextMenuRequested, this, &PrefabOverrideLabelHandler::ShowContextMenu);
    }

    void PrefabOverrideLabelHandler::SetValueFromDom(const AZ::Dom::Value& domValue)
    {
        static QIcon s_overrideIcon(QStringLiteral(":/Entity/entity_modified_as_override.svg"));
        static QIcon s_emptyIcon;
        static QSize s_iconSize(7, 7);

        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        m_overridden = PrefabOverrideLabel::IsOverridden.ExtractFromDomNode(domValue).value_or(false);

        // Set up label
        AZStd::string_view labelText = PrefabOverrideLabel::Text.ExtractFromDomNode(domValue).value_or("");
        m_textLabel->setText(QString::fromUtf8(labelText.data(), aznumeric_cast<int>(labelText.size())));

        m_textLabel->setProperty(OverriddenPropertyName, QVariant(m_overridden));
        m_textLabel->RefreshStyle();

        // Set up icon
        m_iconButton->setIcon(m_overridden ? s_overrideIcon : s_emptyIcon);
        m_iconButton->setIconSize(s_iconSize);

        // Cache the node so we can retrieve data when we handle context menu actions. 
        m_node = domValue;
    }

    void PrefabOverrideLabelHandler::ShowContextMenu(const QPoint& position)
    {
        if (!m_overridden)
        {
            return;
        }

        QMenu contextMenu;
        QAction* revertAction = contextMenu.addAction(tr("Revert override"));

        QAction* selectedItem = contextMenu.exec(mapToGlobal(position));

        if (selectedItem == revertAction)
        {
            AZStd::string_view relativePathFromEntity =
                PrefabPropertyEditorNodes::PrefabOverrideLabel::RelativePath.ExtractFromDomNode(m_node).value_or("");
            PrefabPropertyEditorNodes::PrefabOverrideLabel::RevertOverride.InvokeOnDomNode(m_node, relativePathFromEntity);
        }
    }
} // namespace AzToolsFramework::Prefab
