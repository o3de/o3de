/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef _GAMEEFFECTSYSTEM_INTERFACE_H_
#define _GAMEEFFECTSYSTEM_INTERFACE_H_

#include <GameEffectSystem/GameEffects/IGameEffect.h>
#include <GameEffectSystem/GameEffectsSystemDefines.h>
#include <CryPodArray.h>
#include <AzCore/EBus/EBus.h>

class IGameEffectSystem;
struct ITypeLibrary;

/**
 * For requesting the GameEffectSystem.
 */
class GameEffectSystemRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    virtual IGameEffectSystem* GetIGameEffectSystem() = 0;
};
using GameEffectSystemRequestBus = AZ::EBus<GameEffectSystemRequests>;

/**
 * Dispatches notifications from the GameEffectSystem.
 */
class GameEffectSystemNotifications
    : public AZ::EBusTraits
{
public:
    /// Called when it's appropriate to release all registered GameEffects
    virtual void OnReleaseGameEffects() { }
};
using GameEffectSystemNotificationBus = AZ::EBus<GameEffectSystemNotifications>;

/// Returns global instance of IGameEffectSystem.
/// This function exists to support the legacy GAME_FX_SYSYTEM macro,
/// this is not the suggested way to fetch a singleton.
inline IGameEffectSystem& GetIGameEffectSystem()
{
    IGameEffectSystem* instance = nullptr;
    EBUS_EVENT_RESULT(instance, GameEffectSystemRequestBus, GetIGameEffectSystem);
    return *instance;
}

class IGameEffectSystem
{
public:
    virtual SC_API void RegisterEffect(IGameEffect* effect) = 0;
    virtual SC_API void UnRegisterEffect(IGameEffect* effect) = 0;

    virtual SC_API void GameRenderNodeInstanceReplaced(void* pOldInstance, void* pNewInstance) = 0;
    virtual SC_API void GameRenderElementInstanceReplaced(void* pOldInstance, void* pNewInstance) = 0;

#ifdef SOFTCODE_ENABLED
    // Create soft code instance using libs
    virtual SC_API void* CreateSoftCodeInstance(const char* pTypeName);
    // Register soft code lib for creation of instances
    virtual SC_API void RegisterSoftCodeLib(ITypeLibrary* pLib);
#endif

    SC_API static void RegisterEnteredGameCallback(EnteredGameCallback enteredGameCallback);

#ifdef DEBUG_GAME_FX_SYSTEM
    SC_API static void RegisterEffectDebugData(DebugOnInputEventCallback inputEventCallback,
        DebugDisplayCallback displayCallback,
        const char* effectName);
#endif//DEBUG_GAME_FX_SYSTEM
};

#if DEBUG_GAME_FX_SYSTEM
// Creating a static version of SRegisterEffectDebugData inside an effect cpp registers the
// effect's debug data with the game effects system
struct SRegisterEffectDebugData
{
    SRegisterEffectDebugData(DebugOnInputEventCallback inputEventCallback,
        DebugDisplayCallback debugDisplayCallback, const char* effectName)
    {
        IGameEffectSystem::RegisterEffectDebugData(inputEventCallback, debugDisplayCallback,
            effectName);
    }
};

struct SEffectDebugData
{
    SEffectDebugData(DebugOnInputEventCallback paramInputCallback,
        DebugDisplayCallback paramDisplayCallback, const char* paramEffectName)
    {
        inputCallback = paramInputCallback;
        displayCallback = paramDisplayCallback;
        effectName = paramEffectName;
    }
    DebugOnInputEventCallback inputCallback;
    DebugDisplayCallback displayCallback;
    const char* effectName;
};
#endif//DEBUG_GAME_FX_SYSTEM

// Creating a static version of SRegisterGameCallbacks inside an effect cpp registers the
// effect's game callback functions with the game effects system
struct SRegisterGameCallbacks
{
    SRegisterGameCallbacks(EnteredGameCallback enteredGameCallback)
    {
        IGameEffectSystem::RegisterEnteredGameCallback(enteredGameCallback);
    }
};

//--------------------------------------------------------------------------------------------------
// Desc: Game Effect System Static data - contains access to any data where static initialisation
//       order is critical, this will enforce initialisation on first use
//--------------------------------------------------------------------------------------------------
struct SGameEffectSystemStaticData
{
    static PodArray<EnteredGameCallback>& GetEnteredGameCallbackList()
    {
        static PodArray<EnteredGameCallback> enteredGameCallbackList;
        return enteredGameCallbackList;
    }

#if DEBUG_GAME_FX_SYSTEM
    static PodArray<SEffectDebugData>& GetEffectDebugList()
    {
        static PodArray<SEffectDebugData> effectDebugList;
        return effectDebugList;
    }
#endif//DEBUG_GAME_FX_SYSTEM
};
// Easy access macros
#define s_enteredGameCallbackList SGameEffectSystemStaticData::GetEnteredGameCallbackList()
#if DEBUG_GAME_FX_SYSTEM
#define s_effectDebugList SGameEffectSystemStaticData::GetEffectDebugList()
#endif//DEBUG_GAME_FX_SYSTEM

//--------------------------------------------------------------------------------------------------
// Name: RegisterEnteredGameCallback
// Desc: Registers entered game callback
//--------------------------------------------------------------------------------------------------
inline void IGameEffectSystem::RegisterEnteredGameCallback(EnteredGameCallback enteredGameCallback)
{
    if (enteredGameCallback)
    {
        s_enteredGameCallbackList.push_back(enteredGameCallback);
    }
} //-------------------------------------------------------------------------------------------------

#endif//_GAMEEFFECTSYSTEM_INTERFACE_H_
