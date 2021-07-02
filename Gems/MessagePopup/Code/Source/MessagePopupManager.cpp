/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MessagePopup_precompiled.h>

#include <MessagePopupManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/EBus/EBus.h>

namespace MessagePopup
{
    // Global dynamic unique identifier factory. One ID for each popup
    static AZ::u32 g_GlobalUniqueIDBank = 1;
    
    //-------------------------------------------------------------------------
    MessagePopupManager::MessagePopupManager()
    {
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupManager::CreatePopup()
    {
        AZ::u32 thisID = g_GlobalUniqueIDBank;  // do it need to be thread safe?
        g_GlobalUniqueIDBank++;

        m_currentPopups[thisID] = new MessagePopupInfo();

        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
        return thisID;
    }

    //-------------------------------------------------------------------------
    bool MessagePopupManager::SetPopupData(AZ::u32 _popupID, void *_clientID, AZStd::function<void(int _button)> _callback, float _showTime)
    {
        if (m_currentPopups.find(_popupID) != m_currentPopups.end())
        {
            m_currentPopups[_popupID]->SetData(_clientID, _callback,  _showTime);
            return true;
        }
        return false;
    }

   //-------------------------------------------------------------------------
   bool MessagePopupManager::RemovePopup(AZ::u32 _popupID)
    {
        CurrentPopupsMap::iterator iter = m_currentPopups.find(_popupID);
        bool retVal = false;
        if (iter != m_currentPopups.end())
        {
            delete iter->second;
            m_currentPopups.erase(_popupID);
            retVal = true;
        }

        if (m_currentPopups.size() == 0)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        return retVal;
    }

   //-------------------------------------------------------------------------
   void* MessagePopupManager::GetPopupClientData(AZ::u32 _popupID)
   {
       if (m_currentPopups.find(_popupID) != m_currentPopups.end())
       {
           return m_currentPopups[_popupID]->m_clientData;
       }
       return nullptr;
   }

   //-------------------------------------------------------------------------
   MessagePopupInfo* MessagePopupManager::GetPopupInfo(AZ::u32 _popupID)
   {
       if (m_currentPopups.find(_popupID) != m_currentPopups.end())
       {
           return m_currentPopups[_popupID];
       }
       return nullptr;
   }

   //-------------------------------------------------------------------------
   void MessagePopupManager::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint scriptTimePoint)
   {
       for (CurrentPopupsMap::iterator iter = m_currentPopups.begin(); iter != m_currentPopups.end(); )
       {
           if ((*iter).second->m_showTime > 0.0f)
           {
               (*iter).second->m_showTime -= deltaTime;
               if ((*iter).second->m_showTime <= 0.0f)
               {
                   AZ::u32 thisID = (*iter).first;
                   ++iter; // Get the next now since HidePopup will remove the popup from the list. (avoiding the creation of a new list to be processed after) 
                   MessagePopup::MessagePopupRequestBus::Broadcast(&MessagePopup::MessagePopupRequestBus::Events::HidePopup, thisID, 0);
                   continue;
               }
           }
           ++iter;
       }
   }
}
