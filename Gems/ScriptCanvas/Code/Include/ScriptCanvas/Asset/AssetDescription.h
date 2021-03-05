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
#include "AssetRegistryBus.h"

#include <AzCore/EBus/Results.h>
#include <AzCore/Math/Color.h>

namespace ScriptCanvas
{
    class AssetDescription
    {
    public:

        AZ_TYPE_INFO(AssetDescription, "{2D2C5BF2-5F94-4A74-AF8B-08AC65A733F7}");

        AssetDescription() = default;

        AssetDescription(   AZ::Data::AssetType assetType, 
                            const char* name,
                            const char* description,
                            const char* suggestedSavePath,
                            const char* fileExtension,
                            const char* group,
                            const char* assetNamePattern,
                            const char* fileFilter,
                            const char* assetTypeDisplayName,
                            const char* entityName,
                            const char* iconPath,
                            AZ::Color displayColor,
                            bool isEditableType)
            : m_assetType(assetType)
            , m_name(name)
            , m_description(description)
            , m_suggestedSavePath(suggestedSavePath)
            , m_fileExtension(fileExtension)
            , m_group(group)
            , m_assetNamePattern(assetNamePattern)
            , m_fileFilter(fileFilter)
            , m_assetTypeDisplayName(assetTypeDisplayName)
            , m_entityName(entityName)
            , m_iconPath(iconPath)
            , m_displayColor(displayColor)
            , m_isEditableType(isEditableType)
        {
        }

#define ASSET_DESCRIPTION_GETTER_NT(GetterName) \
        static const char* GetterName(AZ::Data::AssetType assetType) { \
            AZ::EBusAggregateResults<AssetDescription*> descriptions; \
            AssetRegistryRequestBus::EventResult(descriptions, assetType, &AssetRegistryRequests::GetAssetDescription, assetType); \
            if (!descriptions.values.empty()) { \
                return descriptions.values[0] ? descriptions.values[0]->GetterName##Impl() : "";  } \
            return ""; \
        }

#define ASSET_DESCRIPTION_GETTER(GetterName) \
        template <typename AssetType> \
        static const char* GetterName() { \
            return GetterName(azrtti_typeid<AssetType>()); \
        } \
        ASSET_DESCRIPTION_GETTER_NT(GetterName)

        ASSET_DESCRIPTION_GETTER(GetName);
        ASSET_DESCRIPTION_GETTER(GetDescription);
        ASSET_DESCRIPTION_GETTER(GetSuggestedSavePath);
        ASSET_DESCRIPTION_GETTER(GetExtension);
        ASSET_DESCRIPTION_GETTER(GetGroup);
        ASSET_DESCRIPTION_GETTER(GetAssetNamePattern);
        ASSET_DESCRIPTION_GETTER(GetFileFilter);
        ASSET_DESCRIPTION_GETTER(GetAssetTypeDisplayName);
        ASSET_DESCRIPTION_GETTER(GetEntityName);
        ASSET_DESCRIPTION_GETTER(GetIconPath);

#define ASSET_DESCRIPTION_GETTER_COLOR_NT(GetterName) \
        static AZ::Color GetterName(AZ::Data::AssetType assetType) { \
            AZ::EBusAggregateResults<AssetDescription*> descriptions; \
            AssetRegistryRequestBus::EventResult(descriptions, assetType, &AssetRegistryRequests::GetAssetDescription, assetType); \
            if (!descriptions.values.empty()) { \
                return descriptions.values[0] ? descriptions.values[0]->GetterName##Impl() : AZ::Color(0.f,0.f,0.f, 0.f);  } \
            return AZ::Color(0.f,0.f,0.f, 0.f); \
        }

        template<typename AssetType>
        static AZ::Color GetDisplayColor() {
            return GetDisplayColor(azrtti_typeid<AssetType>());
        }
        ASSET_DESCRIPTION_GETTER_COLOR_NT(GetDisplayColor);

#define ASSET_DESCRIPTION_GETTER_BOOL_NT(GetterName) \
        static bool GetterName(AZ::Data::AssetType assetType) { \
            AZ::EBusAggregateResults<AssetDescription*> descriptions; \
            AssetRegistryRequestBus::EventResult(descriptions, assetType, &AssetRegistryRequests::GetAssetDescription, assetType); \
            if (!descriptions.values.empty()) { \
                return descriptions.values[0] ? descriptions.values[0]->GetterName##Impl() : false;  } \
            return false; \
        }

        template<typename AssetType>
        static bool GetIsEditableType() {
            return GetIsEditableType(azrtti_typeid<AssetType>());
        }

        ASSET_DESCRIPTION_GETTER_BOOL_NT(GetIsEditableType);

    private:

        AZ::Data::AssetType m_assetType;
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_suggestedSavePath;
        AZStd::string m_fileExtension;
        AZStd::string m_group;
        AZStd::string m_assetNamePattern;
        AZStd::string m_fileFilter;
        AZStd::string m_assetTypeDisplayName;
        AZStd::string m_entityName;
        AZStd::string m_iconPath;

        AZ::Color     m_displayColor;

        bool          m_isEditableType = false;

    public:

        AZ::Data::AssetType GetAssetType() const { return m_assetType; }

        const char* GetNameImpl() { return m_name.c_str(); }
        const char* GetDescriptionImpl() { return m_description.c_str(); }
        const char* GetSuggestedSavePathImpl() { return m_suggestedSavePath.c_str(); }
        const char* GetExtensionImpl() { return m_fileExtension.c_str(); }
        const char* GetGroupImpl() { return m_group.c_str(); }
        const char* GetAssetNamePatternImpl() { return m_assetNamePattern.c_str(); }
        const char* GetFileFilterImpl() { return m_fileFilter.c_str(); }
        const char* GetAssetTypeDisplayNameImpl() { return m_assetTypeDisplayName.c_str(); }
        const char* GetEntityNameImpl() { return m_entityName.c_str(); }
        const char* GetIconPathImpl() { return m_iconPath.c_str(); }

        AZ::Color GetDisplayColorImpl() { return m_displayColor; }

        bool GetIsEditableTypeImpl() { return m_isEditableType; }
    };

#define ASSET_DESCRIPTION(ClassName, Type, Name, Description, SuggestedSavePath, FileExtension, Group, AssetNamePattern, FileFilter, AssetTypeDisplayName, EntityName, IconPath) \
    template <> \
    class AssetDescription<Type> \
    { \
    public: \
        AssetDescription<Type>() \
            : m_assetType(azrtti_typeid<Type>()) \
            , m_name(Name) \
            , m_description(Description) \
            , m_suggestedSavePath(SuggestedSavePath) \
            , m_fileExtension(FileExtension) \
            , m_group(Group) \
            , m_assetNamePattern(AssetNamePattern) \
            , m_fileFilter(FileFilter) \
            , m_assetTypeDisplayName(AssetTypeDisplayName) \
            , m_entityName(EntityName) \
            , m_iconPath(IconPath) \
                {} \
    };

}
