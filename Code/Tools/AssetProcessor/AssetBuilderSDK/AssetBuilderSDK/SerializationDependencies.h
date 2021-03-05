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

#include <AzCore/Asset/AssetCommon.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AZ
{
    class SerializeContext;
}

namespace AssetBuilderSDK
{
    using UniqueDependencyList = AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::ProductDependencyInfo::ProductDependencyFlags>;
    bool UpdateDependenciesFromClassData(
        const AZ::SerializeContext& serializeContext,
        void* instancePointer,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        UniqueDependencyList& productDependencySet,
        ProductPathDependencySet& productPathDependencySet,
        bool enumerateChildren);

    void FillDependencyVectorFromSet(AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies, UniqueDependencyList& productDependencySet);

    bool GatherProductDependenciesForFile(
        AZ::SerializeContext& serializeContext,
        const AZStd::string& filePath,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        ProductPathDependencySet& productPathDependencySet);

    using DependencyHandler = AZStd::function<bool(
        const AZ::SerializeContext& /*serializeContext*/,
        void* /*instancePointer*/,
        const AZ::SerializeContext::ClassData* /*classData*/,
        const AZ::SerializeContext::ClassElement* /*classElement*/,
        UniqueDependencyList& /*productDependencySet*/,
        ProductPathDependencySet& /*productPathDependencySet*/,
        bool enumerateChildren)>;

    bool GatherProductDependencies(
        AZ::SerializeContext& serializeContext,
        void* obj,
        AZ::TypeId typeId,
        AZStd::vector<ProductDependency>& productDependencies,
        ProductPathDependencySet& productPathDependencySet,
        const DependencyHandler& handler = &UpdateDependenciesFromClassData);

    template<class T>
    bool GatherProductDependencies(
        AZ::SerializeContext& serializeContext,
        AZ::Data::Asset<T>* obj,
        AZ::TypeId typeId,
        AZStd::vector<ProductDependency>& productDependencies,
        ProductPathDependencySet& productPathDependencySet,
        const DependencyHandler& handler = &UpdateDependenciesFromClassData)
    {
        AZ_Error("AssetBuilderSDK", false, "Can't output dependencies for AZ::Data::Asset<T>* - Use T* or another underlying type");
        return false;
    }

    template<class T>
    bool GatherProductDependencies(
        AZ::SerializeContext& serializeContext,
        T* obj,
        AZStd::vector<ProductDependency>& productDependencies,
        ProductPathDependencySet& productPathDependencySet,
        const DependencyHandler& handler = &UpdateDependenciesFromClassData)
    {
        return GatherProductDependencies(serializeContext, obj, azrtti_typeid<T>(), productDependencies, productPathDependencySet, handler);
    }

    bool OutputObject(void* obj, AZ::TypeId typeId, AZStd::string_view outputPath, AZ::Data::AssetType assetType, AZ::u32 subId, JobProduct& jobProduct, AZ::SerializeContext* serializeContext = nullptr,
        const DependencyHandler& handler = &UpdateDependenciesFromClassData);

    template<class T>
    bool OutputObject(T* obj, AZStd::string_view outputPath, AZ::Data::AssetType assetType, AZ::u32 subId, JobProduct& jobProduct, AZ::SerializeContext* serializeContext = nullptr,
        const DependencyHandler& handler = &UpdateDependenciesFromClassData)
    {
        return OutputObject(obj, azrtti_typeid<T>(), outputPath, assetType, subId, jobProduct, serializeContext, handler);
    }

    template<class T>
    bool OutputObject(AZ::Data::Asset<T>* obj, AZStd::string_view outputPath, AZ::Data::AssetType assetType, AZ::u32 subId, JobProduct& jobProduct, AZ::SerializeContext* serializeContext = nullptr,
        const DependencyHandler& handler = &UpdateDependenciesFromClassData)
    {
        AZ_Error("AssetBuilderSDK", false, "Can't output dependencies for AZ::Data::Asset<T>* - Use T* or another underlying type");
        return false;
    }
}
