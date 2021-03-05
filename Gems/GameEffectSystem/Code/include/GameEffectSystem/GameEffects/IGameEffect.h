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
#ifndef _GAMEEFFECT_INTERFACE_H_
#define _GAMEEFFECT_INTERFACE_H_

#include <TypeLibrary.h>

//==================================================================================================
// Name: EGameEffectFlags
// Desc: Game effect flags
// Author: James Chilvers
//==================================================================================================
enum EGameEffectFlags
{
    GAME_EFFECT_INITIALISED = (1 << 0),
    GAME_EFFECT_RELEASED = (1 << 1),
    GAME_EFFECT_AUTO_RELEASE = (1 << 2), // Release called when Game Effect System is destroyed
    GAME_EFFECT_AUTO_DELETE = (1 << 3),  // Delete is called when Game Effect System is destroyed
    GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE = (1 << 4),
    GAME_EFFECT_AUTO_UPDATES_WHEN_NOT_ACTIVE = (1 << 5),
    GAME_EFFECT_REGISTERED = (1 << 6),
    GAME_EFFECT_ACTIVE = (1 << 7),
    GAME_EFFECT_DEBUG_EFFECT = (1 << 8), // Set true for any debug effects to avoid confusion
    GAME_EFFECT_UPDATE_WHEN_PAUSED = (1 << 9),
    GAME_EFFECT_RELEASING = (1 << 10)
}; //-----------------------------------------------------------------------------------------------

//==================================================================================================
// Name: SGameEffectParams
// Desc: Game effect parameters
// Author: James Chilvers
//==================================================================================================
struct SGameEffectParams
{
    friend class CGameEffect;

    // Make constructor private to stop SGameEffectParams ever being created, should always inherit
    // this
    // for each effect to avoid casting problems
protected:
    SGameEffectParams()
    {
        autoUpdatesWhenActive = true;
        autoUpdatesWhenNotActive = false;
        autoRelease = false;
        autoDelete = false;
    }

public:
    bool autoUpdatesWhenActive;
    bool autoUpdatesWhenNotActive;
    bool autoRelease; // Release called when Game Effect System is destroyed
    bool autoDelete;  // Delete is called when Game Effect System is destroyed
}; //-----------------------------------------------------------------------------------------------

//==================================================================================================
// Name: IGameEffect
// Desc: Interface for all game effects
// Author: James Chilvers
//==================================================================================================
struct IGameEffect
{
    DECLARE_TYPELIB(IGameEffect); // Allow soft coding on this interface

    friend class CGameEffectsSystem;

public:
    virtual ~IGameEffect() {}

    virtual void Initialize(const SGameEffectParams* gameEffectParams = NULL) = 0;
    virtual void Release() = 0;
    virtual void Update(float frameTime) = 0;

    virtual void SetActive(bool isActive) = 0;

    virtual void SetFlag(uint32 flag, bool state) = 0;
    virtual bool IsFlagSet(uint32 flag) const = 0;
    virtual uint32 GetFlags() const = 0;
    virtual void SetFlags(uint32 flags) = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual const char* GetName() const = 0;

    virtual void UnloadData() = 0;

private:
    virtual IGameEffect* Next() const = 0;
    virtual IGameEffect* Prev() const = 0;
    virtual void SetNext(IGameEffect* newNext) = 0;
    virtual void SetPrev(IGameEffect* newPrev) = 0;
}; //-----------------------------------------------------------------------------------------------

#endif//_GAMEEFFECT_INTERFACE_H_
