/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/MathStringConversions.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace Multiplayer
{
    //! Categories for Auditing logs used in MultiplayerDebugAuditTrail
    enum class AuditCategory
    {
        Desync,
        Input,
        Event
    };

    //! @class IMultiplayerAuditingDatum
    //! @brief IMultiplayerAuditingDatum provides an interface for datums of a given auditing event
    class IMultiplayerAuditingDatum
    {
    public:
        virtual ~IMultiplayerAuditingDatum() = default;
        virtual IMultiplayerAuditingDatum& operator= (const IMultiplayerAuditingDatum&) { return *this; }

        //! Retrieves the name of the auditing datum
        virtual const AZStd::string& GetName() const = 0;

        //! Retrieves the Client and Server values of the datum as strings
        virtual AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const = 0;

        //! Clones the Datum to a new unique_ptr
        virtual AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() = 0;
    };

    //! @struct MultiplayerAuditingElement
    //! @brief MultiplayerAuditingElement contains a list of datums for a given auditing event
    struct MultiplayerAuditingElement
    {
    public:
        MultiplayerAuditingElement() = default;
        MultiplayerAuditingElement(const MultiplayerAuditingElement& rhs)
        {
            m_name = rhs.m_name;
            m_elements.resize(rhs.m_elements.size());
            for (int32_t i = 0; i < rhs.m_elements.size(); ++i)
            {
                m_elements[i] = rhs.m_elements[i]->Clone();
            }
        }

        MultiplayerAuditingElement& operator= (const MultiplayerAuditingElement& rhs)
        {
            if (&rhs != this)
            {
                m_name = rhs.m_name;
                m_elements.resize(rhs.m_elements.size());
                for (int32_t i = 0; i < rhs.m_elements.size(); ++i)
                {
                    m_elements[i] = rhs.m_elements[i]->Clone();
                }
            }
            return *this;
        }

        AZStd::string m_name;
        AZStd::vector<AZStd::unique_ptr<IMultiplayerAuditingDatum>> m_elements;
    };

    //! @class IMultiplayerDebug
    //! @brief IMultiplayerDebug provides access to multiplayer debug overlays
    class IMultiplayerDebug
    {
    public:
        AZ_RTTI(IMultiplayerDebug, "{C5EB7F3A-E19F-4921-A604-C9BDC910123C}");

        virtual ~IMultiplayerDebug() = default;

        //! Enables printing of debug text over entities that have significant amount of traffic.
        virtual void ShowEntityBandwidthDebugOverlay() = 0;

        //! Disables printing of debug text over entities that have significant amount of traffic.
        virtual void HideEntityBandwidthDebugOverlay() = 0;

        //! Adds a string based entry to the Multiplayer audit trail
        virtual void AddAuditEntry(
            const AuditCategory category,
            const ClientInputId inputId,
            const HostFrameId frameId,
            const AZStd::string& name,
            AZStd::vector<MultiplayerAuditingElement>&& entryDetails) = 0;
    };
}
