/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        AZ::u32 GetNumActivePopups() const { return m_currentPopups.size(); }

    protected:
        //////////////////////////////////////////////////////////////////////////
        // TickBus
        virtual void OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint) override;
    private:
        typedef AZStd::map<AZ::u32, MessagePopupInfo*> CurrentPopupsMap;
        CurrentPopupsMap m_currentPopups;
    };
}
