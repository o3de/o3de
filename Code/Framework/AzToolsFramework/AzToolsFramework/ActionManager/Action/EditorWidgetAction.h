/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>

#include <QAction>
#include <QIcon>

namespace AzToolsFramework
{
    class EditorWidgetAction
    {
    public:
        EditorWidgetAction() = default;
        EditorWidgetAction(
            AZStd::string identifier,
            AZStd::string name,
            AZStd::string category,
            AZStd::function<QWidget*()> generator
        );

        const AZStd::string& GetName() const;
        void SetName(AZStd::string name);
        const AZStd::string& GetCategory() const;
        void SetCategory(AZStd::string category);

        // Generate an instance of this WidgetAction.
        QWidget* GenerateWidget() const;

    private:
        AZStd::string m_identifier;
        AZStd::string m_name;
        AZStd::string m_category;

        AZStd::function<QWidget*()> m_generator = nullptr;
    };

} // namespace AzToolsFramework
