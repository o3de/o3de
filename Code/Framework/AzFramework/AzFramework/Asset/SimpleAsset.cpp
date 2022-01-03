/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace AzFramework
{
    const char* SimpleAssetTypeGetName(const AZ::Data::AssetType& nameUuid)
    {
        char varName[SimpleAssetReferenceBase::kMaxVariableNameLength] = { 0 };
        azsnprintf(varName, SimpleAssetReferenceBase::kMaxVariableNameLength, "assetname%s", nameUuid.ToString<AZStd::string>().c_str());
        auto variable = AZ::Environment::FindVariable<AssetInfoString>(varName);
        if (variable)
        {
            return (*variable).c_str();
        }

        return "";
    }

    const char* SimpleAssetTypeGetFileFilter(const AZ::Data::AssetType& nameUuid)
    {
        char varName[SimpleAssetReferenceBase::kMaxVariableNameLength] = { 0 };
        azsnprintf(varName, SimpleAssetReferenceBase::kMaxVariableNameLength, "assetfilter%s", nameUuid.ToString<AZStd::string>().c_str());
        auto variable = AZ::Environment::FindVariable<AssetInfoString>(varName);
        if (variable)
        {
            return (*variable).c_str();
        }

        return "";
    }
    void SimpleAssetReferenceBase::Reflect(AZ::ReflectContext *context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SimpleAssetReferenceBase>()
                    ->Version(1)
                    ->Field("AssetPath", &SimpleAssetReferenceBase::m_assetPath);

            AZ::EditContext* edit = serializeContext->GetEditContext();
            if (edit)
            {
                edit->Class<SimpleAssetReferenceBase>("Asset path", "Asset reference as a project-relative path")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SimpleAssetReferenceBase>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Asset")
                    ->Attribute(AZ::Script::Attributes::Module, "asset")
                    ->Property("assetPath", &SimpleAssetReferenceBase::GetAssetPath, nullptr)
                    ->Property("assetType", &SimpleAssetReferenceBase::GetAssetType, nullptr)
                    ->Property("fileFilter", &SimpleAssetReferenceBase::GetFileFilter, nullptr)
                    ->Method("SetAssetPath", &SimpleAssetReferenceBase::SetAssetPath)
                    ->Attribute(AZ::Script::Attributes::Alias, "set_asset_path")
                    ;
        }
    }
} // namespace AzFramework
