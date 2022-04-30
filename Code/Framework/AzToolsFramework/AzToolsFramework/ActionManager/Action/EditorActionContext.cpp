/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/EditorActionContext.h>

namespace AzToolsFramework
{
    EditorActionContext::EditorActionContext(
        AZStd::string_view identifier, AZStd::string_view name, AZStd::string_view parentIdentifier, QWidget* widget)
        : m_identifier(identifier)
        , m_name(name)
        , m_parentIdentifier(parentIdentifier)
        , m_widget(widget)
    {
    }

    QWidget* EditorActionContext::GetWidget()
    {
        return m_widget;
    }

} // namespace AzToolsFramework
