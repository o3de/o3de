/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/MultiplayerTypes.h>
#include <AzNetworking/DataStructures/FixedSizeBitset.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzNetworking
{
    class ISerializer;
}

namespace Multiplayer
{
    enum class MultiplayerAuditCategory
    {
        MP_AUDIT_DESYNC,
        MP_AUDIT_INPUT,
        MP_AUDIT_EVENT
    };

    class IMultiplayerAuditingDatum
    {
    public:
        virtual ~IMultiplayerAuditingDatum() = default;
        virtual IMultiplayerAuditingDatum& operator= (const IMultiplayerAuditingDatum&) { return *this; }

        virtual const AZStd::string& GetName() const = 0;
        virtual AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const = 0;
        virtual AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() = 0;
    };

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
        Multiplayer::IMultiplayerAuditingDatum& operator =(const Multiplayer::IMultiplayerAuditingDatum& rhs) override
        {
            *this = *static_cast<const MultiplayerAuditingDatum*>(&rhs);
            return *this;
        }

        const AZStd::string& GetName() const override
        {
            return name;
        }

        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override
        {
            return AZStd::pair<AZStd::string, AZStd::string>(
                AZStd::to_string(clientServerValue.first), AZStd::to_string(clientServerValue.second));
        };

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override
        {
            return AZStd::make_unique<MultiplayerAuditingDatum>(*this);
        }
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
        Multiplayer::IMultiplayerAuditingDatum& operator =(const Multiplayer::IMultiplayerAuditingDatum& rhs) override
        {
            *this = *static_cast<const MultiplayerAuditingDatum*>(&rhs);
            return *this;
        }

        const AZStd::string& GetName() const override
        {
            return name;
        }

        AZStd::pair<AZStd::string, AZStd::string> GetClientServerValues() const override
        {
            return AZStd::pair<AZStd::string, AZStd::string>(
                clientServerValue.first, clientServerValue.second);
        };

        AZStd::unique_ptr<IMultiplayerAuditingDatum> Clone() override
        {
            return AZStd::make_unique<MultiplayerAuditingDatum>(*this);
        }
        
    };

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

    class IMultiplayerComponentInput
    {
    public:
        virtual ~IMultiplayerComponentInput() = default;
        virtual NetComponentId GetNetComponentId() const = 0;
        virtual bool Serialize(AzNetworking::ISerializer& serializer) = 0;
        virtual MultiplayerAuditingElement GetInputDeltaLog() const = 0;
        virtual IMultiplayerComponentInput& operator= (const IMultiplayerComponentInput&) { return *this; }
    };

    using MultiplayerComponentInputVector = AZStd::vector<AZStd::unique_ptr<IMultiplayerComponentInput>>;
}
