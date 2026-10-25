/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/IconElidingLabel.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QSharedPointer>
#include <QToolButton>

namespace AzToolsFramework
{
    class PropertyEditorToolsSystemInterface;
}

namespace AzToolsFramework::Prefab
{
    //! Registers the handler of the override label (PrefabOverrideLabel): it makes a PrefabOverrideLabelHandler, or a
    //! CompactPrefabOverrideLabelHandler while ed_dpeCompactOverrideLabel is on. The cvar is read for every handler made, so a row
    //! built after it changes follows it.
    AZTF_API void RegisterPrefabOverrideLabelHandler(PropertyEditorToolsSystemInterface& propertyEditorToolsSystem);

    //! Class to handle the override label property when encountered in a DPE DOM.
    //! Responsible for setting the ui/ux for overridden properties.
    class AZTF_API PrefabOverrideLabelHandler
        : public PropertyHandlerWidget<QWidget>
    {
    public:
        inline static constexpr const char* OverriddenPropertyName = "overridden";

        PrefabOverrideLabelHandler();

        //! Specifies the behavior when override label property is encountered in the DPE DOM.
        //! @param value The value holding the override label property in the DPE DOM
        void SetValueFromDom(const AZ::Dom::Value& value) override;

        void RefreshUI() override;

        bool ResetToDefaults() override;

        void SetFilter(const QString& filter) override;

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return PrefabPropertyEditorNodes::PrefabOverrideLabel::Name;
        }

    private:
        //! Shows a custom menu when the property is right clicked. Can handle operations like 'Revert override', etc.
        void ShowContextMenu(const QPoint&);

        bool m_overridden;
        AZ::Dom::Value m_node;

        QToolButton* m_iconButton;
        AzQtComponents::ElidingLabel* m_textLabel;
        QSharedPointer<QIcon> m_overrideIcon;
        QSharedPointer<QIcon> m_emptyIcon;

        static QWeakPointer<QIcon> s_sharedOverrideIcon;
        static QWeakPointer<QIcon> s_sharedEmptyIcon;
    };

    //! The override label in one widget (ed_dpeCompactOverrideLabel): an IconElidingLabel draws the override dot before the text, where
    //! PrefabOverrideLabelHandler is a tool button and a label in a container. It looks, sizes and behaves the same: the bold label of
    //! an overridden property, the filter, the "Revert override" menu, and no Tab stop.
    class AZTF_API CompactPrefabOverrideLabelHandler
        : public PropertyHandlerWidget<AzQtComponents::IconElidingLabel>
    {
    public:
        CompactPrefabOverrideLabelHandler();

        void SetValueFromDom(const AZ::Dom::Value& value) override;

        void RefreshUI() override;

        bool ResetToDefaults() override;

        void SetFilter(const QString& filter) override;

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return PrefabOverrideLabelHandler::GetHandlerName();
        }

    private:
        void ShowContextMenu(const QPoint&);

        bool m_overridden = false;
        AZ::Dom::Value m_node;
        QSharedPointer<QIcon> m_overrideIcon; //!< one icon for every compact label, freed with the last
    };
} // namespace AzToolsFramework::Prefab
