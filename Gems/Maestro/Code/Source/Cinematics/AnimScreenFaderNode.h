/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_ANIMSCREENFADERNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMSCREENFADERNODE_H
#pragma once

#include "AnimNode.h"

class CScreenFaderTrack;

class CAnimScreenFaderNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimScreenFaderNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimScreenFaderNode, "{C24D5F2D-B17A-4350-8381-539202A99FDD}", CAnimNode);

    //-----------------------------------------------------------------------------
    //!
    CAnimScreenFaderNode(const int id);
    CAnimScreenFaderNode();
    ~CAnimScreenFaderNode();

    static void Initialize();

    //-----------------------------------------------------------------------------
    //! Overrides from CAnimNode
    virtual void Animate(SAnimContext& ac);

    virtual void CreateDefaultTracks();

    virtual void OnReset();

    virtual void Activate(bool bActivate);

    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    //-----------------------------------------------------------------------------
    //! Overrides from IAnimNode
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    void SetFlags(int flags) override;

    virtual void Render();

    bool IsAnyTextureVisible() const;

    static void Reflect(AZ::ReflectContext* context);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    virtual bool NeedToRender() const { return true; }

private:
    CAnimScreenFaderNode(const CAnimScreenFaderNode&);
    CAnimScreenFaderNode& operator = (const CAnimScreenFaderNode&);

private:
    //-----------------------------------------------------------------------------
    //!
    void PrecacheTexData();

    Vec4        m_startColor;
    bool        m_bActive;
    float       m_screenWidth, m_screenHeight;
    int         m_lastActivatedKey;

    bool        m_texPrecached;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMSCREENFADERNODE_H
