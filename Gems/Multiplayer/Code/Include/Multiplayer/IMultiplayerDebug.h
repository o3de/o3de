/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/MathStringConversions.h>

namespace Multiplayer
{
    enum class MultiplayerAuditCategory
    {
        Desync,
        Input,
        Event
    };

    //! @class IMultiplayerAuditingDatum
    //! @brief IMultiplayerAuditingDatum provides an interface for auditing datums of a given auditing event
    class IMultiplayerAuditingDatum
    {
    public:
        virtual ~IMultiplayerAuditingDatum() = default;
        virtual IMultiplayerAuditingDatum& operator= (const IMultiplayerAuditingDatum&) { return *this; }

        virtual const AZStd::string& GetName() const = 0;
        virtual AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const = 0;
        virtual AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() = 0;
    };

    //! @class MultiplayerAuditingElement
    //! @brief MultiplayerAuditingElement contains a list of datums for a given auditing event
    class MultiplayerAuditingElement
    {
    public:
        MultiplayerAuditingElement() = default;
        MultiplayerAuditingElement(const MultiplayerAuditingElement& rhs)
        {
            name = rhs.name;
            elements.resize(rhs.elements.size());
            for (int32_t i = 0; i < rhs.elements.size(); ++i)
            {
                elements[i] = rhs.elements[i]->Clone();
            }
        }

        MultiplayerAuditingElement& operator= (const MultiplayerAuditingElement& rhs)
        {
            if (&rhs != this)
            {
                name = rhs.name;
                elements.resize(rhs.elements.size());
                for (int32_t i = 0; i < rhs.elements.size(); ++i)
                {
                    elements[i] = rhs.elements[i]->Clone();
                }
            }
            return *this;
        }

        AZStd::string name;
        AZStd::vector<AZStd::unique_ptr<IMultiplayerAuditingDatum>> elements;
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
            const MultiplayerAuditCategory category,
            const ClientInputId inputId,
            const HostFrameId frameId,
            const AZStd::string& name,
            AZStd::vector<MultiplayerAuditingElement>&& entryDetails) = 0;
    };

    #define AZ_MPAUDIT_INPUT_REWINDABLE(INPUT, REWINDABLE, VALUE_TYPE)                                                          \
    if (AZ::Interface<Multiplayer::IMultiplayerDebug>::Get())                                                                   \
    {                                                                                                                           \
        Multiplayer::MultiplayerAuditingElement detail;                                                                         \
        detail.name = INPUT.GetOwnerName();                                                                                     \
        detail.elements.push_back(AZStd::make_unique<Multiplayer::MultiplayerAuditingDatum<VALUE_TYPE>>(                        \
            #REWINDABLE, REWINDABLE.Get(), REWINDABLE.GetAuthority()));                                                         \
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->AddAuditEntry(                                                    \
            Multiplayer::MultiplayerAuditCategory::Event, INPUT.GetClientInputId(), INPUT.GetHostFrameId(),                     \
            AZStd::string::format("%s rewindable: %s", INPUT.GetOwnerName().c_str(), #REWINDABLE), { AZStd::move(detail) });    \
    }

    #define AZ_MPAUDIT_INPUT_VALUE(INPUT, VALUE, VALUE_TYPE)                                                                    \
    if (AZ::Interface<Multiplayer::IMultiplayerDebug>::Get())                                                                   \
    {                                                                                                                           \
        Multiplayer::MultiplayerAuditingElement detail;                                                                         \
        detail.name = INPUT.GetOwnerName();                                                                                     \
        detail.elements.push_back(AZStd::make_unique<Multiplayer::MultiplayerAuditingDatum<VALUE_TYPE>>(#VALUE, VALUE, VALUE)); \
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->AddAuditEntry(                                                    \
            Multiplayer::MultiplayerAuditCategory::Event, INPUT.GetClientInputId(), INPUT.GetHostFrameId(),                     \
            AZStd::string::format("%s: %s", INPUT.GetOwnerName().c_str(), #VALUE), { AZStd::move(detail) });                    \
    }
}
