/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/LuaScriptUtilities.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        void LuaMaterialFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<LuaMaterialFunctor, RPI::MaterialFunctor>()
                    ->Version(1)
                    ->Field("scriptAsset", &LuaMaterialFunctor::m_scriptAsset)
                    ->Field("materialNameContext", &LuaMaterialFunctor::m_materialNameContext)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                ReflectScriptContext(behaviorContext);
            }
        }

        LuaMaterialFunctor::LuaMaterialFunctor() = default;

        void LuaMaterialFunctor::ReflectScriptContext(AZ::BehaviorContext* behaviorContext)
        {
            // We don't need any functions in Image, but just need BehaviorContext to be aware of this
            // type so we can pass around image pointers within lua scripts.
            behaviorContext->Class<Image>(); 

            LuaMaterialFunctorAPI::RenderStates::Reflect(behaviorContext);
            LuaMaterialFunctorAPI::ShaderItem::Reflect(behaviorContext);
            LuaScriptUtilities::Reflect(behaviorContext);
            LuaMaterialFunctorAPI::RuntimeContext::Reflect(behaviorContext);
            LuaMaterialFunctorAPI::PipelineRuntimeContext::Reflect(behaviorContext);
            LuaMaterialFunctorAPI::EditorContext::Reflect(behaviorContext);
        }

        const AZStd::vector<char>& LuaMaterialFunctor::GetScriptBuffer() const
        {
            if (!m_scriptBuffer.empty())
            {
                AZ_Warning("LuaMaterialFunctor", m_scriptAsset.GetId().IsValid() == false, "LuaMaterialFunctor has both a built-in script and an external script asset. The external script will be ignored.");
                return m_scriptBuffer;
            }
            else if (m_scriptAsset.IsReady())
            {
                return m_scriptAsset->m_data.GetScriptBuffer();
            }
            else
            {
                AZ_Error("LuaMaterialFunctor", false, "LuaMaterialFunctor has no script data.");
                return m_scriptBuffer;
            }
        }

        const char* LuaMaterialFunctor::GetScriptDescription() const
        {
            if (!m_scriptBuffer.empty())
            {
                return "<built-in functor script>";
            }
            else if (m_scriptAsset.IsReady())
            {
                return m_scriptAsset.GetHint().c_str();
            }
            else
            {
                return "<none>";
            }
        }

        void LuaMaterialFunctor::InitScriptContext()
        {
            ScriptContext* scriptContext{};
            ScriptSystemRequestBus::BroadcastResult(scriptContext, &ScriptSystemRequests::GetContext, ScriptContextIds::DefaultScriptContextId);
            if (scriptContext == nullptr)
            {
                AZ_ErrorOnce(LuaScriptUtilities::DebugName, false, "Global script context is not available. Cannot initialize scripts");
                m_scriptStatus = ScriptStatus::Uninitialized;
                return;
            }

            if (m_scriptStatus != ScriptStatus::Error)
            {
                auto scriptBuffer = GetScriptBuffer();

                // Remove any existing Process or ProcessEditor from the global table
                // This prevents the case where the Lua ScriptContext could contain a Process/ProcessEditor
                // from a previous call to ScriptContext::Execute, if current script doesn't provide those functions
                scriptContext->RemoveGlobal("Process");
                scriptContext->RemoveGlobal("ProcessEditor");

                if (!scriptContext->Execute(scriptBuffer.data(), GetScriptDescription(), scriptBuffer.size()))
                {
                    AZ_Error(LuaScriptUtilities::DebugName, false, "Error initializing script '%s'.", m_scriptAsset.ToString<AZStd::string>().c_str());
                    m_scriptStatus = ScriptStatus::Error;
                }
                else
                {
                    m_scriptStatus = ScriptStatus::Ready;
                }
            }
        }

        void LuaMaterialFunctor::Process(MaterialFunctorAPI::RuntimeContext& context)
        {
            AZ_PROFILE_FUNCTION(RPI);

            InitScriptContext();

            if (m_scriptStatus == ScriptStatus::Ready)
            {
                ScriptContext* scriptContext{};
                ScriptSystemRequestBus::BroadcastResult(scriptContext, &ScriptSystemRequests::GetContext, ScriptContextIds::DefaultScriptContextId);
                LuaMaterialFunctorAPI::RuntimeContext luaContext{&context, &GetMaterialPropertyDependencies(), &m_materialNameContext};
                AZ::ScriptDataContext call;
                if (scriptContext->Call("Process", call))
                {
                    call.PushArg(luaContext);
                    call.CallExecute();
                }
            }
        }

        void LuaMaterialFunctor::Process(MaterialFunctorAPI::PipelineRuntimeContext& context)
        {
            AZ_PROFILE_FUNCTION(RPI);

            InitScriptContext();

            if (m_scriptStatus == ScriptStatus::Ready)
            {
                ScriptContext* scriptContext{};
                ScriptSystemRequestBus::BroadcastResult(scriptContext, &ScriptSystemRequests::GetContext, ScriptContextIds::DefaultScriptContextId);
                LuaMaterialFunctorAPI::PipelineRuntimeContext luaContext{&context, &GetMaterialPropertyDependencies(), &m_materialNameContext};
                AZ::ScriptDataContext call;
                if (scriptContext->Call("Process", call))
                {
                    call.PushArg(luaContext);
                    call.CallExecute();
                }
            }
        }

        void LuaMaterialFunctor::Process(MaterialFunctorAPI::EditorContext& context)
        {
            AZ_PROFILE_FUNCTION(RPI);

            InitScriptContext();

            if (m_scriptStatus == ScriptStatus::Ready)
            {
                ScriptContext* scriptContext{};
                ScriptSystemRequestBus::BroadcastResult(scriptContext, &ScriptSystemRequests::GetContext, ScriptContextIds::DefaultScriptContextId);
                LuaMaterialFunctorAPI::EditorContext luaContext{&context, &m_materialNameContext};
                AZ::ScriptDataContext call;
                if (scriptContext->Call("ProcessEditor", call))
                {
                    call.PushArg(luaContext);
                    call.CallExecute();
                }
            }
        }

        LuaMaterialFunctorAPI::CommonRuntimeConfiguration::CommonRuntimeConfiguration(
            MaterialPropertyPsoHandling psoHandling,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialPropertiesLayout* materialPropertiesLayout)
            : m_psoHandling(psoHandling)
            , m_materialPropertyDependencies(materialPropertyDependencies)
            , m_materialPropertiesLayout(materialPropertiesLayout)
        {
        }

        AZStd::string LuaMaterialFunctorAPI::CommonRuntimeConfiguration::GetMaterialPropertyDependenciesString() const
        {
            AZStd::vector<AZStd::string> propertyList;
            for (size_t i = 0; i < m_materialPropertyDependencies->size(); ++i)
            {
                if ((*m_materialPropertyDependencies)[i])
                {
                    propertyList.push_back(m_materialPropertiesLayout->GetPropertyDescriptor(MaterialPropertyIndex{i})->GetName().GetStringView());
                }
            }

            AZStd::string propertyListString;
            AzFramework::StringFunc::Join(propertyListString, propertyList.begin(), propertyList.end(), ", ");

            return propertyListString;
        }

        bool LuaMaterialFunctorAPI::CommonRuntimeConfiguration::CheckPsoChangesAllowed()
        {
            if (m_psoHandling == MaterialPropertyPsoHandling::Error)
            {
                if (!m_psoChangesReported)
                {
                    LuaScriptUtilities::Error(
                        AZStd::string::format(
                            "The following material properties must not be changed at runtime because they impact Pipeline State Objects: %s", GetMaterialPropertyDependenciesString().c_str()));
                    
                    m_psoChangesReported = true;
                }

                return false;
            }
            else if (m_psoHandling == MaterialPropertyPsoHandling::Warning)
            {
                if (!m_psoChangesReported)
                {
                    LuaScriptUtilities::Warning(
                        AZStd::string::format(
                            "The following material properties should not be changed at runtime because they impact Pipeline State Objects: %s", GetMaterialPropertyDependenciesString().c_str()));
                    
                    m_psoChangesReported = true;
                }
            }

            return true;
        }

        LuaMaterialFunctorAPI::ReadMaterialPropertyValues::ReadMaterialPropertyValues(
            MaterialFunctorAPI::ReadMaterialPropertyValues* underlyingApi,
            const MaterialNameContext* materialNameContext)
            : m_underlyingApi(underlyingApi)
            , m_materialNameContext(materialNameContext)
        {
        }

        MaterialPropertyIndex LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyIndex(const char* name, const char* functionName) const
        {
            MaterialPropertyIndex propertyIndex;

            Name propertyFullName{name};
            m_materialNameContext->ContextualizeProperty(propertyFullName);
            
            propertyIndex = m_underlyingApi->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyFullName);

            if (!propertyIndex.IsValid())
            {
                LuaScriptUtilities::Error(AZStd::string::format("%s() could not find property '%s'", functionName, propertyFullName.GetCStr()));
            }

            return propertyIndex;
        }

        const MaterialPropertyValue& LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(MaterialPropertyIndex propertyIndex) const
        {
            return m_underlyingApi->GetMaterialPropertyValue(propertyIndex);
        }

        template<typename Type>
        Type LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(const char* name) const
        {
            MaterialPropertyIndex index = GetMaterialPropertyIndex(name, "GetMaterialPropertyValue");

            if (!index.IsValid())
            {
                return {};
            }

            const MaterialPropertyValue& value = GetMaterialPropertyValue(index);

            if (!value.IsValid())
            {
                LuaScriptUtilities::Error(AZStd::string::format("GetMaterialPropertyValue() got invalid value for property '%s'", name));
                return {};
            }

            if (!value.Is<Type>())
            {
                LuaScriptUtilities::Error(AZStd::string::format("GetMaterialPropertyValue() accessed property '%s' using the wrong data type.", name));
                return {};
            }

            return value.GetValue<Type>();
        } 

        // Specialize for type Image* because that will be more intuitive within Lua.
        // The script can then check the result for nil without calling "get()".
        // For example, "GetMaterialPropertyValue_Image(name) == nil" rather than "GetMaterialPropertyValue_Image(name):get() == nil"
        template<>
        Image* LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(const char* name) const
        {
            return GetMaterialPropertyValue<Data::Instance<Image>>(name).get();
        }

        template<typename LuaApiClass>
        void LuaMaterialFunctorAPI::ReadMaterialPropertyValues::ReflectSubclass(BehaviorContext::ClassBuilder<LuaApiClass>* subclassBuilder)
        {
            subclassBuilder
                ->Method("GetMaterialPropertyValue_bool", &LuaApiClass::template GetMaterialPropertyValue<bool>)
                ->Method("GetMaterialPropertyValue_int", &LuaApiClass::template GetMaterialPropertyValue<int32_t>)
                ->Method("GetMaterialPropertyValue_uint", &LuaApiClass::template GetMaterialPropertyValue<uint32_t>)
                ->Method("GetMaterialPropertyValue_enum", &LuaApiClass::template GetMaterialPropertyValue<uint32_t>)
                ->Method("GetMaterialPropertyValue_float", &LuaApiClass::template GetMaterialPropertyValue<float>)
                ->Method("GetMaterialPropertyValue_Vector2", &LuaApiClass::template GetMaterialPropertyValue<Vector2>)
                ->Method("GetMaterialPropertyValue_Vector3", &LuaApiClass::template GetMaterialPropertyValue<Vector3>)
                ->Method("GetMaterialPropertyValue_Vector4", &LuaApiClass::template GetMaterialPropertyValue<Vector4>)
                ->Method("GetMaterialPropertyValue_Color", &LuaApiClass::template GetMaterialPropertyValue<Color>)
                ->Method("GetMaterialPropertyValue_Image", &LuaApiClass::template GetMaterialPropertyValue<Image*>)
                ->Method("HasMaterialProperty", &LuaApiClass::HasMaterialValue)
                ;
        }

        void LuaMaterialFunctorAPI::RuntimeContext::Reflect(BehaviorContext* behaviorContext)
        {
            auto builder = behaviorContext->Class<LuaMaterialFunctorAPI::RuntimeContext>();
            builder
                ->Method("SetShaderConstant_bool", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<bool>)
                ->Method("SetShaderConstant_int", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<int32_t>)
                ->Method("SetShaderConstant_uint", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<uint32_t>)
                ->Method("SetShaderConstant_float", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<float>)
                ->Method("SetShaderConstant_Vector2", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<Vector2>)
                ->Method("SetShaderConstant_Vector3", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<Vector3>)
                ->Method("SetShaderConstant_Vector4", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<Vector4>)
                ->Method("SetShaderConstant_Color", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<Color>)
                ->Method("SetShaderConstant_Matrix3x3", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<Matrix3x3>)
                ->Method("SetShaderConstant_Matrix4x4", &LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant<Matrix4x4>)
                ->Method("SetInternalMaterialPropertyValue_bool", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<bool>)
                ->Method("SetInternalMaterialPropertyValue_int", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<int32_t>)
                ->Method("SetInternalMaterialPropertyValue_uint", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<uint32_t>)
                ->Method("SetInternalMaterialPropertyValue_enum", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<uint32_t>)
                ->Method("SetInternalMaterialPropertyValue_float", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<float>)
                // I'm not really sure what use case there might be for passing these data types to the material pipeline, but we might as well provide
                // them to remain consistent with the types that are supported by the GetMaterialPropertyValue function above.
                ->Method("SetInternalMaterialPropertyValue_Vector2", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<Vector2>)
                ->Method("SetInternalMaterialPropertyValue_Vector3", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<Vector3>)
                ->Method("SetInternalMaterialPropertyValue_Vector4", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<Vector4>)
                ->Method("SetInternalMaterialPropertyValue_Color", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<Color>)
                ->Method("SetInternalMaterialPropertyValue_Image", &LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue<Image*>)
                ;

            LuaMaterialFunctorAPI::ReadMaterialPropertyValues::ReflectSubclass<LuaMaterialFunctorAPI::RuntimeContext>(&builder);
            LuaMaterialFunctorAPI::ConfigureShaders::ReflectSubclass<LuaMaterialFunctorAPI::RuntimeContext>(&builder);
        }

        template<typename LuaApiClass>
        void LuaMaterialFunctorAPI::ConfigureShaders::ReflectSubclass(BehaviorContext::ClassBuilder<LuaApiClass>* subclassBuilder)
        {
            subclassBuilder
                ->Method("SetShaderOptionValue_bool", &LuaApiClass::template SetShaderOptionValue<bool>)
                ->Method("SetShaderOptionValue_uint", &LuaApiClass::template SetShaderOptionValue<uint32_t>)
                ->Method("SetShaderOptionValue_enum", &LuaApiClass::template SetShaderOptionValue<const char*>)
                ->Method("GetShaderCount", &LuaApiClass::GetShaderCount)
                ->Method("GetShader", &LuaApiClass::GetShader)
                ->Method("GetShaderByTag", &LuaApiClass::GetShaderByTag)
                ->Method("HasShaderWithTag", &LuaApiClass::HasShaderWithTag)
                ;
        }

        LuaMaterialFunctorAPI::ConfigureShaders::ConfigureShaders(
            MaterialFunctorAPI::ConfigureShaders* underlyingApi,
            const MaterialNameContext* materialNameContext,
            CommonRuntimeConfiguration* commonRuntimeConfiguration)
            : m_underlyingApi(underlyingApi)
            , m_materialNameContext(materialNameContext)
            , m_commonRuntimeConfiguration(commonRuntimeConfiguration)
        {
        }

        bool LuaMaterialFunctorAPI::ReadMaterialPropertyValues::HasMaterialValue(const char* name) const
        {
            Name propertyFullName{name};
            m_materialNameContext->ContextualizeProperty(propertyFullName);

            MaterialPropertyIndex propertyIndex = m_underlyingApi->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyFullName);
            return propertyIndex.IsValid();
        }

        template<>
        bool LuaMaterialFunctorAPI::ConfigureShaders::SetShaderOptionValue(const char* name, const char* value)
        {
            Name optionName{name};
            m_materialNameContext->ContextualizeShaderOption(optionName);
            return m_underlyingApi->SetShaderOptionValue(optionName, Name{value});
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::ConfigureShaders::SetShaderOptionValue(const char* name, Type value)
        {
            Name optionName{name};
            m_materialNameContext->ContextualizeShaderOption(optionName);
            return m_underlyingApi->SetShaderOptionValue(optionName, ShaderOptionValue{value});
        }

        RHI::ShaderInputConstantIndex LuaMaterialFunctorAPI::RuntimeContext::GetShaderInputConstantIndex(const char* name, const char* functionName) const
        {
            Name fullInputName{name};
            m_materialNameContext->ContextualizeSrgInput(fullInputName);

            RHI::ShaderInputConstantIndex index = m_runtimeContextImpl->GetShaderResourceGroup()->FindShaderInputConstantIndex(fullInputName);

            if (!index.IsValid())
            {
                LuaScriptUtilities::Error(AZStd::string::format("%s() could not find shader input '%s'", functionName, fullInputName.GetCStr()));
            }

            return index;
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::RuntimeContext::SetShaderConstant(const char* name, Type value)
        {
            RHI::ShaderInputConstantIndex index = GetShaderInputConstantIndex(name, "SetShaderConstant");
            if (index.IsValid())
            {
                return m_runtimeContextImpl->GetShaderResourceGroup()->SetConstant(index, value);
            }

            return false;
        }

        AZStd::size_t LuaMaterialFunctorAPI::ConfigureShaders::GetShaderCount() const
        {
            return m_underlyingApi->GetShaderCount();
        }

        LuaMaterialFunctorAPI::ShaderItem LuaMaterialFunctorAPI::ConfigureShaders::GetShader(AZStd::size_t index)
        {
            if (index < GetShaderCount())
            {
                return LuaMaterialFunctorAPI::ShaderItem{&(*m_underlyingApi->m_localShaderCollection)[index], m_commonRuntimeConfiguration};
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format("GetShader(%zu) is invalid.", index));
                return {};
            }
        }

        LuaMaterialFunctorAPI::ShaderItem LuaMaterialFunctorAPI::ConfigureShaders::GetShaderByTag(const char* shaderTag)
        {
            const AZ::Name tag{shaderTag};
            if (m_underlyingApi->m_localShaderCollection->HasShaderTag(tag))
            {
                return LuaMaterialFunctorAPI::ShaderItem{&(*m_underlyingApi->m_localShaderCollection)[tag], m_commonRuntimeConfiguration};
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format(
                    "GetShaderByTag('%s') is invalid: Could not find a shader with the tag '%s'.", tag.GetCStr(), tag.GetCStr()));
                return {};
            }
        }

        bool LuaMaterialFunctorAPI::ConfigureShaders::HasShaderWithTag(const char* shaderTag)
        {
            return m_underlyingApi->m_localShaderCollection->HasShaderTag(AZ::Name{shaderTag});
        }

        LuaMaterialFunctorAPI::RuntimeContext::RuntimeContext(
            MaterialFunctorAPI::RuntimeContext* runtimeContextImpl,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialNameContext* materialNameContext)
            : LuaMaterialFunctorAPI::CommonRuntimeConfiguration(runtimeContextImpl->GetMaterialPropertyPsoHandling(), materialPropertyDependencies, runtimeContextImpl->GetMaterialPropertiesLayout())
            , LuaMaterialFunctorAPI::ReadMaterialPropertyValues(runtimeContextImpl, materialNameContext)
            , LuaMaterialFunctorAPI::ConfigureShaders(runtimeContextImpl, materialNameContext, this)
            , m_runtimeContextImpl(runtimeContextImpl)
            , m_materialNameContext(materialNameContext)
        {
        }

        template<typename Type>
        Type LuaMaterialFunctorAPI::RuntimeContext::GetMaterialPropertyValue(const char* name) const
        {
            return LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Type>(name);
        }

        bool LuaMaterialFunctorAPI::RuntimeContext::HasMaterialValue(const char* name) const
        {
            return LuaMaterialFunctorAPI::ReadMaterialPropertyValues::HasMaterialValue(name);
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::RuntimeContext::SetShaderOptionValue(const char* name, Type value)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::SetShaderOptionValue(name, value);
        }

        AZStd::size_t LuaMaterialFunctorAPI::RuntimeContext::GetShaderCount() const
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::GetShaderCount();
        }

        LuaMaterialFunctorAPI::ShaderItem LuaMaterialFunctorAPI::RuntimeContext::GetShader(AZStd::size_t index)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::GetShader(index);
        }

        LuaMaterialFunctorAPI::ShaderItem LuaMaterialFunctorAPI::RuntimeContext::GetShaderByTag(const char* shaderTag)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::GetShaderByTag(shaderTag);
        }
        
        bool LuaMaterialFunctorAPI::RuntimeContext::HasShaderWithTag(const char* shaderTag)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::HasShaderWithTag(shaderTag);
        }

        template<typename T>
        bool LuaMaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue(const char* name, T value)
        {
            return m_runtimeContextImpl->SetInternalMaterialPropertyValue(AZ::Name{name}, value);
        }

        void LuaMaterialFunctorAPI::PipelineRuntimeContext::Reflect(BehaviorContext* behaviorContext)
        {
            auto builder = behaviorContext->Class<PipelineRuntimeContext>();
            LuaMaterialFunctorAPI::ReadMaterialPropertyValues::ReflectSubclass<LuaMaterialFunctorAPI::PipelineRuntimeContext>(&builder);
            LuaMaterialFunctorAPI::ConfigureShaders::ReflectSubclass<LuaMaterialFunctorAPI::PipelineRuntimeContext>(&builder);
        }

        LuaMaterialFunctorAPI::PipelineRuntimeContext::PipelineRuntimeContext(
            MaterialFunctorAPI::PipelineRuntimeContext* runtimeContextImpl,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialNameContext* materialNameContext)
            : LuaMaterialFunctorAPI::CommonRuntimeConfiguration(runtimeContextImpl->GetMaterialPropertyPsoHandling(), materialPropertyDependencies, runtimeContextImpl->GetMaterialPropertiesLayout())
            , LuaMaterialFunctorAPI::ReadMaterialPropertyValues(runtimeContextImpl, materialNameContext)
            , LuaMaterialFunctorAPI::ConfigureShaders(runtimeContextImpl, materialNameContext, this)
        {
        }

        template<typename Type>
        Type LuaMaterialFunctorAPI::PipelineRuntimeContext::GetMaterialPropertyValue(const char* name) const
        {
            return LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Type>(name);
        }

        bool LuaMaterialFunctorAPI::PipelineRuntimeContext::HasMaterialValue(const char* name) const
        {
            return LuaMaterialFunctorAPI::ReadMaterialPropertyValues::HasMaterialValue(name);
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::PipelineRuntimeContext::SetShaderOptionValue(const char* name, Type value)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::SetShaderOptionValue(name, value);
        }

        AZStd::size_t LuaMaterialFunctorAPI::PipelineRuntimeContext::GetShaderCount() const
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::GetShaderCount();
        }

        LuaMaterialFunctorAPI::ShaderItem LuaMaterialFunctorAPI::PipelineRuntimeContext::GetShader(AZStd::size_t index)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::GetShader(index);
        }

        LuaMaterialFunctorAPI::ShaderItem LuaMaterialFunctorAPI::PipelineRuntimeContext::GetShaderByTag(const char* shaderTag)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::GetShaderByTag(shaderTag);
        }

        bool LuaMaterialFunctorAPI::PipelineRuntimeContext::HasShaderWithTag(const char* shaderTag)
        {
            return LuaMaterialFunctorAPI::ConfigureShaders::HasShaderWithTag(shaderTag);
        }

        void LuaMaterialFunctorAPI::EditorContext::EditorContext::Reflect(BehaviorContext* behaviorContext)
        {
            auto builder = behaviorContext->Class<LuaMaterialFunctorAPI::EditorContext>();
            builder
                ->Method("SetMaterialPropertyVisibility", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyVisibility)
                ->Method("SetMaterialPropertyDescription", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyDescription)
                ->Method("SetMaterialPropertyMinValue_int", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMinValue<int32_t>)
                ->Method("SetMaterialPropertyMinValue_uint", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMinValue<uint32_t>)
                ->Method("SetMaterialPropertyMinValue_float", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMinValue<float>)
                ->Method("SetMaterialPropertyMaxValue_int", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMaxValue<int32_t>)
                ->Method("SetMaterialPropertyMaxValue_uint", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMaxValue<uint32_t>)
                ->Method("SetMaterialPropertyMaxValue_float", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMaxValue<float>)
                ->Method("SetMaterialPropertySoftMinValue_int", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMinValue<int32_t>)
                ->Method("SetMaterialPropertySoftMinValue_uint", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMinValue<uint32_t>)
                ->Method("SetMaterialPropertySoftMinValue_float", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMinValue<float>)
                ->Method("SetMaterialPropertySoftMaxValue_int", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMaxValue<int32_t>)
                ->Method("SetMaterialPropertySoftMaxValue_uint", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMaxValue<uint32_t>)
                ->Method("SetMaterialPropertySoftMaxValue_float", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMaxValue<float>)
                ->Method("SetMaterialPropertyGroupVisibility", &LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyGroupVisibility)
                ;

            LuaMaterialFunctorAPI::ReadMaterialPropertyValues::ReflectSubclass<LuaMaterialFunctorAPI::EditorContext>(&builder);
        }

        LuaMaterialFunctorAPI::EditorContext::EditorContext(
            MaterialFunctorAPI::EditorContext* editorContextImpl,
            const MaterialNameContext* materialNameContext)
            : LuaMaterialFunctorAPI::ReadMaterialPropertyValues(editorContextImpl, materialNameContext)
            , m_editorContextImpl(editorContextImpl)
            , m_materialNameContext(materialNameContext)
        {
        }

        template<typename Type>
        Type LuaMaterialFunctorAPI::EditorContext::GetMaterialPropertyValue(const char* name) const
        {
            return LuaMaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Type>(name);
        }

        bool LuaMaterialFunctorAPI::EditorContext::HasMaterialValue(const char* name) const
        {
            return LuaMaterialFunctorAPI::ReadMaterialPropertyValues::HasMaterialValue(name);
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMinValue(const char* name, Type value)
        {
            const char* functionName = "SetMaterialPropertyMinValue";

            MaterialPropertyIndex index = GetMaterialPropertyIndex(name, functionName);
            if (!index.IsValid())
            {
                return false;
            }

            return m_editorContextImpl->SetMaterialPropertyMinValue(index, value);
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyMaxValue(const char* name, Type value)
        {
            const char* functionName = "SetMaterialPropertyMaxValue";

            MaterialPropertyIndex index = GetMaterialPropertyIndex(name, functionName);
            if (!index.IsValid())
            {
                return false;
            }

            return m_editorContextImpl->SetMaterialPropertyMaxValue(index, value);
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMinValue(const char* name, Type value)
        {
            const char* functionName = "SetMaterialPropertySoftMinValue";

            MaterialPropertyIndex index = GetMaterialPropertyIndex(name, functionName);
            if (!index.IsValid())
            {
                return false;
            }

            return m_editorContextImpl->SetMaterialPropertySoftMinValue(index, value);
        }

        template<typename Type>
        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMaxValue(const char* name, Type value)
        {
            const char* functionName = "SetMaterialPropertySoftMaxValue";

            MaterialPropertyIndex index = GetMaterialPropertyIndex(name, functionName);
            if (!index.IsValid())
            {
                return false;
            }

            return m_editorContextImpl->SetMaterialPropertySoftMaxValue(index, value);
        }
        
        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyGroupVisibility(const char* name, MaterialPropertyGroupVisibility visibility)
        {
            if (m_editorContextImpl)
            {
                Name fullName{name};
                m_materialNameContext->ContextualizeProperty(fullName);
                return m_editorContextImpl->SetMaterialPropertyGroupVisibility(fullName, visibility);
            }
            return false;
        }

        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyVisibility(const char* name, MaterialPropertyVisibility visibility)
        {
            if (m_editorContextImpl)
            {
                Name fullName{name};
                m_materialNameContext->ContextualizeProperty(fullName);
                return m_editorContextImpl->SetMaterialPropertyVisibility(fullName, visibility);
            }
            return false;
        }

        bool LuaMaterialFunctorAPI::EditorContext::SetMaterialPropertyDescription(const char* name, const char* description)
        {
            if (m_editorContextImpl)
            {
                Name fullName{name};
                m_materialNameContext->ContextualizeProperty(fullName);
                return m_editorContextImpl->SetMaterialPropertyDescription(fullName, description);
            }
            return false;
        }

        template<>
        void LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue(const char* name, const char* value);

        void LuaMaterialFunctorAPI::ShaderItem::Reflect(AZ::BehaviorContext* behaviorContext)
        {
            behaviorContext->Class<LuaMaterialFunctorAPI::ShaderItem>()
                ->Method("GetRenderStatesOverride", &LuaMaterialFunctorAPI::ShaderItem::GetRenderStatesOverride)
                ->Method("SetEnabled", &LuaMaterialFunctorAPI::ShaderItem::SetEnabled)
                ->Method("SetDrawListTagOverride", &LuaMaterialFunctorAPI::ShaderItem::SetDrawListTagOverride)
                ->Method("SetShaderOptionValue_bool", &LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue<bool>)
                ->Method("SetShaderOptionValue_uint", &LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue<uint32_t>)
                ->Method("SetShaderOptionValue_enum", &LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue<const char*>)
                ;
        }

        LuaMaterialFunctorAPI::RenderStates LuaMaterialFunctorAPI::ShaderItem::GetRenderStatesOverride()
        {
            if (m_commonRuntimeConfiguration->CheckPsoChangesAllowed() && m_shaderItem)
            {
                return LuaMaterialFunctorAPI::RenderStates{m_shaderItem->GetRenderStatesOverlay()};
            }
            else
            {
                static RHI::RenderStates dummyRenderStates;
                return LuaMaterialFunctorAPI::RenderStates{&dummyRenderStates};
            }
        }

        void LuaMaterialFunctorAPI::ShaderItem::SetEnabled(bool enable)
        {
            if (m_shaderItem)
            {
                m_shaderItem->SetEnabled(enable);
            }
        }

        void LuaMaterialFunctorAPI::ShaderItem::SetDrawListTagOverride(const char* drawListTag)
        {
            if (m_shaderItem)
            {
                m_shaderItem->SetDrawListTagOverride(Name{drawListTag});
            }
        }

        void LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue(
            const Name& name, AZStd::function<bool(ShaderOptionGroup*, ShaderOptionIndex)> setValueCommand)
        {
            ShaderOptionGroup* shaderOptionGroup = m_shaderItem->GetShaderOptions();
            const ShaderOptionGroupLayout* layout = shaderOptionGroup->GetShaderOptionLayout();

            ShaderOptionIndex optionIndex = layout->FindShaderOptionIndex(Name{name});
            if (!optionIndex.IsValid())
            {
                return;
            }

            if (!m_shaderItem->MaterialOwnsShaderOption(optionIndex))
            {
                LuaScriptUtilities::Error(
                    AZStd::string::format(
                        "Shader option '%s' is not owned by the shader '%s'.", name.GetCStr(), m_shaderItem->GetShaderTag().GetCStr()));
                return;
            }

            setValueCommand(shaderOptionGroup, optionIndex);
        }

        template<>
        void LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue(const char* name, const char* value)
        {
            if (m_shaderItem)
            {
                SetShaderOptionValue(Name{name}, [value](ShaderOptionGroup* optionGroup, ShaderOptionIndex optionIndex) {
                    return optionGroup->SetValue(optionIndex, Name{value});
                });
            }
        }

        template<typename Type>
        void LuaMaterialFunctorAPI::ShaderItem::SetShaderOptionValue(const char* name, Type value)
        {
            if (m_shaderItem)
            {
                SetShaderOptionValue(Name{name}, [value](ShaderOptionGroup* optionGroup, ShaderOptionIndex optionIndex) {
                    return optionGroup->SetValue(optionIndex, ShaderOptionValue{value});
                });
            }
        }

        void LuaMaterialFunctorAPI::RenderStates::Reflect(AZ::BehaviorContext* behaviorContext)
        {
            auto classBuilder = behaviorContext->Class<LuaMaterialFunctorAPI::RenderStates>();

            #define TEMP_REFLECT_RENDERSTATE_METHODS(PropertyName) \
                classBuilder->Method("Set" AZ_STRINGIZE(PropertyName), AZ_JOIN(&LuaMaterialFunctorAPI::RenderStates::Set, PropertyName)); \
                classBuilder->Method("Clear" AZ_STRINGIZE(PropertyName), AZ_JOIN(&LuaMaterialFunctorAPI::RenderStates::Clear, PropertyName));

            TEMP_REFLECT_RENDERSTATE_METHODS(MultisampleCustomPosition)
            TEMP_REFLECT_RENDERSTATE_METHODS(MultisampleCustomPositionCount)
            TEMP_REFLECT_RENDERSTATE_METHODS(MultisampleCount)
            TEMP_REFLECT_RENDERSTATE_METHODS(MultisampleQuality)
            TEMP_REFLECT_RENDERSTATE_METHODS(FillMode)
            TEMP_REFLECT_RENDERSTATE_METHODS(CullMode)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthBias)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthBiasClamp)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthBiasSlopeScale)
            TEMP_REFLECT_RENDERSTATE_METHODS(MultisampleEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthClipEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(ConservativeRasterEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(ForcedSampleCount)
            TEMP_REFLECT_RENDERSTATE_METHODS(AlphaToCoverageEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(IndependentBlendEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendWriteMask)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendSource)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendDest)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendAlphaSource)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendAlphaDest)
            TEMP_REFLECT_RENDERSTATE_METHODS(BlendAlphaOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthWriteMask)
            TEMP_REFLECT_RENDERSTATE_METHODS(DepthComparisonFunc)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilEnabled)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilReadMask)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilWriteMask)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilFrontFaceFailOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilFrontFaceDepthFailOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilFrontFacePassOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilFrontFaceFunc)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilBackFaceFailOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilBackFaceDepthFailOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilBackFacePassOp)
            TEMP_REFLECT_RENDERSTATE_METHODS(StencilBackFaceFunc)

            #undef TEMP_REFLECT_RENDERSTATE_METHODS
        }

        #define TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(PropertyName, DataType, Field, InvalidValue) \
            void AZ_JOIN(LuaMaterialFunctorAPI::RenderStates::Set, PropertyName)(DataType value)         \
            {                                                                                       \
                Field = value;                                                                      \
            }                                                                                       \
            void AZ_JOIN(LuaMaterialFunctorAPI::RenderStates::Clear, PropertyName)()                     \
            {                                                                                       \
                Field = InvalidValue;                                                               \
            }

        #define TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(PropertyName, DataType, Field, InvalidValue)           \
            void AZ_JOIN(LuaMaterialFunctorAPI::RenderStates::Set, PropertyName)(AZStd::size_t targetIndex, DataType value)  \
            {                                                                                                           \
                if (targetIndex < RHI::Limits::Pipeline::AttachmentColorCountMax)                                       \
                {                                                                                                       \
                    m_renderStates->m_blendState.m_targets[targetIndex].Field = value;                                  \
                }                                                                                                       \
                else                                                                                                    \
                {                                                                                                       \
                    LuaScriptUtilities::Error(AZStd::string::format(                                    \
                        "Set" AZ_STRINGIZE(PropertyName) "(%zu,...) index is out of range. Must be less than %u.",      \
                        targetIndex, RHI::Limits::Pipeline::AttachmentColorCountMax));                                  \
                }                                                                                                       \
            }                                                                                                           \
            void AZ_JOIN(LuaMaterialFunctorAPI::RenderStates::Clear, PropertyName)(AZStd::size_t targetIndex)                \
            {                                                                                                           \
                if (targetIndex < RHI::Limits::Pipeline::AttachmentColorCountMax)                                       \
                {                                                                                                       \
                    m_renderStates->m_blendState.m_targets[targetIndex].Field = InvalidValue;                           \
                }                                                                                                       \
                else                                                                                                    \
                {                                                                                                       \
                    LuaScriptUtilities::Error(AZStd::string::format(                                    \
                        "Clear" AZ_STRINGIZE(PropertyName) "(%zu,...) index is out of range. Must be less than %u.",    \
                        targetIndex, RHI::Limits::Pipeline::AttachmentColorCountMax));                                  \
                }                                                                                                       \
            }

        void LuaMaterialFunctorAPI::RenderStates::SetMultisampleCustomPosition(AZStd::size_t multisampleCustomLocationIndex, uint8_t x, uint8_t y)
        {
            if (multisampleCustomLocationIndex < RHI::Limits::Pipeline::MultiSampleCustomLocationsCountMax)
            {
                m_renderStates->m_multisampleState.m_customPositions[multisampleCustomLocationIndex].m_x = x;
                m_renderStates->m_multisampleState.m_customPositions[multisampleCustomLocationIndex].m_y = y;
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format("SetMultisampleCustomPosition(%zu,...) index is out of range. Must be less than %u.",
                    multisampleCustomLocationIndex, RHI::Limits::Pipeline::MultiSampleCustomLocationsCountMax));
            }
        }

        void LuaMaterialFunctorAPI::RenderStates::ClearMultisampleCustomPosition(AZStd::size_t multisampleCustomLocationIndex)
        {
            if (multisampleCustomLocationIndex < RHI::Limits::Pipeline::MultiSampleCustomLocationsCountMax)
            {
                m_renderStates->m_multisampleState.m_customPositions[multisampleCustomLocationIndex].m_x = RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize;
                m_renderStates->m_multisampleState.m_customPositions[multisampleCustomLocationIndex].m_y = RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize;
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format("ClearMultisampleCustomPosition(%zu,...) index is out of range. Must be less than %u.",
                    multisampleCustomLocationIndex, RHI::Limits::Pipeline::MultiSampleCustomLocationsCountMax));
            }
        }

        void LuaMaterialFunctorAPI::RenderStates::SetMultisampleCustomPositionCount(uint32_t value)
        {
            if (value == RHI::RenderStates_InvalidUInt || value < RHI::Limits::Pipeline::MultiSampleCustomLocationsCountMax)
            {
                m_renderStates->m_multisampleState.m_customPositionsCount = value;
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format("SetMultisampleCustomPositionCount(%u) value is out of range. Must be less than %u.",
                    value, RHI::Limits::Pipeline::MultiSampleCustomLocationsCountMax));
            }
        }

        void LuaMaterialFunctorAPI::RenderStates::ClearMultisampleCustomPositionCount()
        {                                                                                      
            m_renderStates->m_multisampleState.m_customPositionsCount = RHI::RenderStates_InvalidUInt;
        }

        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(MultisampleCount,                uint16_t,             m_renderStates->m_multisampleState.m_samples,                             RHI::RenderStates_InvalidUInt16)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(MultisampleQuality,              uint16_t,             m_renderStates->m_multisampleState.m_quality,                             RHI::RenderStates_InvalidUInt16)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(FillMode,                        RHI::FillMode,        m_renderStates->m_rasterState.m_fillMode,                                 RHI::FillMode::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(CullMode,                        RHI::CullMode,        m_renderStates->m_rasterState.m_cullMode,                                 RHI::CullMode::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthBias,                       int32_t,              m_renderStates->m_rasterState.m_depthBias,                                RHI::RenderStates_InvalidInt)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthBiasClamp,                  float,                m_renderStates->m_rasterState.m_depthBiasClamp,                           RHI::RenderStates_InvalidFloat)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthBiasSlopeScale,             float,                m_renderStates->m_rasterState.m_depthBiasSlopeScale,                      RHI::RenderStates_InvalidFloat)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(MultisampleEnabled,              bool,                 m_renderStates->m_rasterState.m_multisampleEnable,                        RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthClipEnabled,                bool,                 m_renderStates->m_rasterState.m_depthClipEnable,                          RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(ConservativeRasterEnabled,       bool,                 m_renderStates->m_rasterState.m_conservativeRasterEnable,                 RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(ForcedSampleCount,               uint32_t,             m_renderStates->m_rasterState.m_forcedSampleCount,                        RHI::RenderStates_InvalidUInt)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(AlphaToCoverageEnabled,          bool,                 m_renderStates->m_blendState.m_alphaToCoverageEnable,                     RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(IndependentBlendEnabled,         bool,                 m_renderStates->m_blendState.m_independentBlendEnable,                    RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendEnabled,          bool,                 m_enable,                                                                 RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendWriteMask,        uint32_t,             m_writeMask,                                                              RHI::RenderStates_InvalidUInt)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendSource,           RHI::BlendFactor,     m_blendSource,                                                            RHI::BlendFactor::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendDest,             RHI::BlendFactor,     m_blendDest,                                                              RHI::BlendFactor::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendOp,               RHI::BlendOp,         m_blendOp,                                                                RHI::BlendOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendAlphaSource,      RHI::BlendFactor,     m_blendAlphaSource,                                                       RHI::BlendFactor::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendAlphaDest,        RHI::BlendFactor,     m_blendAlphaDest,                                                         RHI::BlendFactor::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(BlendAlphaOp,          RHI::BlendOp,         m_blendAlphaOp,                                                           RHI::BlendOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthEnabled,                    bool,                 m_renderStates->m_depthStencilState.m_depth.m_enable,                     RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthWriteMask,                  RHI::DepthWriteMask,  m_renderStates->m_depthStencilState.m_depth.m_writeMask,                  RHI::DepthWriteMask::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(DepthComparisonFunc,             RHI::ComparisonFunc,  m_renderStates->m_depthStencilState.m_depth.m_func,                       RHI::ComparisonFunc::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilEnabled,                  bool,                 m_renderStates->m_depthStencilState.m_stencil.m_enable,                   RHI::RenderStates_InvalidBool)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilReadMask,                 uint32_t,             m_renderStates->m_depthStencilState.m_stencil.m_readMask,                 RHI::RenderStates_InvalidUInt)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilWriteMask,                uint32_t,             m_renderStates->m_depthStencilState.m_stencil.m_writeMask,                RHI::RenderStates_InvalidUInt)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilFrontFaceFailOp,          RHI::StencilOp,       m_renderStates->m_depthStencilState.m_stencil.m_frontFace.m_failOp,       RHI::StencilOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilFrontFaceDepthFailOp,     RHI::StencilOp,       m_renderStates->m_depthStencilState.m_stencil.m_frontFace.m_depthFailOp,  RHI::StencilOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilFrontFacePassOp,          RHI::StencilOp,       m_renderStates->m_depthStencilState.m_stencil.m_frontFace.m_passOp,       RHI::StencilOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilFrontFaceFunc,            RHI::ComparisonFunc,  m_renderStates->m_depthStencilState.m_stencil.m_frontFace.m_func,         RHI::ComparisonFunc::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilBackFaceFailOp,           RHI::StencilOp,       m_renderStates->m_depthStencilState.m_stencil.m_backFace.m_failOp,        RHI::StencilOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilBackFaceDepthFailOp,      RHI::StencilOp,       m_renderStates->m_depthStencilState.m_stencil.m_backFace.m_depthFailOp,   RHI::StencilOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilBackFacePassOp,           RHI::StencilOp,       m_renderStates->m_depthStencilState.m_stencil.m_backFace.m_passOp,        RHI::StencilOp::Invalid)
        TEMP_DEFINE_RENDERSTATE_METHODS_COMMON(StencilBackFaceFunc,             RHI::ComparisonFunc,  m_renderStates->m_depthStencilState.m_stencil.m_backFace.m_func,          RHI::ComparisonFunc::Invalid)

        #undef TEMP_DEFINE_RENDERSTATE_METHODS_COMMON
        #undef TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET

    } // namespace RPI
} // namespace AZ
