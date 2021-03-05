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
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Asset/AssetDescription.h>

namespace ScriptCanvas
{
    class RuntimeAsset;

    class RuntimeAssetDescription : public AssetDescription
    {
    public:

        AZ_TYPE_INFO(RuntimeAssetDescription, "{7F49CB81-0655-4AF6-A1B5-95417A6FD568}");

        RuntimeAssetDescription()
            : AssetDescription(
                azrtti_typeid<RuntimeAsset>(),
                "Script Canvas Runtime",
                "Script Canvas Runtime Graph",
                "@devassets@/scriptcanvas",
                ".scriptcanvas_compiled",
                "Script Canvas Runtime",
                "Untitled-%i",
                "Script Canvas Files (*.scriptcanvas_compiled)",
                "Script Canvas Runtime",
                "Script Canvas Runtime",
                "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas.png",
                AZ::Color(1.0f,0.0f,0.0f,1.0f),
                false
            )
        {}
    };

    struct RuntimeData
    {
        AZ_TYPE_INFO(RuntimeData, "{A935EBBC-D167-4C59-927C-5D98C6337B9C}");
        AZ_CLASS_ALLOCATOR(RuntimeData, AZ::SystemAllocator, 0);
        RuntimeData() = default;
        ~RuntimeData() = default;
        RuntimeData(const RuntimeData&) = default;
        RuntimeData& operator=(const RuntimeData&) = default;
        RuntimeData(RuntimeData&&);
        RuntimeData& operator=(RuntimeData&&);

        static void Reflect(AZ::ReflectContext* reflectContext);

        GraphData m_graphData;
        VariableData m_variableData;
    };

    class RuntimeAssetBase
        : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(RuntimeAssetBase, "{19BAD220-E505-4443-AA95-743106748F37}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(RuntimeAssetBase, AZ::SystemAllocator, 0);
        RuntimeAssetBase(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {

        }

    };
    template <typename DataType>
    class RuntimeAssetTyped
        : public RuntimeAssetBase
    {
    public:
        AZ_RTTI(RuntimeAssetBase, "{C925213E-A1FA-4487-831F-9551A984700E}", RuntimeAssetBase);
        AZ_CLASS_ALLOCATOR(RuntimeAssetBase, AZ::SystemAllocator, 0);

        RuntimeAssetTyped(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : RuntimeAssetBase(assetId, status)
        {

        }
        ~RuntimeAssetTyped() override { m_runtimeData.m_graphData.Clear(true); }

        static const char* GetFileExtension() { return "scriptcanvas_compiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_compiled"; }

        const DataType& GetData() const { return m_runtimeData; }
        DataType& GetData() { return m_runtimeData; }
        void SetData(const DataType& runtimeData)
        {
            m_runtimeData = runtimeData;
            // When setting data instead of serializing, immediately mark the asset as ready.
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }

        DataType m_runtimeData;

    protected:
        friend class RuntimeAssetHandler;
        RuntimeAssetTyped(const RuntimeAssetTyped&) = delete;

    };


    class RuntimeAsset : public RuntimeAssetTyped<RuntimeData>
    {
    public:

        AZ_RTTI(RuntimeAsset, "{3E2AC8CD-713F-453E-967F-29517F331784}", RuntimeAssetTyped<RuntimeData>);

        RuntimeAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : RuntimeAssetTyped<RuntimeData>(assetId, status)
        {

        }

    };

    class RuntimeFunctionAsset;

    class RuntimeFunctionAssetDescription : public AssetDescription
    {
    public:

        AZ_TYPE_INFO(RuntimeFunctionAssetDescription, "{7F7BE1A5-9447-41C2-9190-18580075094C}");

        RuntimeFunctionAssetDescription()
            : AssetDescription(
                azrtti_typeid<RuntimeFunctionAsset>(),
                "Script Canvas Runtime Function",
                "Script Canvas Runtime Function Graph",
                "@devassets@/scriptcanvas",
                ".scriptcanvas_fn_compiled",
                "Script Canvas Runtime Function",
                "Untitled-Function-%i",
                "Script Canvas Compiled Function Files (*.scriptcanvas_fn_compiled)",
                "Script Canvas Runtime Function",
                "Script Canvas Runtime Function",
                "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas_Function.png",
                AZ::Color(1.0f,0.0f,0.0f,1.0f),
                false
            )
        {}
    };

    struct FunctionRuntimeData : public RuntimeData
    {
        AZ_TYPE_INFO(FunctionRuntimeData, "{1734C569-7D40-4491-9EEE-A225E333C9BA}");
        AZ_CLASS_ALLOCATOR(FunctionRuntimeData, AZ::SystemAllocator, 0);
        FunctionRuntimeData() = default;
        ~FunctionRuntimeData() = default;
        FunctionRuntimeData(const FunctionRuntimeData&) = default;
        FunctionRuntimeData& operator=(const FunctionRuntimeData&) = default;
        FunctionRuntimeData(FunctionRuntimeData&&);
        FunctionRuntimeData& operator=(FunctionRuntimeData&&);

        static void Reflect(AZ::ReflectContext* reflectContext);

        size_t m_version;
        AZStd::string m_name;

        AZStd::vector< AZ::EntityId > m_executionNodeOrder;
        AZStd::vector< VariableId > m_variableOrder;
    };


    class RuntimeFunctionAsset
        : public RuntimeAssetTyped<FunctionRuntimeData>
    {
    public:
        AZ_RTTI(RuntimeFunctionAsset, "{E22967AC-7673-4778-9125-AF49D82CAF9F}", RuntimeAssetTyped<FunctionRuntimeData>);
        AZ_CLASS_ALLOCATOR(RuntimeFunctionAsset, AZ::SystemAllocator, 0);

        RuntimeFunctionAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : RuntimeAssetTyped<FunctionRuntimeData>(assetId, status)
        {}

        ~RuntimeFunctionAsset() override
        {
            m_runtimeData.m_graphData.Clear(true);
        }

        void SetData(const FunctionRuntimeData& runtimeData)
        {
            m_runtimeData = runtimeData;
        }

        static const char* GetFileExtension() { return "scriptcanvas_fn_compiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_fn_compiled"; }

        friend class RuntimeFunctionAssetHandler;        
    };
}
