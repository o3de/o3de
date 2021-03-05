/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
