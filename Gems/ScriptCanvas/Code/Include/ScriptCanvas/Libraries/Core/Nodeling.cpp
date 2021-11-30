


/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FunctionDefinitionNode.h"
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Core/Contracts/DisplayGroupConnectedSlotLimitContract.h>
#include <ScriptCanvas/Libraries/Core/FunctionBus.h>

#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidPropertyEvent.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                /////////////
                // Nodeling
                /////////////

                Nodeling::Nodeling()
                    : m_identifier(AZ::Uuid::CreateRandom())
                    , m_displayName(" ")
                {
                }

                void Nodeling::OnInit()
                {
                    m_displayNameInterface.SetPropertyReference(&m_displayName);
                    m_displayNameInterface.RegisterListener(this);

                    if (GetOwningScriptCanvasId().IsValid())
                    {
                        NodelingRequestBus::Handler::BusConnect(GetScopedNodeId());
                    }

                    m_previousName = m_displayName;
                }

                void Nodeling::OnGraphSet()
                {
                    if (GetEntity() != nullptr)
                    {
                        NodelingRequestBus::Handler::BusConnect(GetScopedNodeId());
                    }
                }

                void Nodeling::ConfigureVisualExtensions()
                {
                    {
                        VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);

                        visualExtensions.m_name = "";
                        visualExtensions.m_tooltip = "";

                        // Should be centered. But we'll sort that out in specialized display pass.
                        visualExtensions.m_connectionType = ConnectionType::Input;

                        visualExtensions.m_identifier = GetPropertyId();

                        RegisterExtension(visualExtensions);
                    }
                }

                NodePropertyInterface* Nodeling::GetPropertyInterface(AZ::Crc32 propertyId)
                {
                    if (propertyId == GetPropertyId())
                    {
                        return &m_displayNameInterface;
                    }

                    return nullptr;
                }

                const AZStd::string& Nodeling::GetDisplayName() const
                {
                    return m_displayName;
                }

                void Nodeling::SetDisplayName(const AZStd::string& displayName)
                {
                    m_displayName = displayName;

                    m_displayNameInterface.SignalDataChanged();
                    OnDisplayNameChanged();
                }

                void Nodeling::OnDisplayNameChanged()
                {
                }

                void Nodeling::OnPropertyChanged()
                {
                    if (m_displayName.empty())
                    {
                        m_displayName = m_previousName;

                        if (!m_previousName.empty())
                        {
                            m_displayNameInterface.SignalDataChanged();
                        }

                        OnDisplayNameChanged();
                        return;
                    }
                    else
                    {
                        m_previousName = m_displayName;
                    }

                    OnDisplayNameChanged();
                    NodelingNotificationBus::Event(GetScopedNodeId(), &NodelingNotifications::OnNameChanged, m_displayName);
                }

                void Nodeling::RemapId()
                {
                    m_identifier = AZ::Uuid::CreateRandom();
                }
            }
        }
    }
}
