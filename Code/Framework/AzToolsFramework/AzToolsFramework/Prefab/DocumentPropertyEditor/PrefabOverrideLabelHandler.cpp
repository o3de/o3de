/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabOverrideLabelHandler.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>
#include <AzCore/Console/IConsole.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>

AZ_CVAR(
    bool,
    ed_dpeCompactOverrideLabel,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
    "If set (the default), the override label of a DPE row is one widget that draws the override dot itself "
    "(CompactPrefabOverrideLabelHandler); if not, a tool button and a label in a container (PrefabOverrideLabelHandler). Rows built "
    "after a change follow it.");

namespace AzToolsFramework::Prefab
{
    QWeakPointer<QIcon> PrefabOverrideLabelHandler::s_sharedOverrideIcon;
    QWeakPointer<QIcon> PrefabOverrideLabelHandler::s_sharedEmptyIcon;
    constexpr QSize kIconSize(6, 6);

    // ----------------------------------------------------------------------------------
    // Registration: the composite handler or the compact one, as ed_dpeCompactOverrideLabel says
    // ----------------------------------------------------------------------------------

    void RegisterPrefabOverrideLabelHandler(PropertyEditorToolsSystemInterface& propertyEditorToolsSystem)
    {
        PropertyEditorToolsSystemInterface::HandlerData handlerData;
        handlerData.m_name = PrefabOverrideLabelHandler::GetHandlerName();
        handlerData.m_shouldHandleType = [](const AZ::TypeId& typeId)
        {
            return PrefabOverrideLabelHandler::ShouldHandleType(typeId);
        };
        handlerData.m_factory = []() -> PropertyEditorToolsSystemInterface::PropertyHandlerInstance
        {
            if (ed_dpeCompactOverrideLabel)
            {
                return AZStd::make_unique<CompactPrefabOverrideLabelHandler>();
            }
            return AZStd::make_unique<PrefabOverrideLabelHandler>();
        };
        handlerData.m_isDefaultHandler = false;
        propertyEditorToolsSystem.RegisterHandler(AZStd::move(handlerData));
    }

    // ----------------------------------------------------------------------------------
    // PrefabOverrideLabelHandler: a tool button and a label in a container
    // ----------------------------------------------------------------------------------

    PrefabOverrideLabelHandler::PrefabOverrideLabelHandler()
        : m_overridden(false)
        , m_iconButton(new QToolButton())
        , m_textLabel(new AzQtComponents::ElidingLabel())
        , m_overrideIcon(s_sharedOverrideIcon.lock())
        , m_emptyIcon(s_sharedEmptyIcon.lock())
    {
        if (!m_overrideIcon)
        {
            m_overrideIcon = QSharedPointer<QIcon>(new QIcon(QStringLiteral(":/Entity/entity_modified_as_override.svg")));
            s_sharedOverrideIcon = m_overrideIcon;
        }
        if (!m_emptyIcon)
        {
            m_emptyIcon = QSharedPointer<QIcon>(new QIcon);
            s_sharedEmptyIcon = m_emptyIcon;
        }

        m_textLabel->setProperty(OverriddenPropertyName, false);
        // The label of an overridden property is bold: with style sheets, an inline sheet on every label (the GuiPerfFloor noop and
        // passthrough rungs leave it out); flat, the element Global.Text.OverriddenPropertyLabel (PropertyEditorElements.cpp) when
        // RefreshUI repolishes the label, so that no row label carries a sheet (and a QStyleSheetStyle) of its own.
        if (!AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            AzQtComponents::GuiPerfFloor::setInlineStyleSheet(
                m_textLabel, QString("[%1=\"true\"] { font-weight: bold }").arg(OverriddenPropertyName));
        }

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_iconButton);
        layout->addWidget(m_textLabel);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &PrefabOverrideLabelHandler::customContextMenuRequested, this, &PrefabOverrideLabelHandler::ShowContextMenu);
    }

    void PrefabOverrideLabelHandler::SetValueFromDom(const AZ::Dom::Value& domValue)
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        m_overridden = PrefabOverrideLabel::IsOverridden.ExtractFromDomNode(domValue).value_or(false);
        // Cache the node so we can retrieve data when we handle context menu actions. 
        m_node = domValue;
    }

    void PrefabOverrideLabelHandler::RefreshUI()
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        // Set up label
        AZStd::string_view labelText = PrefabOverrideLabel::Value.ExtractFromDomNode(m_node).value_or("");
        m_textLabel->setText(QString::fromUtf8(labelText.data(), aznumeric_cast<int>(labelText.size())));

        if (AzQtComponents::StyleManager::setObjectProperty(m_textLabel, OverriddenPropertyName, m_overridden))
        {
            m_textLabel->RefreshStyle();
        }
        // Set up icon
        m_iconButton->setIcon(m_overridden ? *m_overrideIcon : *m_emptyIcon);
        m_iconButton->setIconSize(kIconSize);

    }

    bool PrefabOverrideLabelHandler::ResetToDefaults()
    {
        // avoid setting properties related to styling, we'll do that in refreshUI
        m_overridden = false;
        m_textLabel->setText(QString()); 
        m_iconButton->setIcon(*m_emptyIcon);
        m_iconButton->setIconSize(kIconSize);
        m_node = AZ::Dom::Value();
        return true;
    }

    void PrefabOverrideLabelHandler::SetFilter(const QString& filter)
    {
        m_textLabel->SetFilter(filter);
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

    // ----------------------------------------------------------------------------------
    // CompactPrefabOverrideLabelHandler: the same label in one widget
    // ----------------------------------------------------------------------------------

    CompactPrefabOverrideLabelHandler::CompactPrefabOverrideLabelHandler()
    {
        // Bold when overridden, as PrefabOverrideLabelHandler's label is: the inline sheet with style sheets, the flattened GUI's
        // element without (Global.Text.OverriddenPropertyLabel, which matches an ElidingLabel and so this one)
        setProperty(PrefabOverrideLabelHandler::OverriddenPropertyName, false);
        if (!AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            AzQtComponents::GuiPerfFloor::setInlineStyleSheet(
                this, QString("[%1=\"true\"] { font-weight: bold }").arg(PrefabOverrideLabelHandler::OverriddenPropertyName));
        }
        setIcon(QIcon(), kIconSize);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &CompactPrefabOverrideLabelHandler::customContextMenuRequested, this, &CompactPrefabOverrideLabelHandler::ShowContextMenu);
    }

    void CompactPrefabOverrideLabelHandler::SetValueFromDom(const AZ::Dom::Value& domValue)
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        m_overridden = PrefabOverrideLabel::IsOverridden.ExtractFromDomNode(domValue).value_or(false);
        // The node gives the context menu its relative path and its revert message
        m_node = domValue;
    }

    void CompactPrefabOverrideLabelHandler::RefreshUI()
    {
        using PrefabPropertyEditorNodes::PrefabOverrideLabel;

        AZStd::string_view labelText = PrefabOverrideLabel::Value.ExtractFromDomNode(m_node).value_or("");
        setText(QString::fromUtf8(labelText.data(), aznumeric_cast<int>(labelText.size())));

        if (AzQtComponents::StyleManager::setObjectProperty(this, PrefabOverrideLabelHandler::OverriddenPropertyName, m_overridden))
        {
            RefreshStyle();
        }
        static const QIcon s_overrideIcon(QStringLiteral(":/Entity/entity_modified_as_override.svg"));
        setIcon(m_overridden ? s_overrideIcon : QIcon(), kIconSize);
    }

    bool CompactPrefabOverrideLabelHandler::ResetToDefaults()
    {
        // the styling properties are left to RefreshUI, as PrefabOverrideLabelHandler leaves them
        m_overridden = false;
        setText(QString());
        setIcon(QIcon(), kIconSize);
        m_node = AZ::Dom::Value();
        return true;
    }

    void CompactPrefabOverrideLabelHandler::SetFilter(const QString& filter)
    {
        AzQtComponents::IconElidingLabel::SetFilter(filter);
    }

    void CompactPrefabOverrideLabelHandler::ShowContextMenu(const QPoint& position)
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
