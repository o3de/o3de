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

#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QSharedPointer>
#include <QToolButton>

namespace AzToolsFramework::Prefab
{
    //! Class to handle the override label property when encountered in a DPE DOM.
    //! Responsible for setting the ui/ux for overridden properties.
    class PrefabOverrideLabelHandler
        : public PropertyHandlerWidget<QWidget>
    {
    public:
        inline static constexpr const char* OverriddenPropertyName = "overridden";

        PrefabOverrideLabelHandler();

        //! Specifies the behavior when override label property is encountered in the DPE DOM.
        //! @param value The value holding the override label property in the DPE DOM
        void SetValueFromDom(const AZ::Dom::Value& value) override;

        bool ResetToDefaults() override;

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
} // namespace AzToolsFramework::Prefab
