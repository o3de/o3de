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
#include <AzCore/IO/SystemFile.h>

#include "TextureAtlas/TextureAtlasBus.h"
#include "TextureAtlas/TextureAtlas.h"
#include "TextureAtlas/TextureAtlasNotificationBus.h"
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace TextureAtlasNamespace
{
    class TextureAtlasSystemComponent : public AZ::Component, protected TextureAtlasRequestBus::Handler, public AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        AZ_COMPONENT(TextureAtlasSystemComponent, "{436E8E5A-76CA-458D-8DAD-835C30D8C41B}");
        

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // TextureAtlasRequestBus interface implementation

        //! Saves a texture atlas to file
        void SaveAtlasToFile(const AZStd::string& outputPath,
            AtlasCoordinateSets& handles, int width, int height) override;

        //! Tells the TextureAtlas system to load an Atlas and return a pointer for the atlas
        TextureAtlas* LoadAtlas(const AZStd::string& filePath) override;

        //! Returns a pointer to the first Atlas that contains the image, or nullptr if no atlas contains it
        TextureAtlas* FindAtlasContainingImage(const AZStd::string& filePath) override;

        //! Tells the TextureAtlas system to unload an Atlas
        void UnloadAtlas(TextureAtlas* atlas) override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

        //! A struct that aids in the management of texture atlases
        struct AtlasInfo
        {
            TextureAtlas* m_atlas;
            AZStd::string m_path;
            int m_refs;
            AZ::Data::AssetId m_atlasAssetId;

            //! A simple constructor that generates the AtlasInfo based on its parameters in a one to one fashion.
            AtlasInfo(TextureAtlas* atlas, AZStd::string path)
            {
                m_atlas = atlas;
                m_path = path;
                m_refs = 0;
            }

            AtlasInfo()
            {
                m_atlas = nullptr;
                m_path = "";
                m_refs = 0;
            }
        };

    private:
        AZStd::unordered_map<AZStd::string, AtlasInfo> m_atlases;
    };
}
