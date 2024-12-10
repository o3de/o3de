/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace UnitTest
{
    class LuaMaterialFunctorTests;
}

namespace AZ
{
    class ScriptAsset;
    class ScriptContext;

    namespace RPI
    {
        namespace LuaMaterialFunctorAPI
        {
            class ATOM_RPI_REFLECT_API CommonRuntimeConfiguration
            {
            public:
                CommonRuntimeConfiguration(
                    MaterialPropertyPsoHandling psoHandling,
                    const MaterialPropertyFlags* materialPropertyDependencies,
                    const MaterialPropertiesLayout* materialPropertiesLayout);

                //! Returns false if PSO changes are not allowed, and may report errors or warnings
                bool CheckPsoChangesAllowed();

            private:
                AZStd::string GetMaterialPropertyDependenciesString() const;

                MaterialPropertyPsoHandling m_psoHandling;
                bool m_psoChangesReported = false; //!< errors/warnings about PSO changes will only be reported once per execution of the functor
                const MaterialPropertyFlags* m_materialPropertyDependencies = nullptr;
                const MaterialPropertiesLayout* m_materialPropertiesLayout = nullptr;
            };

            //! Wraps MaterialFunctorAPI::ReadMaterialPropertyValues for LuaMaterialFunctor access
            class ATOM_RPI_REFLECT_API ReadMaterialPropertyValues
            {
            public:
                AZ_TYPE_INFO(LuaMaterialFunctorAPI::ReadMaterialPropertyValues, "{2CCCB9A9-AD4F-447C-B587-E7A91CEA8088}");

                template<typename LuaApiClass>
                static void ReflectSubclass(BehaviorContext::ClassBuilder<LuaApiClass>* subclassBuilder);

                ReadMaterialPropertyValues(
                    MaterialFunctorAPI::ReadMaterialPropertyValues* underlyingApi,
                    const MaterialNameContext* materialNameContext);

            protected:

                template<typename Type>
                Type GetMaterialPropertyValue(const char* name) const;

                MaterialPropertyIndex GetMaterialPropertyIndex(const char* name, const char* functionName) const;

                const MaterialPropertyValue& GetMaterialPropertyValue(MaterialPropertyIndex propertyIndex) const;

                bool HasMaterialValue(const char* name) const;

            private:

                const MaterialNameContext* m_materialNameContext;
                MaterialFunctorAPI::ReadMaterialPropertyValues* m_underlyingApi = nullptr;
            };

            //! Wraps RHI::RenderStates for LuaMaterialFunctor access
            class ATOM_RPI_REFLECT_API RenderStates
            {
            public:
                AZ_TYPE_INFO(LuaMaterialFunctorAPI::RenderStates, "{DF724568-0579-4E0D-95CB-1CD9AD484D2F}");

                static void Reflect(BehaviorContext* behaviorContext);

                explicit RenderStates(RHI::RenderStates* renderStates) : m_renderStates(renderStates) {}

                // Set MultisampleState...

                void SetMultisampleCustomPosition(AZStd::size_t multisampleCustomLocationIndex, uint8_t x, uint8_t y);
                void SetMultisampleCustomPositionCount(uint32_t value);
                void SetMultisampleCount(uint16_t value);
                void SetMultisampleQuality(uint16_t value);

                // Set RasterState...

                void SetFillMode(RHI::FillMode value);
                void SetCullMode(RHI::CullMode value);
                void SetDepthBias(int32_t value);
                void SetDepthBiasClamp(float value);
                void SetDepthBiasSlopeScale(float value);
                void SetMultisampleEnabled(bool value);
                void SetDepthClipEnabled(bool value);
                void SetConservativeRasterEnabled(bool value);
                void SetForcedSampleCount(uint32_t value);

                // Set BlendState...

                void SetAlphaToCoverageEnabled(bool value);
                void SetIndependentBlendEnabled(bool value);

                void SetBlendEnabled(AZStd::size_t targetIndex, bool value);
                void SetBlendWriteMask(AZStd::size_t targetIndex, uint32_t value);
                void SetBlendSource(AZStd::size_t targetIndex, RHI::BlendFactor value);
                void SetBlendDest(AZStd::size_t targetIndex, RHI::BlendFactor value);
                void SetBlendOp(AZStd::size_t targetIndex, RHI::BlendOp value);
                void SetBlendAlphaSource(AZStd::size_t targetIndex, RHI::BlendFactor value);
                void SetBlendAlphaDest(AZStd::size_t targetIndex, RHI::BlendFactor value);
                void SetBlendAlphaOp(AZStd::size_t targetIndex, RHI::BlendOp value);

                // Set DepthState...

                void SetDepthEnabled(bool value);
                void SetDepthWriteMask(RHI::DepthWriteMask value);
                void SetDepthComparisonFunc(RHI::ComparisonFunc value);

                // Set StencilState...

                void SetStencilEnabled(bool value);
                void SetStencilReadMask(uint32_t value);
                void SetStencilWriteMask(uint32_t value);
                void SetStencilFrontFaceFailOp(RHI::StencilOp value);
                void SetStencilFrontFaceDepthFailOp(RHI::StencilOp value);
                void SetStencilFrontFacePassOp(RHI::StencilOp value);
                void SetStencilFrontFaceFunc(RHI::ComparisonFunc value);
                void SetStencilBackFaceFailOp(RHI::StencilOp value);
                void SetStencilBackFaceDepthFailOp(RHI::StencilOp value);
                void SetStencilBackFacePassOp(RHI::StencilOp value);
                void SetStencilBackFaceFunc(RHI::ComparisonFunc value);

                // Clear MultisampleState override values...

                void ClearMultisampleCustomPosition(AZStd::size_t multisampleCustomLocationIndex);
                void ClearMultisampleCustomPositionCount();
                void ClearMultisampleCount();
                void ClearMultisampleQuality();

                // Clear RasterState override values...

                void ClearFillMode();
                void ClearCullMode();
                void ClearDepthBias();
                void ClearDepthBiasClamp();
                void ClearDepthBiasSlopeScale();
                void ClearMultisampleEnabled();
                void ClearDepthClipEnabled();
                void ClearConservativeRasterEnabled();
                void ClearForcedSampleCount();

                // Clear BlendState override values...

                void ClearAlphaToCoverageEnabled();
                void ClearIndependentBlendEnabled();

                void ClearBlendEnabled(AZStd::size_t targetIndex);
                void ClearBlendWriteMask(AZStd::size_t targetIndex);
                void ClearBlendSource(AZStd::size_t targetIndex);
                void ClearBlendDest(AZStd::size_t targetIndex);
                void ClearBlendOp(AZStd::size_t targetIndex);
                void ClearBlendAlphaSource(AZStd::size_t targetIndex);
                void ClearBlendAlphaDest(AZStd::size_t targetIndex);
                void ClearBlendAlphaOp(AZStd::size_t targetIndex);

                // Clear DepthState override values...

                void ClearDepthEnabled();
                void ClearDepthWriteMask();
                void ClearDepthComparisonFunc();

                // Clear StencilState override values...

                void ClearStencilEnabled();
                void ClearStencilReadMask();
                void ClearStencilWriteMask();
                void ClearStencilFrontFaceFailOp();
                void ClearStencilFrontFaceDepthFailOp();
                void ClearStencilFrontFacePassOp();
                void ClearStencilFrontFaceFunc();
                void ClearStencilBackFaceFailOp();
                void ClearStencilBackFaceDepthFailOp();
                void ClearStencilBackFacePassOp();
                void ClearStencilBackFaceFunc();

            private:
                RHI::RenderStates* m_renderStates = nullptr;
            };

            class ATOM_RPI_REFLECT_API ShaderItem
            {
            public:
                AZ_TYPE_INFO(LuaMaterialFunctorAPI::ShaderItem, "{F5BF0362-AA43-408A-96A8-6F9980A4CF93}");

                static void Reflect(BehaviorContext* behaviorContext);

                ShaderItem() = default;

                ShaderItem(
                    ShaderCollection::Item* shaderItem,
                    CommonRuntimeConfiguration* commonRuntimeConfiguration)
                    : m_commonRuntimeConfiguration(commonRuntimeConfiguration)
                    , m_shaderItem(shaderItem)
                {
                }

                LuaMaterialFunctorAPI::RenderStates GetRenderStatesOverride();
                void SetEnabled(bool enable);
                void SetDrawListTagOverride(const char* drawListTag);

                template<typename Type>
                void SetShaderOptionValue(const char* name, Type value);

            private:
                void SetShaderOptionValue(const Name& name, AZStd::function<bool(ShaderOptionGroup*, ShaderOptionIndex)> setValueCommand);

                CommonRuntimeConfiguration* m_commonRuntimeConfiguration = nullptr;
                ShaderCollection::Item* m_shaderItem = nullptr;
            };

            //! Wraps MaterialFunctorAPI::ConfigureShaders for LuaMaterialFunctor access
            class ATOM_RPI_REFLECT_API ConfigureShaders
            {
            public:
                AZ_TYPE_INFO(LuaMaterialFunctorAPI::ConfigureShaders, "{DD498919-A135-4430-857B-B00146AEB5EC}");

                template<typename LuaApiClass>
                static void ReflectSubclass(BehaviorContext::ClassBuilder<LuaApiClass>* subclassBuilder);

                ConfigureShaders(
                    MaterialFunctorAPI::ConfigureShaders* underlyingApi,
                    const MaterialNameContext* materialNameContext,
                    CommonRuntimeConfiguration* psoChangeChecker);

            protected:

                //! Set the value of a shader option. Applies to any shader that has an option with this name.
                //! @param name the name of the shader option to set
                //! @param value the new value for the shader option
                template<typename Type>
                bool SetShaderOptionValue(const char* name, Type value);

                AZStd::size_t GetShaderCount() const;
                LuaMaterialFunctorAPI::ShaderItem GetShader(AZStd::size_t index);
                LuaMaterialFunctorAPI::ShaderItem GetShaderByTag(const char* shaderTag);
                bool HasShaderWithTag(const char* shaderTag);

            private:

                MaterialFunctorAPI::ConfigureShaders* m_underlyingApi = nullptr;
                CommonRuntimeConfiguration* m_commonRuntimeConfiguration = nullptr;
                const MaterialNameContext* m_materialNameContext;
            };

            //! Wraps MaterialFunctorAPI::RuntimeContext with lua bindings
            class ATOM_RPI_REFLECT_API RuntimeContext
                : public CommonRuntimeConfiguration
                , public LuaMaterialFunctorAPI::ReadMaterialPropertyValues
                , public LuaMaterialFunctorAPI::ConfigureShaders
            {
            public:
                AZ_TYPE_INFO(LuaMaterialFunctorAPI::RuntimeContext, "{00FF6AE5-DE0A-41E2-B3F8-FBB9E265C399}");

                static void Reflect(BehaviorContext* behaviorContext);

                RuntimeContext(
                    MaterialFunctorAPI::RuntimeContext* runtimeContextImpl,
                    const MaterialPropertyFlags* materialPropertyDependencies,
                    const MaterialNameContext* materialNameContext);

                //! Sets the value of a constant in the Material ShaderResourceGroup
                template<typename T>
                bool SetShaderConstant(const char* name, T value);

                //! Sets the value of an intermediate material property, used to pass data to the material pipelines.
                template<typename T>
                bool SetInternalMaterialPropertyValue(const char* name, T value);

                // The subclass must have wrapper functions for the underlying implementations in the base classes,
                // in order to expose the entire API in lua in a single context class...
                template<typename Type>
                Type GetMaterialPropertyValue(const char* name) const;
                bool HasMaterialValue(const char* name) const;
                template<typename Type>
                bool SetShaderOptionValue(const char* name, Type value);
                AZStd::size_t GetShaderCount() const;
                LuaMaterialFunctorAPI::ShaderItem GetShader(AZStd::size_t index);
                LuaMaterialFunctorAPI::ShaderItem GetShaderByTag(const char* shaderTag);
                bool HasShaderWithTag(const char* shaderTag);

            private:

                RHI::ShaderInputConstantIndex GetShaderInputConstantIndex(const char* name, const char* functionName) const;

                MaterialFunctorAPI::RuntimeContext* m_runtimeContextImpl = nullptr;
                const MaterialNameContext* m_materialNameContext;
            };

            //! Wraps MaterialFunctorAPI::PipelineRuntimeContext with lua bindings
            class ATOM_RPI_REFLECT_API PipelineRuntimeContext
                : public CommonRuntimeConfiguration
                , public LuaMaterialFunctorAPI::ReadMaterialPropertyValues
                , public LuaMaterialFunctorAPI::ConfigureShaders
            {
            public:
                AZ_TYPE_INFO(PipelineRuntimeContext, "{632F1E52-79EE-4184-A7B0-55C0EEEC5AB2}");

                static void Reflect(BehaviorContext* behaviorContext);

                explicit PipelineRuntimeContext(
                    MaterialFunctorAPI::PipelineRuntimeContext* runtimeContextImpl,
                    const MaterialPropertyFlags* materialPropertyDependencies,
                    const MaterialNameContext* materialNameContext);

                // The subclass must have wrapper functions for the underlying implementations in the base classes,
                // in order to expose the entire API in lua in a single context class...
                template<typename Type>
                Type GetMaterialPropertyValue(const char* name) const;
                bool HasMaterialValue(const char* name) const;
                template<typename Type>
                bool SetShaderOptionValue(const char* name, Type value);
                AZStd::size_t GetShaderCount() const;
                LuaMaterialFunctorAPI::ShaderItem GetShader(AZStd::size_t index);
                LuaMaterialFunctorAPI::ShaderItem GetShaderByTag(const char* shaderTag);
                bool HasShaderWithTag(const char* shaderTag);
            };

            //! Wraps MaterialFunctorAPI::EditorContext with lua bindings
            class ATOM_RPI_REFLECT_API EditorContext
                : public LuaMaterialFunctorAPI::ReadMaterialPropertyValues
            {
            public:
                AZ_TYPE_INFO(LuaMaterialFunctorAPI::EditorContext, "{AAF380F0-9ED2-4BB7-8E60-656992B14B71}");

                static void Reflect(BehaviorContext* behaviorContext);

                EditorContext(
                    MaterialFunctorAPI::EditorContext* editorContextImpl,
                    const MaterialNameContext* materialNameContext);

                bool SetMaterialPropertyVisibility(const char* name, MaterialPropertyVisibility visibility);

                template<typename Type>
                bool SetMaterialPropertyMinValue(const char* name, Type value);

                template<typename Type>
                bool SetMaterialPropertyMaxValue(const char* name, Type value);

                template<typename Type>
                bool SetMaterialPropertySoftMinValue(const char* name, Type value);

                template<typename Type>
                bool SetMaterialPropertySoftMaxValue(const char* name, Type value);

                bool SetMaterialPropertyDescription(const char* name, const char* description);

                bool SetMaterialPropertyGroupVisibility(const char* name, MaterialPropertyGroupVisibility visibility);

                // The subclass must have wrapper functions for the underlying implementations in the base classes,
                // in order to expose the entire API in lua in a single context class...
                template<typename Type>
                Type GetMaterialPropertyValue(const char* name) const;
                bool HasMaterialValue(const char* name) const;

            private:

                MaterialFunctorAPI::EditorContext* m_editorContextImpl = nullptr;
                const MaterialNameContext* m_materialNameContext;
            };

            class ATOM_RPI_REFLECT_API Utilities
            {
                friend CommonRuntimeConfiguration;
                friend LuaMaterialFunctorAPI::RuntimeContext;
                friend LuaMaterialFunctorAPI::PipelineRuntimeContext;
                friend LuaMaterialFunctorAPI::EditorContext;
                friend LuaMaterialFunctorAPI::RenderStates;
                friend LuaMaterialFunctorAPI::ShaderItem;
            public:
                static void Reflect(BehaviorContext* behaviorContext);
                static constexpr char const DebugName[] = "LuaMaterialFunctor";

            private:
                static void Script_Error(const AZStd::string& message);
                static void Script_Warning(const AZStd::string& message);
                static void Script_Print(const AZStd::string& message);
            };

        } // namespace LuaMaterialFunctorAPI

        //! Materials can use this functor to create custom scripted operations.
        class ATOM_RPI_REFLECT_API LuaMaterialFunctor final
            : public RPI::MaterialFunctor
        {
            friend class LuaMaterialFunctorSourceData;
            friend class UnitTest::LuaMaterialFunctorTests;
        public:
            AZ_RTTI(AZ::RPI::LuaMaterialFunctor, "{1EBDFEC1-FC45-4506-9B0F-AE05FA3779E1}", RPI::MaterialFunctor);
            AZ_CLASS_ALLOCATOR(AZ::RPI::LuaMaterialFunctor, SystemAllocator);
            
            static void Reflect(ReflectContext* context);

            LuaMaterialFunctor();

            void Process(MaterialFunctorAPI::RuntimeContext& context) override;
            void Process(MaterialFunctorAPI::PipelineRuntimeContext& context) override;
            void Process(MaterialFunctorAPI::EditorContext& context) override;

        private:

            // Registers functions in a BehaviorContext so they can be exposed to Lua scripts.
            static void ReflectScriptContext(AZ::BehaviorContext* context);

            void InitScriptContext();

            // Utility function that returns either m_scriptBuffer or the content of m_scriptAsset, depending on which as the data
            const AZStd::vector<char>& GetScriptBuffer() const;

            const char* GetScriptDescription() const;

            // Only one of these will contain data, either an external asset or a built-in script buffer
            Data::Asset<ScriptAsset> m_scriptAsset;
            AZStd::vector<char> m_scriptBuffer;
            
            MaterialNameContext m_materialNameContext;
            
            enum class ScriptStatus
            {
                Uninitialized,
                Ready,
                Error
            };
            ScriptStatus m_scriptStatus = ScriptStatus::Uninitialized;
        };


    } // namespace Render

} // namespace AZ
