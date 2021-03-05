/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Common texture manager declarations.


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_TEXTUREMANAGER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_TEXTUREMANAGER_H
#pragma once


class CTexture;

#include "CryName.h"

//------------------------------------------------------------------------------
// [Shaders System] - to replace CCryNameTSCRC by NameHashPair after new renderer 
//  integration.
//------------------------------------------------------------------------------
struct MaterialTextureSemantic
{
    MaterialTextureSemantic(EEfResTextures texSlot, int8 pri, CTexture* defTex, CTexture* neutralTex, char const* suff)
        : slot(texSlot), priority(pri), def(defTex), neutral(neutralTex) 
    {
        suffix = new char[strlen(suff)+1];  // added one to protect from empty strings.
        azstrcpy(suffix, strlen(suff) + 1, suff);
    };

    MaterialTextureSemantic& operator=(MaterialTextureSemantic& srcSemantic)
    {
        SAFE_DELETE_ARRAY(suffix);

        slot = srcSemantic.slot;
        priority = srcSemantic.priority;
        def = srcSemantic.def;
        neutral = srcSemantic.neutral;
        suffix = new char[strlen(srcSemantic.suffix) + 1];  // added one to protect from empty strings.
        azstrcpy(suffix, strlen(srcSemantic.suffix) + 1, srcSemantic.suffix);

        return *this;
    }

    MaterialTextureSemantic()
    {
        slot = EFTT_MAX;
        def = nullptr;
        neutral = nullptr;
        suffix = nullptr;
    };

    ~MaterialTextureSemantic()
    {
        SAFE_DELETE_ARRAY(suffix);
    };

    EEfResTextures  slot;
    int8            priority;
    CTexture*       def;
    CTexture*       neutral;
    char*           suffix;
};

//==============================================================================
// CTextureManager is a singleton class.  
// It is NOT the texture resource manager but rather holds vital textures required 
// by the engine and renderer.
// These textures are the default textures (for example - missing resource, resource 
//  compiling...), the render engine pipeline textures such as the textures that 
// are used for noise, and finally, the material 'snapshot' textures which are 
// assigned just before the shader's resources are bound.
// 
// Possible improvements on that are:
// 1. Move the m_EngineTextures and m_MaterialTextures to ResourceGroupManager 
//      that will manage the 'per frequency' resource snapshot.
// 2. Keep only pointers / refs to the resources, while the resources themselves are 
//      managed and held by the ResourceManager (currently carried out by CBaseResource..)
// 3. Address the default textures via enum and not strings if turns out to be a hit.
// 4. Move to use NameHashPair instead of CCryNameTSCRC for Amazon internal + allow 
// s    having name debug info
// 
// Remarks:
// 1. The work for moving all static Cry textures to the manager is NOT complete yet and
//      should be continued and include changing usage of all default 'error textures' and
//      removing all static CTextures from CTexture class and stop using it as 
//      a static manager class.
// 
//------------------------------------------------------------------------------
class CTextureManager
{
protected:

    CTextureManager() 
    {
        s_Instance = nullptr;
        m_texNoTexture = nullptr;
        m_texNoTextureCM = nullptr;
        m_texWhite = nullptr;
        m_texBlack = nullptr;
        m_texBlackCM = nullptr;
    }

    void ReleaseResources()
    {
        ReleaseTextures();
        ReleaseTextureSemantics();
    }

    virtual ~CTextureManager()
    {
        ReleaseResources();
    }

    void ReleaseTextures();
    void ReleaseTextureSemantics();

public:

    void Init( bool forceInit=false )
    {
        if (forceInit)
        {
            ReleaseResources();
        }

        // this will be carried out only if it is the first item or after force init.
        if (!m_DefaultTextures.size() && !m_EngineTextures.size() && !m_MaterialTextures.size())
        {
            AZ_TracePrintf("[Shaders System]", "Textures Manager - allocating default resources");
            // First load is for loading semantics but the texture slots will remain null as the default 
            // textures were not loaded yet.   This is required as the semantics suffix will be used during
            // the default texture load.
            LoadMaterialTexturesSemantics();

            // Default texture load
            LoadDefaultTextures();
            CreateEngineTextures();

            // References to legacy static textures
            CreateStaticEngineTextureReferences();

            // Second time is for attaching the loaded textures to the semantics
            LoadMaterialTexturesSemantics();
        }
    }

    inline static CTextureManager* Instance()
    {
        if (!s_Instance)
            s_Instance = new CTextureManager;

        return s_Instance;
    }

    inline static bool InstanceExists()
    {
        return s_Instance ? true : false;
    }

    void Release()
    {
        SAFE_DELETE(s_Instance);
    }

    void LoadDefaultTextures();
    void LoadMaterialTexturesSemantics();
    void CreateEngineTextures() {};
    void CreateStaticEngineTextureReferences();

    inline MaterialTextureSemantic& GetTextureSemantic(int texSlot)
    {
        AZ_Assert(texSlot >= 0 && texSlot <= EFTT_MAX, "[CTextureManager::GetTextureSemantic] Slot is out of range: %d", texSlot);
        //we need to correct the texSlot if by any chance there is an unexpected input value. 
        if (texSlot < 0 || texSlot > EFTT_MAX)
        {
            texSlot = EFTT_UNKNOWN;
        }
        return m_TexSlotSemantics[texSlot];
    }

    inline CTexture* GetDefaultTexture( const char* sTextureName) const
    {
        CCryNameTSCRC   hashEntry(sTextureName);
        auto            iter = m_DefaultTextures.find(hashEntry);
        return (iter != m_DefaultTextures.end() ? iter->second : nullptr);
    }

    inline CTexture* GetEngineTexture( const char* sTextureName) const
    {
        CCryNameTSCRC hashEntry(sTextureName);
        return GetEngineTexture(hashEntry);
    }

    inline CTexture* GetEngineTexture( const CCryNameTSCRC& crc) const
    {
        auto iter = m_EngineTextures.find(crc);
        return iter != m_EngineTextures.end() ? iter->second : GetStaticEngineTexture(crc);
    }

    inline CTexture* GetMaterialTexture(const char* sTextureName) const
    {
        CCryNameTSCRC   hashEntry(sTextureName);
        auto            iter = m_MaterialTextures.find(hashEntry);
        return (iter != m_MaterialTextures.end() ? iter->second : nullptr);
    }

    inline CTexture* GetNoTexture()     { return m_texNoTexture;     }
    inline CTexture* GetNoTextureCM()   { return m_texNoTextureCM;   }
    inline CTexture* GetWhiteTexture()  { return m_texWhite;    }
    inline CTexture* GetBlackTexture()  { return m_texBlack; }
    inline CTexture* GetBlackTextureCM() { return m_texBlackCM; }

private:
    inline CTexture* GetStaticEngineTexture(const CCryNameTSCRC& crc) const
    {
        auto iter = m_StaticEngineTextureReferences.find(crc);
        return iter != m_StaticEngineTextureReferences.end() ? *iter->second : nullptr;
    }

    MaterialTextureSemantic   m_TexSlotSemantics[EFTT_MAX];       // [Shaders System] - to do - replace with map

    // [Shaders System] CCryNameTSCRC should be replaced by 
    typedef std::map<CCryNameTSCRC, CTexture*>            OrderedTextureMap;
    typedef std::map<CCryNameTSCRC, CTexture*>            TextureMap;
    typedef std::map<CCryNameTSCRC, CTexture**>           TextureRefMap;

    // Textures used as defaults for internal engine usage, place holders etc...
    TextureMap    m_DefaultTextures;    

    // The following two textures are set separately since they are called often
    CTexture*     m_texWhite;
    CTexture*     m_texBlack;
    CTexture*     m_texBlackCM;
    CTexture*     m_texNoTexture;
    CTexture*     m_texNoTextureCM;

    // Runtime engine textures used by the various shaders and represents last
    // taken 'snapshot'
    TextureMap          m_EngineTextures;       

    // pending the move to using CTextureManager for all engine textures,
    // provides access to existing static engine textures 
    TextureRefMap       m_StaticEngineTextureReferences;       

    // material 'snapshot' (most recent) textures to be used by the shader.
    OrderedTextureMap   m_MaterialTextures;

    // The CTextureManager is a singleton - this is its single instance
    static CTextureManager*     s_Instance;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_TEXTUREMANAGER_H
