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
}

#include <Multiplayer/MultiplayerDebug.inl>
