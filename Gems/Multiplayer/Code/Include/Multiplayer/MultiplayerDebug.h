/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/MathStringConversions.h>
#include <Multiplayer/IMultiplayerDebug.h>

namespace Multiplayer
{
    template<class T>
    class MultiplayerAuditingDatum : public IMultiplayerAuditingDatum
    {
    public:
        MultiplayerAuditingDatum(AZStd::string datumName)
            : m_name(datumName){};
        MultiplayerAuditingDatum(AZStd::string datumName, T client, T server)
            : m_name(datumName)
            , m_clientServerValue(client, server){};
        virtual ~MultiplayerAuditingDatum() = default;
        IMultiplayerAuditingDatum& operator=(const IMultiplayerAuditingDatum& rhs) override;

        const AZStd::string& GetName() const override;
        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override;

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override;

    private:
        AZStd::string m_name;
        AZStd::pair<T, T> m_clientServerValue;
    };

    template<>
    class MultiplayerAuditingDatum<bool> : public IMultiplayerAuditingDatum
    {
    public:
        MultiplayerAuditingDatum(AZStd::string datumName)
            : m_name(datumName){};
        MultiplayerAuditingDatum(AZStd::string datumName, bool client, bool server)
            : m_name(datumName)
            , m_clientServerValue(client, server){};
        virtual ~MultiplayerAuditingDatum() = default;
        IMultiplayerAuditingDatum& operator=(const IMultiplayerAuditingDatum& rhs) override;

        const AZStd::string& GetName() const override;
        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override;

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override;

    private:
        AZStd::string m_name;
        AZStd::pair<bool, bool> m_clientServerValue;
    };

    template<>
    class MultiplayerAuditingDatum<AZStd::string> : public IMultiplayerAuditingDatum
    {
    public:
        MultiplayerAuditingDatum(AZStd::string datumName)
            : m_name(datumName){};
        MultiplayerAuditingDatum(AZStd::string datumName, AZStd::string client, AZStd::string server)
            : m_name(datumName)
            , m_clientServerValue(client, server){};
        virtual ~MultiplayerAuditingDatum() = default;
        IMultiplayerAuditingDatum& operator=(const IMultiplayerAuditingDatum& rhs) override;

        const AZStd::string& GetName() const override;

        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override;

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override;

    private:
        AZStd::string m_name;
        AZStd::pair<AZStd::string, AZStd::string> m_clientServerValue;
    };

    //! Adds an audit trail entry detailing the local value of a rewindable and its last known server value against a given NetworkInput
    //! @param INPUT The network input to audit
    //! @param REWINDABLE The RewindableObject to audit
    //! @param VALUE_TYPE The underlying type of the RewindableObject
    #define AZ_MPAUDIT_INPUT_REWINDABLE(INPUT, REWINDABLE, VALUE_TYPE)                                                                     \
    if (AZ::Interface<Multiplayer::IMultiplayerDebug>::Get())                                                                              \
    {                                                                                                                                      \
        Multiplayer::MultiplayerAuditingElement detail;                                                                                    \
        detail.m_name = INPUT.GetOwnerName().empty() ? "null owner" : INPUT.GetOwnerName();                                                \
        detail.m_elements.push_back(AZStd::make_unique<Multiplayer::MultiplayerAuditingDatum<VALUE_TYPE>>(                                 \
            #REWINDABLE, REWINDABLE.Get(), REWINDABLE.GetLastSerializedValue()));                                                          \
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->AddAuditEntry(                                                               \
            Multiplayer::AuditCategory::Event, INPUT.GetClientInputId(), INPUT.GetHostFrameId(),                                           \
            AZStd::string::format("%s rewindable: %s", INPUT.GetOwnerName().c_str(), #REWINDABLE), { AZStd::move(detail) });               \
    }

    //! Adds an audit trail entry detailing the value of a given variable against a given NetworkInput
    //! @param INPUT The network input to audit
    //! @param VALUE The variable to audit
    //! @param VALUE_TYPE The type of the variable
    #define AZ_MPAUDIT_INPUT_VALUE(INPUT, VALUE, VALUE_TYPE)                                                                               \
    if (AZ::Interface<Multiplayer::IMultiplayerDebug>::Get())                                                                              \
    {                                                                                                                                      \
        Multiplayer::MultiplayerAuditingElement detail;                                                                                    \
        detail.m_name = INPUT.GetOwnerName().empty() ? "null owner" : INPUT.GetOwnerName();                                                \
        detail.m_elements.push_back(AZStd::make_unique<Multiplayer::MultiplayerAuditingDatum<VALUE_TYPE>>(#VALUE, VALUE, VALUE));          \
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->AddAuditEntry(                                                               \
            Multiplayer::AuditCategory::Event, INPUT.GetClientInputId(), INPUT.GetHostFrameId(),                                           \
            AZStd::string::format("%s: %s", INPUT.GetOwnerName().c_str(), #VALUE), { AZStd::move(detail) });                               \
    }

    //! Adds an audit trail entry detailing the value of a given variable
    //! @param VALUE The variable to audit
    //! @param VALUE_TYPE The type of the variable
    #define AZ_MPAUDIT_VALUE(VALUE, VALUE_TYPE)                                                                                            \
    if (AZ::Interface<Multiplayer::IMultiplayerDebug>::Get())                                                                              \
    {                                                                                                                                      \
        Multiplayer::MultiplayerAuditingElement detail;                                                                                    \
        detail.m_name = AZStd::string::format("%s", #VALUE);                                                                               \
        detail.m_elements.push_back(AZStd::make_unique<Multiplayer::MultiplayerAuditingDatum<VALUE_TYPE>>(#VALUE, VALUE, VALUE));          \
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->AddAuditEntry(                                                               \
            Multiplayer::AuditCategory::Event, Multiplayer::ClientInputId(0), Multiplayer::HostFrameId(0),                                 \
            detail.m_name, { AZStd::move(detail) });                               \
    }
}

#include <Multiplayer/MultiplayerDebug.inl>
