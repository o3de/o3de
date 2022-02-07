/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace AssetBuilderSDK
{
    bool UpdateDependenciesFromClassData(
        const AZ::SerializeContext& serializeContext,
        void* instancePointer,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        UniqueDependencyList& productDependencySet,
        ProductPathDependencySet& productPathDependencySet,
        bool enumerateChildren)
    {
        if(classData == nullptr)
        {
            return false;
        }
        if (classData->m_typeId == AZ::GetAssetClassId())
        {
            auto* asset = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(instancePointer);

            if (asset->GetId().IsValid())
            {
                productDependencySet[asset->GetId()] = AZ::Data::ProductDependencyInfo::CreateFlags(asset->GetAutoLoadBehavior());
            }
        }
        else if (classData->m_typeId == azrtti_typeid<AZ::Data::AssetId>())
        {
            auto* assetId = reinterpret_cast<AZ::Data::AssetId*>(instancePointer);

            if (assetId->IsValid())
            {
                // For asset ID dependencies, set the behavior to "NoLoad" so that loading the parent asset doesn't trigger a load
                // of the dependent asset.
                productDependencySet[*assetId] = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::NoLoad);
            }
        }
        else if (classData->m_azRtti && classData->m_azRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
        {
            auto* asset = reinterpret_cast<AzFramework::SimpleAssetReferenceBase*>(instancePointer);

            if (!asset->GetAssetPath().empty())
            {
                AZStd::string filePath = asset->GetAssetPath();
                AZStd::string fileExtension;
                if (!AzFramework::StringFunc::Path::GetExtension(filePath.c_str(), fileExtension))
                {
                    // GetFileFilter can return either
                    // 1) one file extension like "*.fileExtension"
                    // 2) one file extension like "fileExtension"
                    // 3) a semi colon separated list of file extensions like  "*.fileExtension1; *.fileExtension2"
                    // Please note that if file extension is missing from the path and we get a list of semicolon separated file extensions
                    // we will extract the first file extension and use that. 
                    fileExtension = asset->GetFileFilter();
                    AZStd::regex fileExtensionRegex("^(?:\\*\\.)?(\\w+);?");
                    AZStd::smatch match;
                    if (AZStd::regex_search(fileExtension, match, fileExtensionRegex))
                    {
                        fileExtension = match[1];
                        AzFramework::StringFunc::Path::ReplaceExtension(filePath, fileExtension.c_str());
                    }
                }

                productPathDependencySet.emplace(filePath, ProductPathDependencyType::ProductFile);
            }
        }
        else if(enumerateChildren)
        {

            auto beginCallback = [&serializeContext, &productDependencySet, &productPathDependencySet](void* instancePointer, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                // EnumerateInstance calls are already recursive, so no need to keep going, set enumerateChildren to false.
                return UpdateDependenciesFromClassData(serializeContext, instancePointer, classData, classElement, productDependencySet, productPathDependencySet, false);
            };
            AZ::SerializeContext::EnumerateInstanceCallContext callContext(
                beginCallback,
                {},
                &serializeContext,
                AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
                nullptr
            );

            return serializeContext.EnumerateInstance(&callContext, instancePointer, classData->m_typeId, classData, classElement);
        }
        return true;
    }

    void FillDependencyVectorFromSet(
        AZStd::vector<ProductDependency>& productDependencies,
        UniqueDependencyList& productDependencySet)
    {
        productDependencies.reserve(productDependencySet.size());

        for (const auto& thisEntry : productDependencySet)
        {
            productDependencies.emplace_back(thisEntry.first, thisEntry.second);
        }
    }

    bool GatherProductDependenciesForFile(
        AZ::SerializeContext& serializeContext,
        const AZStd::string& filePath,
        AZStd::vector<ProductDependency>& productDependencies,
        ProductPathDependencySet& productPathDependencySet)
    {
        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            return false;
        }
        UniqueDependencyList productDependencySet;

        // UpdateDependenciesFromClassData is also looking for assets. In some cases, the assets may not be ready to use
        // in UpdateDependenciesFromClassData, and have an invalid asset ID. This asset filter will be called with valid, ready to use assets,
        // but it's only called on assets and not other supported types, and it's only available when loading the file, and not on an in-memory stream.
        AZ::ObjectStream::FilterDescriptor assetReadyFilterDescriptor([&productDependencySet](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            if (filterInfo.m_assetId.IsValid())
            {
                productDependencySet[filterInfo.m_assetId] = AZ::Data::ProductDependencyInfo::CreateFlags(filterInfo.m_loadBehavior);
            }
            return false;
        });

        if (!AZ::ObjectStream::LoadBlocking(&fileStream, serializeContext, [&productDependencySet, &productPathDependencySet](void* instancePointer, const AZ::Uuid& classId, const AZ::SerializeContext* callbackSerializeContext)
        {
            auto classData = callbackSerializeContext->FindClassData(classId);
            // LoadBlocking only enumerates the topmost level objects, so call UpdateDependenciesFromClassData with enumerateChildren set.
            UpdateDependenciesFromClassData(*callbackSerializeContext, instancePointer, classData, nullptr, productDependencySet, productPathDependencySet, true);
            return true;
        }, assetReadyFilterDescriptor))
        {
            return false;
        }
        FillDependencyVectorFromSet(productDependencies, productDependencySet);
        return true;
    }

    bool GatherProductDependencies(
        AZ::SerializeContext& serializeContext,
        const void* obj,
        AZ::TypeId typeId,
        AZStd::vector<ProductDependency>& productDependencies,
        ProductPathDependencySet& productPathDependencySet,
        const DependencyHandler& handler)
    {
        if (obj == nullptr)
        {
            AZ_Error("AssetBuilderSDK", false, "Cannot gather product dependencies for null data.");
            return false;
        }

        // start with a set to make it easy to avoid duplicate entries.
        UniqueDependencyList productDependencySet;
        auto beginCallback = [&serializeContext, &productDependencySet, &productPathDependencySet, handler](void* instancePointer, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
        {
            // EnumerateObject already visits every element, so no need to enumerate farther, set enumerateChildren to false.
            return handler(serializeContext, instancePointer, classData, classElement, productDependencySet, productPathDependencySet, false);
        };
        bool enumerateResult = serializeContext.EnumerateInstanceConst(obj, typeId, beginCallback, {}, AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr, nullptr);

        FillDependencyVectorFromSet(productDependencies, productDependencySet);
        return enumerateResult;
    }

    bool OutputObject(const void* obj, AZ::TypeId typeId, AZStd::string_view outputPath, AZ::Data::AssetType assetType, AZ::u32 subId, JobProduct& jobProduct, AZ::SerializeContext* serializeContext, const DependencyHandler& handler)
    {
        if (!serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        }

        if(!serializeContext)
        {
            AZ_Error("AssetBuilderSDK", false, "Failed to retrieve serialization context.");
            return false;
        }

        jobProduct = JobProduct(outputPath, assetType, subId);

        if (GatherProductDependencies(*serializeContext, obj, typeId, jobProduct.m_dependencies, jobProduct.m_pathDependencies, handler))
        {
            jobProduct.m_dependenciesHandled = true;

            return true;
        }

        jobProduct = {};
        return false;
    }
}
