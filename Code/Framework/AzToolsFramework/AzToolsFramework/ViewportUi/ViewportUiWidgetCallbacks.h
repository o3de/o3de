/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/function/function_template.h>

#include <QMetaMethod>
#include <QObject>
#include <QPointer>

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Helper class to manage widgets and update them simultaneously.
    class ViewportUiWidgetCallbacks
    {
    public:
        ViewportUiWidgetCallbacks() = default;
        ~ViewportUiWidgetCallbacks() = default;

        void AddWidget(QPointer<QObject> widget, const AZStd::function<void(QPointer<QObject>)>& updateCallback = {});
        void RemoveWidget(QPointer<QObject> widget);
        //! Must call ViewportUiWidgetCallbacks::Update to execute the callback.
        void RegisterUpdateCallback(QPointer<QObject> widget, const AZStd::function<void(QPointer<QObject>)>& callback);
        void Update();

        const AZStd::vector<QPointer<QObject>>& GetWidgets() const;

    protected:
        //! A map of all update callbacks and their respective widgets.
        //! Note: key is kept as QObject* since QPointer cannot be implicitly hashed.
        AZStd::unordered_map<QObject*, AZStd::function<void(QPointer<QObject>)>> m_updateCallbacks;
        AZStd::vector<QPointer<QObject>> m_widgets;
    };

    inline const AZStd::vector<QPointer<QObject>>& ViewportUiWidgetCallbacks::GetWidgets() const
    {
        return m_widgets;
    }
} // namespace AzToolsFramework::ViewportUi::Internal
