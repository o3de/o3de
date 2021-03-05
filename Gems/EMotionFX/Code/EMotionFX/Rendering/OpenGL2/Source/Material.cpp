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

#include "Material.h"
#include "GraphicsManager.h"
#include "glactor.h"
#include <MCore/Source/StringConversions.h>


namespace RenderGL
{
    // constructor
    Material::Material(GLActor* actor)
    {
        mActor = actor;
    }


    // destructor
    Material::~Material()
    {
    }


    // return attribute name
    const char* Material::AttributeToString(const EAttribute attribute)
    {
        switch (attribute)
        {
        case LIGHTING:
            return "LIGHTING";
        case SKINNING:
            return "SKINNING";
        case SHADOWS:
            return "SHADOWS";
        case TEXTURING:
            return "TEXTURING";
        default:
            ;
        }

        return "";
    }


    // load the given texture
    Texture* Material::LoadTexture(const char* fileName, bool genMipMaps)
    {
        Texture*        result      = nullptr;
        AZStd::string   filename    = mActor->GetTexturePath() + fileName;
        AZStd::string   extension;
        AzFramework::StringFunc::Path::GetExtension(fileName, extension, false /* include dot */);

        // the filename may or may not have an extension on it
        AZStd::string texturePath;
        if (extension.size() == 0)
        {
            // supported texture formats
            static uint32     numExtensions = 9;
            static const char* extensions[] = { ".dds", ".png", ".jpg", ".tga", ".hdr", ".bmp", ".dib", ".pfm", ".ppm" };

            // try to load the texture
            for (uint32 i = 0; i < numExtensions; ++i)
            {
                texturePath = filename + extensions[i];

                // try to load the texture
                result = GetGraphicsManager()->LoadTexture(texturePath.c_str(), genMipMaps);
                if (result)
                {
                    break;
                }
            }
        }
        else
        {
            result = GetGraphicsManager()->LoadTexture(fileName, genMipMaps);
        }

        // check if we were able to load the texture correctly
        if (result == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load the texture '%s'", filename.c_str());
            //result = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
        }

        return result;
    }


    Texture* Material::LoadTexture(const char* fileName)
    {
        return LoadTexture(fileName, GetGraphicsManager()->GetCreateMipMaps());
    }
}
