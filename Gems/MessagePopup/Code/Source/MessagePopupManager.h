/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <MessagePopup/MessagePopupBus.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Component/TickBus.h>

namespace MessagePopup
{
    //////////////////////////////////////////////////////////////////////////
    class MessagePopupManager : 
        public AZ::TickBus::Handler
    {
    public:
        MessagePopupManager();

        bool SetPopupData(AZ::u32 _popupID, void *_clientID, AZStd::function<void(int _button)> _callback, float _showTime);
        AZ::u32 CreatePopup();
        bool RemovePopup(AZ::u32 _popupID);
        void* GetPopupClientData(AZ::u32 _popupID);
        MessagePopupInfo* GetPopupInfo(AZ::u32 _popupID);
        AZ::u32 GetNumActivePopups() const { return static_cast<AZ::u32>(m_currentPopups.size()); }

    protected:
        //////////////////////////////////////////////////////////////////////////
        // TickBus
        virtual void OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint) override;
    private:
        typedef AZStd::map<AZ::u32, MessagePopupInfo*> CurrentPopupsMap;
        CurrentPopupsMap m_currentPopups;
    };
}
