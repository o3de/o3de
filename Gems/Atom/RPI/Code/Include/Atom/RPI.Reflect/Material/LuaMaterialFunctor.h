/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
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
        //! Materials can use this functor to create custom scripted operations.
        class LuaMaterialFunctor final
            : public RPI::MaterialFunctor
        {
            friend class LuaMaterialFunctorSourceData;
            friend class UnitTest::LuaMaterialFunctorTests;
        public:
            AZ_RTTI(AZ::RPI::LuaMaterialFunctor, "{1EBDFEC1-FC45-4506-9B0F-AE05FA3779E1}", RPI::MaterialFunctor);
            AZ_CLASS_ALLOCATOR(AZ::RPI::LuaMaterialFunctor, SystemAllocator, 0);
            
            static void Reflect(ReflectContext* context);

            LuaMaterialFunctor();

            void Process(RuntimeContext& context) override;
            void Process(EditorContext& context) override;

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

            AZStd::unique_ptr<AZ::BehaviorContext> m_sriptBehaviorContext;
            AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
            
            // These are prefix strings that will be applied to every name lookup in the lua functor.
            // This allows the lua script to be reused in different contexts.
            AZStd::string m_propertyNamePrefix;
            AZStd::string m_srgNamePrefix;
            AZStd::string m_optionsNamePrefix;

            enum class ScriptStatus
            {
                Uninitialized,
                Ready,
                Error
            };
            ScriptStatus m_scriptStatus = ScriptStatus::Uninitialized;
        };


        //! Provides some shared code for LuaMaterialFunctorRuntimeContext and LuaMaterialFunctorEditorContext.
        class LuaMaterialFunctorCommonContext
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::LuaMaterialFunctorCommonContext, "{2CCCB9A9-AD4F-447C-B587-E7A91CEA8088}");

            explicit LuaMaterialFunctorCommonContext(MaterialFunctor::RuntimeContext* runtimeContextImpl,
                const MaterialPropertyFlags* materialPropertyDependencies,
                const AZStd::string& propertyNamePrefix,
                const AZStd::string& srgNamePrefix,
                const AZStd::string& optionsNamePrefix);

            explicit LuaMaterialFunctorCommonContext(MaterialFunctor::EditorContext* editorContextImpl,
                const MaterialPropertyFlags* materialPropertyDependencies,
                const AZStd::string& propertyNamePrefix,
                const AZStd::string& srgNamePrefix,
                const AZStd::string& optionsNamePrefix);
            
            //! Returns false if PSO changes are not allowed, and may report errors or warnings
            bool CheckPsoChangesAllowed();

        protected:

            template<typename Type>
            Type GetMaterialPropertyValue(const char* name) const;

            MaterialPropertyIndex GetMaterialPropertyIndex(const char* name, const char* functionName) const;

            const MaterialPropertyValue& GetMaterialPropertyValue(MaterialPropertyIndex propertyIndex) const;
            
            MaterialPropertyPsoHandling GetMaterialPropertyPsoHandling() const;

            RHI::ConstPtr<MaterialPropertiesLayout> GetMaterialPropertiesLayout() const;

            AZStd::string GetMaterialPropertyDependenciesString() const;

            // These are prefix strings that will be applied to every name lookup in the lua functor.
            // This allows the lua script to be reused in different contexts.
            const AZStd::string& m_propertyNamePrefix;
            const AZStd::string& m_srgNamePrefix;
            const AZStd::string& m_optionsNamePrefix;

        private:

            // Only one of these will be valid
            MaterialFunctor::RuntimeContext* m_runtimeContextImpl = nullptr;
            MaterialFunctor::EditorContext* m_editorContextImpl = nullptr;
            const MaterialPropertyFlags* m_materialPropertyDependencies = nullptr;
            bool m_psoChangesReported = false; //!< errors/warnings about PSO changes will only be reported once per execution of the functor
        };

        //! Wraps RHI::RenderStates for LuaMaterialFunctor access
        class LuaMaterialFunctorRenderStates
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::LuaMaterialFunctorRenderStates, "{DF724568-0579-4E0D-95CB-1CD9AD484D2F}");

            static void Reflect(BehaviorContext* behaviorContext);

            explicit LuaMaterialFunctorRenderStates(RHI::RenderStates* renderStates) : m_renderStates(renderStates) {}

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

        class LuaMaterialFunctorShaderItem
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::LuaMaterialFunctorShaderItem, "{F5BF0362-AA43-408A-96A8-6F9980A4CF93}");

            static void Reflect(BehaviorContext* behaviorContext);

            LuaMaterialFunctorShaderItem() :
                m_context(nullptr), m_shaderItem(nullptr) {}

            explicit LuaMaterialFunctorShaderItem(LuaMaterialFunctorCommonContext* context, ShaderCollection::Item* shaderItem) :
                m_context(context), m_shaderItem(shaderItem) {}

            LuaMaterialFunctorRenderStates GetRenderStatesOverride();
            void SetEnabled(bool enable);
            void SetDrawListTagOverride(const char* drawListTag);

            template<typename Type>
            void SetShaderOptionValue(const char* name, Type value);

        private:
            void SetShaderOptionValue(const Name& name, AZStd::function<bool(ShaderOptionGroup*, ShaderOptionIndex)> setValueCommand);

            LuaMaterialFunctorCommonContext* m_context = nullptr;
            ShaderCollection::Item* m_shaderItem = nullptr;
        };

        //! Wraps MaterialFunctor::RuntimeContext with lua bindings
        class LuaMaterialFunctorRuntimeContext : public LuaMaterialFunctorCommonContext
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::LuaMaterialFunctorRuntimeContext, "{00FF6AE5-DE0A-41E2-B3F8-FBB9E265C399}");

            static void Reflect(BehaviorContext* behaviorContext);

            explicit LuaMaterialFunctorRuntimeContext(MaterialFunctor::RuntimeContext* runtimeContextImpl,
                const MaterialPropertyFlags* materialPropertyDependencies,
                const AZStd::string& propertyNamePrefix,
                const AZStd::string& srgNamePrefix,
                const AZStd::string& optionsNamePrefix);

            template<typename Type>
            Type GetMaterialPropertyValue(const char* name) const;

            //! Set the value of a shader option. Applies to any shader that has an option with this name.
            //! @param name the name of the shader option to set
            //! @param value the new value for the shader option
            template<typename Type>
            bool SetShaderOptionValue(const char* name, Type value);

            template<typename T>
            bool SetShaderConstant(const char* name, T value);

            AZStd::size_t GetShaderCount() const;
            LuaMaterialFunctorShaderItem GetShader(AZStd::size_t index);
            LuaMaterialFunctorShaderItem GetShaderByTag(const char* shaderTag);
            bool HasShaderWithTag(const char* shaderTag);

        private:

            bool SetShaderOptionValueHelper(const char* name, AZStd::function<bool(ShaderOptionGroup*, ShaderOptionIndex)> setValueCommand);

            RHI::ShaderInputConstantIndex GetShaderInputConstantIndex(const char* name, const char* functionName) const;

            MaterialFunctor::RuntimeContext* m_runtimeContextImpl = nullptr;
        };

        //! Wraps MaterialFunctor::EditorContext with lua bindings
        class LuaMaterialFunctorEditorContext : public LuaMaterialFunctorCommonContext
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::LuaMaterialFunctorEditorContext, "{AAF380F0-9ED2-4BB7-8E60-656992B14B71}");

            static void Reflect(BehaviorContext* behaviorContext);

            explicit LuaMaterialFunctorEditorContext(MaterialFunctor::EditorContext* editorContextImpl,
                const MaterialPropertyFlags* materialPropertyDependencies,
                const AZStd::string& propertyNamePrefix,
                const AZStd::string& srgNamePrefix,
                const AZStd::string& optionsNamePrefix);

            template<typename Type>
            Type GetMaterialPropertyValue(const char* name) const;

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

        private:

            MaterialFunctor::EditorContext* m_editorContextImpl = nullptr;
        };

        class LuaMaterialFunctorUtilities
        {
            friend LuaMaterialFunctorCommonContext;
            friend LuaMaterialFunctorRuntimeContext;
            friend LuaMaterialFunctorEditorContext;
            friend LuaMaterialFunctorRenderStates;
            friend LuaMaterialFunctorShaderItem;
        public:
            static void Reflect(BehaviorContext* behaviorContext);
            static constexpr char const DebugName[] = "LuaMaterialFunctor";

        private:
            static void Script_Error(const AZStd::string& message);
            static void Script_Warning(const AZStd::string& message);
            static void Script_Print(const AZStd::string& message);
        };


    } // namespace Render

} // namespace AZ
