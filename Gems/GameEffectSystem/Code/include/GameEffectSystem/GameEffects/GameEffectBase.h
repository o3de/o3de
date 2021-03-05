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
#ifndef _GAMEEFFECTBASE_H_
#define _GAMEEFFECTBASE_H_

#include <GameEffectSystem/GameEffects/IGameEffect.h>
#include <GameEffectSystem/GameEffectsSystemDefines.h>
#include "TypeLibrary.h"

// Forward declares
struct SGameEffectParams;

//==================================================================================================
// Name: Flag macros
// Desc: Flag macros to make code more readable
// Author: James Chilvers
//==================================================================================================
#define SET_FLAG(currentFlags, flag, state) ((state) ? (currentFlags |= flag) : (currentFlags &= ~flag));
#define IS_FLAG_SET(currentFlags, flag) ((currentFlags & flag) ? true : false)
//--------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CGameEffect
// Desc: Game effect - Ideal for handling a specific visual game feature
// Author: James Chilvers
//==================================================================================================
class CGameEffect
    : public IGameEffect
{
    DECLARE_TYPE(CGameEffect, IGameEffect); // Exposes this type for SoftCoding

public:
    CGameEffect();
    virtual ~CGameEffect();

    void Initialize(const SGameEffectParams* gameEffectParams = NULL) override;
    void Release() override;
    void Update(float frameTime) override;

    void SetActive(bool isActive) override;

    void SetFlag(uint32 flag, bool state) override { SET_FLAG(m_flags, flag, state); }
    bool IsFlagSet(uint32 flag) const override { return IS_FLAG_SET(m_flags, flag); }
    uint32 GetFlags() const override { return m_flags; }
    void SetFlags(uint32 flags) override { m_flags = flags; }

    void GetMemoryUsage(ICrySizer* pSizer) const override { pSizer->AddObject(this, sizeof(*this)); }

    void UnloadData() override { }

protected:
    // General data functions
    static _smart_ptr<IMaterial> LoadMaterial(const char* pMaterialName);

private:
    IGameEffect* Next() const override { return m_next; }
    IGameEffect* Prev() const override { return m_prev; }
    void SetNext(IGameEffect* newNext) override { m_next = newNext; }
    void SetPrev(IGameEffect* newPrev) override { m_prev = newPrev; }

    IGameEffect* m_prev;
    IGameEffect* m_next;
    uint16 m_flags;
    IGameEffectSystem* m_gameEffectSystem = nullptr;

#if DEBUG_GAME_FX_SYSTEM
    CryFixedStringT<32> m_debugName;
#endif
}; //-----------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: CGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
inline CGameEffect::CGameEffect()
{
    m_prev = NULL;
    m_next = NULL;
    m_flags = 0;
    EBUS_EVENT_RESULT(m_gameEffectSystem, GameEffectSystemRequestBus, GetIGameEffectSystem);
} //------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
inline CGameEffect::~CGameEffect()
{
#if DEBUG_GAME_FX_SYSTEM
    // Output message if effect hasn't been released before being deleted
    const bool bEffectIsReleased =
        (m_flags & GAME_EFFECT_RELEASED) ||     // -> Needs to be released before deleted
        !(m_flags & GAME_EFFECT_INITIALISED) || // -> Except when not initialised
        (gEnv->IsEditor()); // -> Or the editor (memory safely released by editor)
    if (!bEffectIsReleased)
    {
        string dbgMessage = m_debugName + " being destroyed without being released first";
        FX_ASSERT_MESSAGE(bEffectIsReleased, dbgMessage.c_str());
    }
#endif

    if (m_gameEffectSystem)
    {
        // -> Effect should have been released and been unregistered, but to avoid
        //    crashes call unregister here too
        m_gameEffectSystem->UnRegisterEffect(this);
    }
} //------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes game effect
//--------------------------------------------------------------------------------------------------
inline void CGameEffect::Initialize(const SGameEffectParams* gameEffectParams)
{
#if DEBUG_GAME_FX_SYSTEM
    m_debugName = GetName(); // Store name so it can be accessed in destructor and debugging
#endif

    if (!IsFlagSet(GAME_EFFECT_INITIALISED))
    {
        SGameEffectParams params;
        if (gameEffectParams)
        {
            params = *gameEffectParams;
        }

        SetFlag(GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE, params.autoUpdatesWhenActive);
        SetFlag(GAME_EFFECT_AUTO_UPDATES_WHEN_NOT_ACTIVE, params.autoUpdatesWhenNotActive);
        SetFlag(GAME_EFFECT_AUTO_RELEASE, params.autoRelease);
        SetFlag(GAME_EFFECT_AUTO_DELETE, params.autoDelete);

        m_gameEffectSystem->RegisterEffect(this);

        SetFlag(GAME_EFFECT_INITIALISED, true);
        SetFlag(GAME_EFFECT_RELEASED, false);
    }
} //------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Release
// Desc: Releases game effect
//--------------------------------------------------------------------------------------------------
inline void CGameEffect::Release()
{
    SetFlag(GAME_EFFECT_RELEASING, true);
    if (IsFlagSet(GAME_EFFECT_ACTIVE))
    {
        SetActive(false);
    }
    m_gameEffectSystem->UnRegisterEffect(this);
    SetFlag(GAME_EFFECT_INITIALISED, false);
    SetFlag(GAME_EFFECT_RELEASING, false);
    SetFlag(GAME_EFFECT_RELEASED, true);
} //------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates game effect
//--------------------------------------------------------------------------------------------------
inline void CGameEffect::Update(float frameTime)
{
    FX_ASSERT_MESSAGE(IsFlagSet(GAME_EFFECT_INITIALISED),
        "Effect being updated without being initialised first");
    FX_ASSERT_MESSAGE((IsFlagSet(GAME_EFFECT_RELEASED) == false),
        "Effect being updated after being released");
} //------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetActive
// Desc: Sets active status
//--------------------------------------------------------------------------------------------------
inline void CGameEffect::SetActive(bool isActive)
{
    FX_ASSERT_MESSAGE(IsFlagSet(GAME_EFFECT_INITIALISED),
        "Effect changing active status without being initialised first");
    FX_ASSERT_MESSAGE((IsFlagSet(GAME_EFFECT_RELEASED) == false),
        "Effect changing active status after being released");

    SetFlag(GAME_EFFECT_ACTIVE, isActive);
    m_gameEffectSystem->RegisterEffect(this); // Re-register effect with game effects system
} //------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: LoadMaterial
// Desc: Loads and calls AddRef on material
//--------------------------------------------------------------------------------------------------
inline _smart_ptr<IMaterial> CGameEffect::LoadMaterial(const char* pMaterialName)
{
    _smart_ptr<IMaterial> pMaterial = NULL;
    I3DEngine* p3DEngine = gEnv->p3DEngine;
    if (pMaterialName && p3DEngine)
    {
        IMaterialManager* pMaterialManager = p3DEngine->GetMaterialManager();
        if (pMaterialManager)
        {
            pMaterial = pMaterialManager->LoadMaterial(pMaterialName);
        }
    }
    return pMaterial;
} //------------------------------------------------------------------------------------------------


#endif//_GAMEEFFECTBASE_H_
