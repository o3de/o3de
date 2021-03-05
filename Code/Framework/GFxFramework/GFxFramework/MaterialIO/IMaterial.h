#pragma once

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

#ifndef AZINCLUDE_GFXFRAMEWORK_IMATERIAL_H_
#define AZINCLUDE_GFXFRAMEWORK_IMATERIAL_H_

#include <AzCore/std/string/string.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class Vector3;

    namespace GFxFramework
    {
        //These strings form the basis for a default illum or physics shader that can be 
        //used in the case where we are generating new mtl files and not just updating or 
        //reading an mtl file for processing purposes. 
        namespace MaterialExport
        {
            extern const char* g_subMaterialString;
            extern const char* g_materialString; 
            extern const char* g_texturesString;
            extern const char* g_textureString;
            extern const char* g_diffuseMapName;
            extern const char* g_specularMapName;
            extern const char* g_bumpMapName;
            extern const char* g_shininessString;
            extern const char* g_opacity;
            extern const char* g_genString;
            extern const char* g_illumShaderName;
            extern const char* g_defaultIllumGenMask;
            extern const char* g_defaultNoDrawGenMask;
            extern const char* g_defaultShininess;
            extern const char* g_defaultOpacity;
            extern const char* g_mapString;
            extern const char* g_fileString;
            extern const char* g_nameString;
            extern const char* g_shaderString;
            extern const char* g_noDrawShaderName;
            extern const char* g_mtlFlagString;
            extern const char* g_whiteColor;
            extern const char* g_blackColor;

            extern const char* g_stringGenMask;
            extern const char* g_stringGenMaskOption_VertexColors;
            extern const char* g_stringPhysicsNoDraw;

            extern const char* g_mtlExtension;
            extern const char* g_dccMaterialExtension;
            extern const char* g_dccMaterialHashString;

            extern const unsigned g_materialNotFound;
        }

        enum EMaterialFlags
        {
            MTL_FLAG_PURE_CHILD = 0x0080,   // Not shared sub material, sub material unique to his parent multi material.
            MTL_FLAG_MULTI_SUBMTL = 0x0100,   // This material is a multi sub material.
            MTL_FLAG_NODRAW = 0x0400,   // Do not render this material.
            MTL_64BIT_SHADERGENMASK = 0x80000,   // ShaderGen mask is remapped
            ///! Set to 0x8000000 because it is not in CryCommon::EMaterialFlags
            MTL_FLAG_NODRAW_TOUCHBENDING = 0x8000000, //Do not render this material. Used for TouchBending Simulation Trigger.
        };

        enum class TextureMapType
        {
            Diffuse,
            Specular,
            Bump
        };

        enum class ShaderType
        {
            Illum,
            NoDraw
        };

        class IMaterial
        {
        public:
            AZ_RTTI(IMaterial, "{9623F88B-0DD0-4772-A019-C109DE287335}");

            virtual ~IMaterial() = default;
            
            virtual void SetDataFromMtl(rapidxml::xml_node<char>* materialNode) = 0;

            virtual const AZStd::string& GetName() const = 0;
            virtual void SetName(const AZStd::string& name) = 0;
            virtual const AZStd::string& GetTexture(TextureMapType type) const = 0;
            virtual void SetTexture(TextureMapType type, const AZStd::string& texture) = 0;

            virtual bool UseVertexColor() const = 0;
            virtual void EnableUseVertexColor(bool useVertexColor) = 0;

            ///! ORed enum EMaterialFlags
            virtual int GetMaterialFlags() const = 0;
            virtual void SetMaterialFlags(int flags) = 0;

            virtual const AZ::Vector3& GetDiffuseColor() const = 0;
            virtual void SetDiffuseColor(const AZ::Vector3& color) = 0;
            virtual const AZ::Vector3& GetSpecularColor() const = 0;
            virtual void SetSpecularColor(const AZ::Vector3& color) = 0;
            virtual const AZ::Vector3& GetEmissiveColor() const = 0;
            virtual void SetEmissiveColor(const AZ::Vector3& color) = 0;
            virtual float GetOpacity() const = 0;
            virtual void SetOpacity(float opacity) = 0;
            virtual float GetShininess() const = 0;
            virtual void SetShininess(float shininess) = 0;

            virtual AZ::u32 GetDccMaterialHash() const = 0;
            virtual void SetDccMaterialHash(AZ::u32 hash) = 0;

            AZ::u32 CalculateDccMaterialHash()
            {
                // Hash name
                AZ::Crc32 hash(GetName().c_str());

                // Hash texture names
                hash.Add(GetTexture(TextureMapType::Diffuse).c_str());
                hash.Add(GetTexture(TextureMapType::Specular).c_str());
                hash.Add(GetTexture(TextureMapType::Bump).c_str());
                
                // Hash colors
                hash.Add(&GetDiffuseColor(), sizeof(AZ::Vector3));
                hash.Add(&GetSpecularColor(), sizeof(AZ::Vector3));
                hash.Add(&GetEmissiveColor(), sizeof(AZ::Vector3));

                // Hash floats
                float tempFloat = GetOpacity();
                hash.Add(&tempFloat, sizeof(float));
                tempFloat = GetShininess();
                hash.Add(&tempFloat, sizeof(float));

                // Hash ints 
                int tempInt = GetMaterialFlags();
                hash.Add(&tempInt, sizeof(int));

                // Hash booleans
                bool tempBool = UseVertexColor();
                hash.Add(&tempBool, sizeof(bool));

                return hash;
            }
        };


        class IMaterialGroup
        {
        public:
            AZ_RTTI(IMaterialGroup, "{D9417F20-D52B-4E00-9DB3-13F9ED5F2F28}")

            virtual ~IMaterialGroup() = default;
           
            virtual void AddMaterial(const AZStd::shared_ptr<IMaterial>& material) = 0;
            virtual void RemoveMaterial(const AZStd::string& name) = 0;
            virtual size_t FindMaterialIndex(const AZStd::string& name) = 0;
            virtual size_t GetMaterialCount() = 0;
            virtual AZStd::shared_ptr<IMaterial> GetMaterial(size_t index) = 0;
            virtual AZStd::shared_ptr<const IMaterial> GetMaterial(size_t index) const = 0;
            
            virtual bool ReadMtlFile(const char* fileName) = 0;
            virtual bool WriteMtlFile(const char* filename) = 0;
            virtual const AZStd::string& GetMtlName() = 0;
            virtual void SetMtlName(const AZStd::string& name) = 0;
        };

    }  //namespace GFxFramework
}  //namespace AZ

#endif // AZINCLUDE_GFXFRAMEWORK_IMATERIAL_H_