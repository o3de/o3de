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
    private:
        AZStd::string name;
        AZStd::pair<T, T> clientServerValue;

    public:
        MultiplayerAuditingDatum(AZStd::string datumName)
            : name(datumName){};
        MultiplayerAuditingDatum(AZStd::string datumName, T client, T server)
            : name(datumName)
            , clientServerValue(client, server){};
        virtual ~MultiplayerAuditingDatum() = default;
        IMultiplayerAuditingDatum& operator=(const IMultiplayerAuditingDatum& rhs) override;

        const AZStd::string& GetName() const override;
        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override;

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override;
    };

    template<>
    class MultiplayerAuditingDatum<bool> : public IMultiplayerAuditingDatum
    {
    private:
        AZStd::string name;
        AZStd::pair<bool, bool> clientServerValue;

    public:
        MultiplayerAuditingDatum(AZStd::string datumName)
            : name(datumName){};
        MultiplayerAuditingDatum(AZStd::string datumName, bool client, bool server)
            : name(datumName)
            , clientServerValue(client, server){};
        virtual ~MultiplayerAuditingDatum() = default;
        IMultiplayerAuditingDatum& operator=(const IMultiplayerAuditingDatum& rhs) override;

        const AZStd::string& GetName() const override;
        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override;

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override;
    };

    template<>
    class MultiplayerAuditingDatum<AZStd::string> : public IMultiplayerAuditingDatum
    {
    private:
        AZStd::string name;
        AZStd::pair<AZStd::string, AZStd::string> clientServerValue;

    public:
        MultiplayerAuditingDatum(AZStd::string datumName)
            : name(datumName){};
        MultiplayerAuditingDatum(AZStd::string datumName, AZStd::string client, AZStd::string server)
            : name(datumName)
            , clientServerValue(client, server){};
        virtual ~MultiplayerAuditingDatum() = default;
        IMultiplayerAuditingDatum& operator=(const IMultiplayerAuditingDatum& rhs) override;

        const AZStd::string& GetName() const override;

        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override;

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override;
        
    };
}

#include <Multiplayer/MultiplayerDebug.inl>
