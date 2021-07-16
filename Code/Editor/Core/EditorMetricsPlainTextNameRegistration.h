/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzCore/std/containers/set.h>

namespace Editor
{
    class EditorMetricsPlainTextNameRegistrationBusListener : public AzFramework::MetricsPlainTextNameRegistrationBus::Handler
    {
    public:
        EditorMetricsPlainTextNameRegistrationBusListener();
        virtual ~EditorMetricsPlainTextNameRegistrationBusListener();

        void RegisterForNameSending(const AZStd::vector<AZ::Uuid>& typeIdsThatCanBeSentAsPlainText) override;

        bool IsTypeRegisteredForNameSending(const AZ::Uuid& typeId) override;

    private:
        AZStd::set<AZ::Uuid> m_registeredTypeIds;
    };

} // namespace editor
