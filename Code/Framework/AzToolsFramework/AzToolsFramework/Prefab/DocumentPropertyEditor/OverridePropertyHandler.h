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

#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QToolButton>


namespace AzToolsFramework::Prefab
{
    //! Class to handle the override property when encountered in a DPE DOM.
    //! Responsible for setting the ui/ux for overridden properties.
    class OverridePropertyHandler
        : public PropertyHandlerWidget<QToolButton>
    {
    public:
        OverridePropertyHandler();
        
        //! Specifies the behavior when override property is encountered in the DPE DOM.
        //! @param value The value holding the overridden property in the DPE DOM
        void SetValueFromDom(const AZ::Dom::Value& value);

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return PrefabPropertyEditorNodes::PrefabOverrideProperty::Name;
        }

    private:
        //! Shows a custom menu when the property is clicked. Can handle operations like 'Revert', etc.
        void ShowContextMenu(const QPoint&);
    };
} // namespace AzToolsFramework::Prefab
