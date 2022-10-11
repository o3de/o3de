/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_base.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>

#include <QObject>

namespace AzToolsFramework
{
    constexpr const char* DefaultModeIdentifier = "default";

    //! Editor Action Context class definition.
    //! Identifies a collection of Actions in the context of 
    class EditorActionContext
        : public QObject
    {
        Q_OBJECT

    public:
        EditorActionContext() = default;
        EditorActionContext(AZStd::string identifier, AZStd::string name, AZStd::string parentIdentifier, QWidget* widget);

        bool HasMode(const AZStd::string& modeIdentifier) const;
        void AddMode(AZStd::string modeIdentifier);

        AZStd::string GetActiveMode() const;
        bool SetActiveMode(AZStd::string modeIdentifier);

        void AddAction(AZStd::string actionIdentifier);
        void IterateActionIdentifiers(const AZStd::function<bool(const AZStd::string&)>& callback) const;

        QWidget* GetWidget();

    private:
        AZStd::string m_identifier;
        AZStd::string m_parentIdentifier;
        AZStd::string m_name;

        AZStd::string m_activeModeIdentifier = DefaultModeIdentifier;
        AZStd::unordered_set<AZStd::string> m_modeIdentifiers = { DefaultModeIdentifier };

        AZStd::unordered_set<AZStd::string> m_actionIdentifiers;

        QWidget* m_widget = nullptr;
    };

} // namespace AzToolsFramework
