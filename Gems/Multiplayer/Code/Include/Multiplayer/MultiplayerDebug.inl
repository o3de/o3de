/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace Multiplayer
{
    template<class T>
    inline Multiplayer::IMultiplayerAuditingDatum& MultiplayerAuditingDatum<T>::operator =(const Multiplayer::IMultiplayerAuditingDatum& rhs)
    {
        *this = *static_cast<const MultiplayerAuditingDatum*>(&rhs);
        return *this;
    }

    template<class T>
    inline const AZStd::string& MultiplayerAuditingDatum<T>::GetName() const
    {
        return m_name;
    }

    template<class T>
    inline AZStd::pair<AZStd::string, AZStd::string> MultiplayerAuditingDatum<T>::GetClientServerValues() const
    {
        return AZStd::pair<AZStd::string, AZStd::string>(
            AZStd::to_string(m_clientServerValue.first), AZStd::to_string(m_clientServerValue.second));
    }

    template<class T>
    inline AZStd::unique_ptr<IMultiplayerAuditingDatum> MultiplayerAuditingDatum<T>::Clone()
    {
        return AZStd::make_unique<MultiplayerAuditingDatum>(*this);
    }

    inline Multiplayer::IMultiplayerAuditingDatum& MultiplayerAuditingDatum<bool>::operator=(
        const Multiplayer::IMultiplayerAuditingDatum& rhs)
    {
        *this = *static_cast<const MultiplayerAuditingDatum*>(&rhs);
        return *this;
    }

    inline const AZStd::string& MultiplayerAuditingDatum<bool>::GetName() const
    {
        return m_name;
    }

    inline AZStd::pair<AZStd::string, AZStd::string> MultiplayerAuditingDatum<bool>::GetClientServerValues() const
    {
        return AZStd::pair<AZStd::string, AZStd::string>(
            m_clientServerValue.first ? "true" : "false", m_clientServerValue.second ? "true" : "false");
    }

    inline AZStd::unique_ptr<IMultiplayerAuditingDatum> MultiplayerAuditingDatum<bool>::Clone()
    {
        return AZStd::make_unique<MultiplayerAuditingDatum>(*this);
    }

    inline Multiplayer::IMultiplayerAuditingDatum& MultiplayerAuditingDatum<AZStd::string>::operator=(
        const Multiplayer::IMultiplayerAuditingDatum& rhs)
    {
        *this = *static_cast<const MultiplayerAuditingDatum*>(&rhs);
        return *this;
    }

    inline const AZStd::string& MultiplayerAuditingDatum<AZStd::string>::GetName() const
    {
        return m_name;
    }

    inline AZStd::pair<AZStd::string, AZStd::string> MultiplayerAuditingDatum<AZStd::string>::GetClientServerValues() const
    {
        return AZStd::pair<AZStd::string, AZStd::string>(m_clientServerValue.first, m_clientServerValue.second);
    }

    inline AZStd::unique_ptr<IMultiplayerAuditingDatum> MultiplayerAuditingDatum<AZStd::string>::Clone()
    {
        return AZStd::make_unique<MultiplayerAuditingDatum>(*this);
    }
}
