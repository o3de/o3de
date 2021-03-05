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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include "TextureAtlas/TextureAtlas.h"
#include "TextureAtlas/TextureAtlasBus.h"

#include <ITexture.h>

namespace TextureAtlasNamespace
{
    struct equal_to_case_insensitive
    {
        AZ_TYPE_INFO(equal_to_case_insensitive, "{92DE03B1-B84F-4DEB-905D-CBE41DD6939D}");
        bool operator()(const AZStd::string& left, const AZStd::string& right) const
        {
            return AzFramework::StringFunc::Equal(left.data(), right.data());
        }
    };

    struct hash_case_insensitive : public AZStd::hash<AZStd::string>
    {
        AZ_TYPE_INFO(hash_case_insensitive, "{FE0F4349-D80D-4286-8874-733966A32B29}");
        inline result_type operator()(const AZStd::string& value) const
        {
            AZStd::string lowerStr = value;
            AZStd::to_lower(lowerStr.begin(), lowerStr.end());
            return hash::operator()(lowerStr);
        }
    };

    class TextureAtlasImpl: public TextureAtlas
    {
    public:
        AZ_CLASS_ALLOCATOR(TextureAtlasImpl, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(TextureAtlasImpl, "{2CA51C61-1B5F-4480-A257-F28D8944AA35}");

        TextureAtlasImpl();
        TextureAtlasImpl(AtlasCoordinateSets handles);
        ~TextureAtlasImpl();

        static bool TextureAtlasVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static void Reflect(AZ::ReflectContext* context);
        
        //! Retrieve a coordinate set from the Atlas by its handle
        AtlasCoordinates GetAtlasCoordinates(const AZStd::string& handle) const override;
        
        //! Links this atlas to an image pointer
        void SetTexture(ITexture* image) override;
        
        //! Returns the image linked to this atlas
        ITexture* GetTexture() const override;
        
        //! Replaces the mappings of this Texture Atlas Object, with the source's mappings
        void OverwriteMappings(TextureAtlasImpl* source);
        
        //! Returns the width of the atlas
        int GetWidth() const override { return m_width; }
        //! Sets the width of the atlas
        void SetWidth(int value) { m_width = value; }
        //! Returns the height of the atlas
        int GetHeight() const override { return m_height; }
        //! Sets the height of the atlas
        void SetHeight(int value) { m_height = value; }

    private:
        AZStd::unordered_map<AZStd::string, AtlasCoordinates, hash_case_insensitive, equal_to_case_insensitive> m_data;
        ITexture* m_image;
        int m_width;
        int m_height;
    };

} // namespace TextureAtlasNamespace
