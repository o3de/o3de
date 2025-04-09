/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TextureAtlasImpl.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace TextureAtlasNamespace
{
    constexpr const char* s_CoordinatePairsName = "Coordinate Pairs";

    TextureAtlasImpl::TextureAtlasImpl()
    {
        m_image = nullptr;
    }

    TextureAtlasImpl::~TextureAtlasImpl() {}

    TextureAtlasImpl::TextureAtlasImpl(AtlasCoordinateSets handles)
    {
        for (int i = 0; i < handles.size(); ++i)
        {
            this->m_data[handles[i].first] = handles[i].second;
        }
        m_image = nullptr;
    }

    bool TextureAtlasImpl::TextureAtlasVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 2)
        {
            AZStd::unordered_map<AZStd::string, AtlasCoordinates> oldData;
            if (!rootElement.GetChildData(AZ_CRC_CE(s_CoordinatePairsName), oldData))
            {
                AZ_Error("TextureAtlas", false, "Failed to find old %s unordered_map element on version %u", s_CoordinatePairsName, rootElement.GetVersion());
                return false;
            }

            AZStd::unordered_map<AZStd::string, AtlasCoordinates, hash_case_insensitive, equal_to_case_insensitive> newData{ oldData.begin(), oldData.end() };
            rootElement.RemoveElementByName(AZ_CRC_CE(s_CoordinatePairsName));
            rootElement.AddElementWithData(context, s_CoordinatePairsName, newData);
        }

        return true;
    }

    // Reflect The coordinates and the coordinate format
    void TextureAtlasImpl::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->ClassDeprecate("SimpleAssetReference_TextureAtlasAsset", AZ::Uuid("{6F612FE6-A054-4E49-830C-0288F3C79A52}"),
                [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
            {
                AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
                for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
                {
                    childNodeElements.push_back(rootElement.GetSubElement(index));
                }
                // Convert the rootElement now, the existing child DataElmentNodes are now removed
                rootElement.Convert<AzFramework::SimpleAssetReference<TextureAtlasAsset>>(context);
                for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
                {
                    rootElement.AddElement(AZStd::move(childNodeElement));
                }
                return true;
            });
            
            // Need to serialize the old AZStd::unordered_map<AZStd::string, AtlasCoordinates> type
            serialize->RegisterGenericType<AZStd::unordered_map<AZStd::string, AtlasCoordinates>>();
            serialize->Class<TextureAtlasImpl>()->Version(2, &TextureAtlasVersionConverter)
                ->Field(s_CoordinatePairsName, &TextureAtlasImpl::m_data)
                ->Field("Width", &TextureAtlasImpl::m_width)
                ->Field("Height", &TextureAtlasImpl::m_height);
            AzFramework::SimpleAssetReference<TextureAtlasAsset>::Register(*serialize);
        }
        AtlasCoordinates::Reflect(context);
    }

    // Coordinates reflect their internal properties
    void AtlasCoordinates::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtlasCoordinates>()
                ->Version(1)
                ->Field("Left", &AtlasCoordinates::m_left)
                ->Field("Top", &AtlasCoordinates::m_top)
                ->Field("Width", &AtlasCoordinates::m_width)
                ->Field("Height", &AtlasCoordinates::m_height);
        }
    }

    // Retrieves the value that corresponds to a given handle in the atlas
    AtlasCoordinates TextureAtlasImpl::GetAtlasCoordinates(const AZStd::string& handle) const
    {
        AZStd::string path = handle;
        path = path.substr(0, path.find_last_of('.'));
        // Use an iterator to check if the key is being used in the hash table
        auto iterator = m_data.find(path);
        if (iterator != m_data.end())
        {
            return iterator->second;
        }
        else
        {
            return AtlasCoordinates(-1, -1, -1, -1);
        }
    }

    // Links this atlas to an image pointer
    void TextureAtlasImpl::SetTexture(AZ::Data::Instance<AZ::RPI::Image> image)
    {
        // We don't need to delete the old value because the pointer is handled elsewhere
        m_image = image;
    }
    
    // Returns the image linked to this atlas
    AZ::Data::Instance<AZ::RPI::Image> TextureAtlasImpl::GetTexture() const
    {
        return m_image;
    }
    
    // Internal to gem function for overwriting parameters
    void TextureAtlasImpl::OverwriteMappings(TextureAtlasImpl* source)
    {
        m_data.clear();
        m_data = source->m_data;
        m_width = source->m_width;
        m_height = source->m_height;
    }
}
