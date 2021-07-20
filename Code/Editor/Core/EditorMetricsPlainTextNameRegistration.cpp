/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorMetricsPlainTextNameRegistration.h"

namespace Editor
{

EditorMetricsPlainTextNameRegistrationBusListener::EditorMetricsPlainTextNameRegistrationBusListener()
{
    AzFramework::MetricsPlainTextNameRegistrationBus::Handler::BusConnect();
}

EditorMetricsPlainTextNameRegistrationBusListener::~EditorMetricsPlainTextNameRegistrationBusListener()
{
    AzFramework::MetricsPlainTextNameRegistrationBus::Handler::BusDisconnect();
}

void EditorMetricsPlainTextNameRegistrationBusListener::RegisterForNameSending(const AZStd::vector<AZ::Uuid>& typeIdsThatCanBeSentAsPlainText)
{
    m_registeredTypeIds.insert(typeIdsThatCanBeSentAsPlainText.begin(), typeIdsThatCanBeSentAsPlainText.end());
}

bool EditorMetricsPlainTextNameRegistrationBusListener::IsTypeRegisteredForNameSending(const AZ::Uuid& typeId)
{
    return (m_registeredTypeIds.find(typeId) != m_registeredTypeIds.end());
}

} // namespace editor
