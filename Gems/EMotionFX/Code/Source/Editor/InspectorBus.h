/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <QtGlobal>
#include <QString>
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace EMStudio
{
    class InspectorRequests
        : public AZ::EBusTraits
    {
    public:
        // Call in case a fully customized widget shall be shown in the inspector. The header widget will be shown above the given widget.
        // NOTE: The inspector will NOT take ownership of the widget and it is your responsibility to destruct it along with the owning plugin.
        virtual void UpdateWithHeader([[maybe_unused]] const QString& headerTitle, [[maybe_unused]] const QString& iconFilename, [[maybe_unused]] QWidget* widget) = 0;

        // Call in case a fully customized widget shall be shown in the inspector without the header.
        // NOTE: The inspector will NOT take ownership of the widget and it is your responsibility to destruct it along with the owning plugin.
        virtual void Update(QWidget* widget) = 0;

        // Call in case the objects to be inspected are reflected. This creates a card with a reflected property editor inside in the standard way for each object along with a header widget above.
        // Use this method whenever a single, reflected object shall be visible in the inspector.
        struct CardElement
        {
            void* m_object = nullptr;
            AZ::TypeId m_objectTypeId;
            QString m_cardName;
            QWidget* m_customWidget = nullptr;
        };
        virtual void UpdateWithRpe([[maybe_unused]] const QString& headerTitle, [[maybe_unused]] const QString& iconFilename, [[maybe_unused]] const AZStd::vector<CardElement>& cardElements) = 0;

        // Call in case the inspected object got removed or unselected. This will show the no selection widget in the inspector.
        virtual void Clear() = 0;

        // Clear the inspector and show the default no selection widget in case the given widget is currently shown.
        virtual void ClearIfShown(QWidget* widget) = 0;
    };

    using InspectorRequestBus = AZ::EBus<InspectorRequests>;


    class InspectorNotifications
        : public AZ::EBusTraits
    {
    public:
    };

    using InspectorNotificationBus = AZ::EBus<InspectorNotifications>;
} // namespace EMStudio
