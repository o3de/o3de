/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <QObject>

namespace AzToolsFramework
{
    class EditorActionContext
        : public QObject
    {
        Q_OBJECT

    public:
        EditorActionContext() = default;
        EditorActionContext(AZStd::string identifier, AZStd::string name, AZStd::string parentIdentifier, QWidget* widget);

        QWidget* GetWidget();

    private:
        AZStd::string m_identifier;
        AZStd::string m_parentIdentifier;
        AZStd::string m_name;

        QWidget* m_widget = nullptr;
    };

} // namespace AzToolsFramework
