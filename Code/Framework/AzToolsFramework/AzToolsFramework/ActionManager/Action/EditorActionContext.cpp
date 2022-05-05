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
        AZStd::string identifier, AZStd::string name, AZStd::string parentIdentifier, QWidget* widget)
        : m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
        , m_parentIdentifier(AZStd::move(parentIdentifier))
        , m_widget(widget)
    {
    }

    QWidget* EditorActionContext::GetWidget()
    {
        return m_widget;
    }

} // namespace AzToolsFramework
