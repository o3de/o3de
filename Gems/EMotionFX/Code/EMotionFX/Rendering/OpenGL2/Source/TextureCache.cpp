/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TextureCache.h"
#include "GraphicsManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace RenderGL
{
    // constructor
    Texture::Texture()
    {
        initializeOpenGLFunctions();
        mTexture    = 0;
        mWidth      = 0;
        mHeight     = 0;
    }


    // constructor
    Texture::Texture(GLuint texID, uint32 width, uint32 height)
    {
        initializeOpenGLFunctions();
        mTexture    = texID;
        mWidth      = width;
        mHeight     = height;
    }


    // destructor
    Texture::~Texture()
    {
        glDeleteTextures(1, &mTexture);
    }


    // constructor
    TextureCache::TextureCache()
    {
        mWhiteTexture           = nullptr;
        mDefaultNormalTexture   = nullptr;

        mEntries.SetMemoryCategory(MEMCATEGORY_RENDERING);
        mEntries.Reserve(128);
    }


    // destructor
    TextureCache::~TextureCache()
    {
        Release();
    }


    // initialize
    bool TextureCache::Init()
    {
        // create the white dummy texture
        initializeOpenGLFunctions();
        const bool whiteTexture  = CreateWhiteTexture();
        const bool normalTexture = CreateDefaultNormalTexture();
        return (whiteTexture && normalTexture);
    }


    // release all textures
    void TextureCache::Release()
    {
        // delete all textures
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            delete mEntries[i].mTexture;
        }

        // clear all entries
        mEntries.Clear();

        // delete the white texture
        delete mWhiteTexture;
        mWhiteTexture = nullptr;

        delete mDefaultNormalTexture;
        mDefaultNormalTexture = nullptr;
    }


    // add the texture to the cache (assume there are no duplicate names)
    void TextureCache::AddTexture(const char* filename, Texture* texture)
    {
        mEntries.AddEmpty();
        mEntries.GetLast().mName    = filename;
        mEntries.GetLast().mTexture = texture;
    }


    // try to locate a texture based on its name
    Texture* TextureCache::FindTexture(const char* filename) const
    {
        // get the number of entries and iterate through them
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (AzFramework::StringFunc::Equal(mEntries[i].mName.c_str(), filename, false /* no case */)) // non-case-sensitive name compare
            {
                return mEntries[i].mTexture;
            }
        }

        // not found
        return nullptr;
    }


    // check if we have a given texture in the cache
    bool TextureCache::CheckIfHasTexture(Texture* texture) const
    {
        // get the number of entries and iterate through them
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (mEntries[i].mTexture == texture)
            {
                return true;
            }
        }

        return false;
    }


    // remove an item from the cache
    void TextureCache::RemoveTexture(Texture* texture)
    {
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (mEntries[i].mTexture == texture)
            {
                delete mEntries[i].mTexture;
                mEntries.Remove(i);
                return;
            }
        }
    }

    // create the white texture
    bool TextureCache::CreateWhiteTexture()
    {
        GLuint textureID;
        glGenTextures(1, &textureID);

        uint32 width = 2;
        uint32 height = 2;
        uint32 imageBuffer[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            imageBuffer[i] = MCore::RGBA(255, 255, 255, 255); // actually abgr
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        mWhiteTexture = new Texture(textureID, width, height);
        return true;
    }


    // create the white texture
    bool TextureCache::CreateDefaultNormalTexture()
    {
        GLuint textureID;
        glGenTextures(1, &textureID);

        uint32 width = 2;
        uint32 height = 2;
        uint32 imageBuffer[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            imageBuffer[i] = MCore::RGBA(255, 128, 128, 255); // opengl wants abgr
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        mDefaultNormalTexture = new Texture(textureID, width, height);
        return true;
    }
} // namespace RenderGL
