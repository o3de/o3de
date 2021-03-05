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

class ITexture;

namespace TextureAtlasNamespace
{
    //! Struct that represents a slot on a texture atlas
    struct AtlasCoordinates
    {
        AZ_CLASS_ALLOCATOR(AtlasCoordinates, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(AtlasCoordinates, "{FC5D6A60-1056-4F6C-96F7-6A47912F8A35}");

    private:
        int m_left;
        int m_width;
        int m_top;
        int m_height;

    public:
        AtlasCoordinates() {}

        //! The left-right style is prefferable for input because thats what the asset builder operates with
        AtlasCoordinates(int left, int right, int top, int bottom)
        {
            m_left = left;
            m_width = right - left;
            m_top = top;
            m_height = bottom - top;
        }

        int GetRight() const { return m_left + m_width; }
        void SetRight(int value) { m_width = value - m_left; }

        int GetBottom() const { return m_top + m_height; }
        void SetBottom(int value) { m_height = value - m_top; }

        int GetLeft() const { return m_left; }
        void SetLeft(int value) { m_left = value; }

        int GetTop() const { return m_top; }
        void SetTop(int value) { m_top = value; }

        int GetWidth() const { return m_width; }
        void SetWidth(int value) { m_width = value; }

        int GetHeight() const { return m_height; }
        void SetHeight(int value) { m_height = value; }

        static void Reflect(AZ::ReflectContext* context);
    };

    //! An abstract class that exposes atlas pointers to the rest of the codebase
    class TextureAtlas
    {
    public:
        AZ_CLASS_ALLOCATOR(TextureAtlas, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(TextureAtlas, "{56FF34CF-7C5B-4BBC-9E2B-AFCA1C6C7561}");

        virtual ~TextureAtlas() {}

        //! Retrieve a coordinate set from the Atlas by its handle
        virtual AtlasCoordinates GetAtlasCoordinates(const AZStd::string& handle) const = 0;
        //! Links this atlas to an image pointer
        virtual void SetTexture(ITexture* image) = 0;
        //! Returns the image linked to this atlas
        virtual ITexture* GetTexture() const = 0;
        //! Returns the width of the atlas
        virtual int GetWidth() const = 0;
        //! Returns the height of the atlas
        virtual int GetHeight() const = 0;
    };

} // namespace TextureAtlasNamespace
