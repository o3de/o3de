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
        }

        LuaMaterialFunctor::LuaMaterialFunctor()
        {
            // [GFX TODO][ATOM-13648] Add local system allocator to material system
            // ScriptContext creates a new allocator if null (default) is passed in.
            // Temporarily using system allocator for preventing hitting the max allocator number.
            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>(AZ_CRC_CE("MaterialFunctor"), &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
            m_sriptBehaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

            ReflectScriptContext(m_sriptBehaviorContext.get());

            m_scriptContext->BindTo(m_sriptBehaviorContext.get());
        }

        void LuaMaterialFunctor::ReflectScriptContext(AZ::BehaviorContext* behaviorContext)
        {
            AZ::MathReflect(behaviorContext);

            // We don't need any functions in Image, but just need BehaviorContext to be aware of this
            // type so we can pass around image pointers within lua scripts.
            behaviorContext->Class<Image>(); 

            MaterialPropertyDescriptor::Reflect(behaviorContext);
            ReflectMaterialDynamicMetadata(behaviorContext);

            LuaMaterialFunctorRenderStates::Reflect(behaviorContext);
            LuaMaterialFunctorShaderItem::Reflect(behaviorContext);
            LuaScriptUtilities::Reflect(behaviorContext);
            LuaMaterialFunctorRuntimeContext::Reflect(behaviorContext);
            LuaMaterialFunctorEditorContext::Reflect(behaviorContext);
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
            if (m_scriptStatus == ScriptStatus::Uninitialized)
            {
                auto scriptBuffer = GetScriptBuffer();

                if (!m_scriptContext->Execute(scriptBuffer.data(), GetScriptDescription(), scriptBuffer.size()))
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

        void LuaMaterialFunctor::Process(RuntimeContext& context)
        {
            AZ_PROFILE_FUNCTION(RPI);

            InitScriptContext();

            if (m_scriptStatus == ScriptStatus::Ready)
            {
                LuaMaterialFunctorRuntimeContext luaContext{&context, &GetMaterialPropertyDependencies(), m_materialNameContext};
                AZ::ScriptDataContext call;
                if (m_scriptContext->Call("Process", call))
                {
                    call.PushArg(luaContext);
                    call.CallExecute();
                }
            }
        }

        void LuaMaterialFunctor::Process(EditorContext& context)
        {
            AZ_PROFILE_FUNCTION(RPI);

            InitScriptContext();

            if (m_scriptStatus == ScriptStatus::Ready)
            {
                LuaMaterialFunctorEditorContext luaContext{&context, &GetMaterialPropertyDependencies(), m_materialNameContext};
                AZ::ScriptDataContext call;
                if (m_scriptContext->Call("ProcessEditor", call))
                {
                    call.PushArg(luaContext);
                    call.CallExecute();
                }
            }
        }

        LuaMaterialFunctorCommonContext::LuaMaterialFunctorCommonContext(MaterialFunctor::RuntimeContext* runtimeContextImpl,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialNameContext& materialNameContext)
            : m_runtimeContextImpl(runtimeContextImpl)
            , m_materialPropertyDependencies(materialPropertyDependencies)
            , m_materialNameContext(materialNameContext)
        {
        }

        LuaMaterialFunctorCommonContext::LuaMaterialFunctorCommonContext(MaterialFunctor::EditorContext* editorContextImpl,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialNameContext& materialNameContext)
            : m_editorContextImpl(editorContextImpl)
            , m_materialPropertyDependencies(materialPropertyDependencies)
            , m_materialNameContext(materialNameContext)
        {
        }
        
        MaterialPropertyPsoHandling LuaMaterialFunctorCommonContext::GetMaterialPropertyPsoHandling() const
        {
            if (m_runtimeContextImpl)
            {
                return m_runtimeContextImpl->GetMaterialPropertyPsoHandling();
            }
            else
            {
                return m_editorContextImpl->GetMaterialPropertyPsoHandling();
            }
        }

        RHI::ConstPtr<MaterialPropertiesLayout> LuaMaterialFunctorCommonContext::GetMaterialPropertiesLayout() const
        {
            if (m_runtimeContextImpl)
            {
                return m_runtimeContextImpl->GetMaterialPropertiesLayout();
            }
            else
            {
                return m_editorContextImpl->GetMaterialPropertiesLayout();
            }
        }

        AZStd::string LuaMaterialFunctorCommonContext::GetMaterialPropertyDependenciesString() const
        {
            AZStd::vector<AZStd::string> propertyList;
            for (size_t i = 0; i < m_materialPropertyDependencies->size(); ++i)
            {
                if ((*m_materialPropertyDependencies)[i])
                {
                    propertyList.push_back(GetMaterialPropertiesLayout()->GetPropertyDescriptor(MaterialPropertyIndex{i})->GetName().GetStringView());
                }
            }

            AZStd::string propertyListString;
            AzFramework::StringFunc::Join(propertyListString, propertyList.begin(), propertyList.end(), ", ");

            return propertyListString;
        }
        
        bool LuaMaterialFunctorCommonContext::CheckPsoChangesAllowed()
        {
            if (GetMaterialPropertyPsoHandling() == MaterialPropertyPsoHandling::Error)
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
            else if (GetMaterialPropertyPsoHandling() == MaterialPropertyPsoHandling::Warning)
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

        MaterialPropertyIndex LuaMaterialFunctorCommonContext::GetMaterialPropertyIndex(const char* name, const char* functionName) const
        {
            MaterialPropertyIndex propertyIndex;

            Name propertyFullName{name};
            m_materialNameContext.ContextualizeProperty(propertyFullName);
            
            propertyIndex = GetMaterialPropertiesLayout()->FindPropertyIndex(propertyFullName);

            if (!propertyIndex.IsValid())
            {
                LuaScriptUtilities::Error(AZStd::string::format("%s() could not find property '%s'", functionName, propertyFullName.GetCStr()));
            }

            return propertyIndex;
        }

        const MaterialPropertyValue& LuaMaterialFunctorCommonContext::GetMaterialPropertyValue(MaterialPropertyIndex propertyIndex) const
        {
            if (m_runtimeContextImpl)
            {
                return m_runtimeContextImpl->GetMaterialPropertyValue(propertyIndex);
            }
            else if (m_editorContextImpl)
            {
                return m_editorContextImpl->GetMaterialPropertyValue(propertyIndex);
            }
            else
            {
                AZ_Assert(false, "Context not initialized properly");
                static MaterialPropertyValue defaultValue;
                return defaultValue;
            }
        }

        template<typename Type>
        Type LuaMaterialFunctorCommonContext::GetMaterialPropertyValue(const char* name) const
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
        Image* LuaMaterialFunctorCommonContext::GetMaterialPropertyValue(const char* name) const
        {
            return GetMaterialPropertyValue<Data::Instance<Image>>(name).get();
        }

        // Explicit specialization must be declared before expanding it in LuaMaterialFunctorRuntimeContext::Reflect()
        template<>
        bool LuaMaterialFunctorRuntimeContext::SetShaderOptionValue(const char* name, const char* value);

        void LuaMaterialFunctorRuntimeContext::Reflect(BehaviorContext* behaviorContext)
        {
            behaviorContext->Class<LuaMaterialFunctorRuntimeContext>()
                ->Method("GetMaterialPropertyValue_bool", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<bool>)
                ->Method("GetMaterialPropertyValue_int", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<int32_t>)
                ->Method("GetMaterialPropertyValue_uint", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<uint32_t>)
                ->Method("GetMaterialPropertyValue_enum", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<uint32_t>)
                ->Method("GetMaterialPropertyValue_float", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<float>)
                ->Method("GetMaterialPropertyValue_Vector2", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<Vector2>)
                ->Method("GetMaterialPropertyValue_Vector3", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<Vector3>)
                ->Method("GetMaterialPropertyValue_Vector4", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<Vector4>)
                ->Method("GetMaterialPropertyValue_Color", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<Color>)
                ->Method("GetMaterialPropertyValue_Image", &LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue<Image*>)
                ->Method("HasMaterialProperty", &LuaMaterialFunctorRuntimeContext::HasMaterialValue)
                ->Method("SetShaderConstant_bool", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<bool>)
                ->Method("SetShaderConstant_int", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<int32_t>)
                ->Method("SetShaderConstant_uint", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<uint32_t>)
                ->Method("SetShaderConstant_float", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<float>)
                ->Method("SetShaderConstant_Vector2", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<Vector2>)
                ->Method("SetShaderConstant_Vector3", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<Vector3>)
                ->Method("SetShaderConstant_Vector4", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<Vector4>)
                ->Method("SetShaderConstant_Color", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<Color>)
                ->Method("SetShaderConstant_Matrix3x3", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<Matrix3x3>)
                ->Method("SetShaderConstant_Matrix4x4", &LuaMaterialFunctorRuntimeContext::SetShaderConstant<Matrix4x4>)
                ->Method("SetShaderOptionValue_bool", &LuaMaterialFunctorRuntimeContext::SetShaderOptionValue<bool>)
                ->Method("SetShaderOptionValue_uint", &LuaMaterialFunctorRuntimeContext::SetShaderOptionValue<uint32_t>)
                ->Method("SetShaderOptionValue_enum", &LuaMaterialFunctorRuntimeContext::SetShaderOptionValue<const char*>)
                ->Method("GetShaderCount", &LuaMaterialFunctorRuntimeContext::GetShaderCount)
                ->Method("GetShader", &LuaMaterialFunctorRuntimeContext::GetShader)
                ->Method("GetShaderByTag", &LuaMaterialFunctorRuntimeContext::GetShaderByTag)
                ->Method("HasShaderWithTag", &LuaMaterialFunctorRuntimeContext::HasShaderWithTag)
                ;
        }

        LuaMaterialFunctorRuntimeContext::LuaMaterialFunctorRuntimeContext(MaterialFunctor::RuntimeContext* runtimeContextImpl,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialNameContext& materialNameContext)
            : LuaMaterialFunctorCommonContext(runtimeContextImpl, materialPropertyDependencies, materialNameContext)
            , m_runtimeContextImpl(runtimeContextImpl)
        {
        }

        template<typename Type>
        Type LuaMaterialFunctorRuntimeContext::GetMaterialPropertyValue(const char* name) const
        {
            return LuaMaterialFunctorCommonContext::GetMaterialPropertyValue<Type>(name);
        }

        bool LuaMaterialFunctorRuntimeContext::HasMaterialValue(const char* name) const
        {
            Name propertyFullName{name};
            m_materialNameContext.ContextualizeProperty(propertyFullName);
            
            MaterialPropertyIndex propertyIndex = GetMaterialPropertiesLayout()->FindPropertyIndex(propertyFullName);
            return propertyIndex.IsValid();
        }

        bool LuaMaterialFunctorRuntimeContext::SetShaderOptionValueHelper(const char* name, AZStd::function<bool(ShaderOptionGroup*, ShaderOptionIndex)> setValueCommand)
        {
            bool didSetOne = false;

            Name fullOptionName{name};
            m_materialNameContext.ContextualizeShaderOption(fullOptionName);

            for (AZStd::size_t i = 0; i < m_runtimeContextImpl->m_shaderCollection->size(); ++i)
            {
                ShaderCollection::Item& shaderItem = (*m_runtimeContextImpl->m_shaderCollection)[i];
                ShaderOptionGroup* shaderOptionGroup = shaderItem.GetShaderOptions();
                const ShaderOptionGroupLayout* layout = shaderOptionGroup->GetShaderOptionLayout();

                ShaderOptionIndex optionIndex = layout->FindShaderOptionIndex(fullOptionName);
                if (!optionIndex.IsValid())
                {
                    continue;
                }

                if (!shaderItem.MaterialOwnsShaderOption(optionIndex))
                {
                    LuaScriptUtilities::Error(AZStd::string::format("Shader option '%s' is not owned by this material.", fullOptionName.GetCStr()));
                    break;
                }

                if (setValueCommand(shaderOptionGroup, optionIndex))
                {
                    didSetOne = true;
                }
            }

            return didSetOne;
        }

        template<>
        bool LuaMaterialFunctorRuntimeContext::SetShaderOptionValue(const char* name, const char* value)
        {
            return SetShaderOptionValueHelper(name, [value](ShaderOptionGroup* optionGroup, ShaderOptionIndex optionIndex)
                {
                    return optionGroup->SetValue(optionIndex, Name{value});
                });
        }

        template<typename Type>
        bool LuaMaterialFunctorRuntimeContext::SetShaderOptionValue(const char* name, Type value)
        {
            return SetShaderOptionValueHelper(name, [value](ShaderOptionGroup* optionGroup, ShaderOptionIndex optionIndex)
                {
                    return optionGroup->SetValue(optionIndex, ShaderOptionValue{value});
                });
        }


        RHI::ShaderInputConstantIndex LuaMaterialFunctorRuntimeContext::GetShaderInputConstantIndex(const char* name, const char* functionName) const
        {
            Name fullInputName{name};
            m_materialNameContext.ContextualizeSrgInput(fullInputName);

            RHI::ShaderInputConstantIndex index = m_runtimeContextImpl->m_shaderResourceGroup->FindShaderInputConstantIndex(fullInputName);

            if (!index.IsValid())
            {
                LuaScriptUtilities::Error(AZStd::string::format("%s() could not find shader input '%s'", functionName, fullInputName.GetCStr()));
            }

            return index;
        }

        template<typename Type>
        bool LuaMaterialFunctorRuntimeContext::SetShaderConstant(const char* name, Type value)
        {
            RHI::ShaderInputConstantIndex index = GetShaderInputConstantIndex(name, "SetShaderConstant");
            if (index.IsValid())
            {
                return m_runtimeContextImpl->m_shaderResourceGroup->SetConstant(index, value);
            }

            return false;
        }

        AZStd::size_t LuaMaterialFunctorRuntimeContext::GetShaderCount() const
        {
            return m_runtimeContextImpl->GetShaderCount();
        }

        LuaMaterialFunctorShaderItem LuaMaterialFunctorRuntimeContext::GetShader(AZStd::size_t index)
        {
            if (index < GetShaderCount())
            {
                return LuaMaterialFunctorShaderItem{this, &(*m_runtimeContextImpl->m_shaderCollection)[index]};
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format("GetShader(%zu) is invalid.", index));
                return {};
            }
        }

        LuaMaterialFunctorShaderItem LuaMaterialFunctorRuntimeContext::GetShaderByTag(const char* shaderTag)
        {
            const AZ::Name tag{shaderTag};
            if (m_runtimeContextImpl->m_shaderCollection->HasShaderTag(tag))
            {
                return LuaMaterialFunctorShaderItem{this, &(*m_runtimeContextImpl->m_shaderCollection)[tag]};
            }
            else
            {
                LuaScriptUtilities::Error(AZStd::string::format(
                    "GetShaderByTag('%s') is invalid: Could not find a shader with the tag '%s'.", tag.GetCStr(), tag.GetCStr()));
                return {};
            }
        }
        
        bool LuaMaterialFunctorRuntimeContext::HasShaderWithTag(const char* shaderTag)
        {
            return m_runtimeContextImpl->m_shaderCollection->HasShaderTag(AZ::Name{shaderTag});
        }

        void LuaMaterialFunctorEditorContext::LuaMaterialFunctorEditorContext::Reflect(BehaviorContext* behaviorContext)
        {
            behaviorContext->Class<LuaMaterialFunctorEditorContext>()
                ->Method("GetMaterialPropertyValue_bool", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<bool>)
                ->Method("GetMaterialPropertyValue_int", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<int32_t>)
                ->Method("GetMaterialPropertyValue_uint", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<uint32_t>)
                ->Method("GetMaterialPropertyValue_enum", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<uint32_t>)
                ->Method("GetMaterialPropertyValue_float", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<float>)
                ->Method("GetMaterialPropertyValue_Vector2", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<Vector2>)
                ->Method("GetMaterialPropertyValue_Vector3", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<Vector3>)
                ->Method("GetMaterialPropertyValue_Vector4", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<Vector4>)
                ->Method("GetMaterialPropertyValue_Color", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<Color>)
                ->Method("GetMaterialPropertyValue_Image", &LuaMaterialFunctorEditorContext::GetMaterialPropertyValue<Image*>)
                ->Method("SetMaterialPropertyVisibility", &LuaMaterialFunctorEditorContext::SetMaterialPropertyVisibility)
                ->Method("SetMaterialPropertyDescription", &LuaMaterialFunctorEditorContext::SetMaterialPropertyDescription)
                ->Method("SetMaterialPropertyMinValue_int", &LuaMaterialFunctorEditorContext::SetMaterialPropertyMinValue<int32_t>)
                ->Method("SetMaterialPropertyMinValue_uint", &LuaMaterialFunctorEditorContext::SetMaterialPropertyMinValue<uint32_t>)
                ->Method("SetMaterialPropertyMinValue_float", &LuaMaterialFunctorEditorContext::SetMaterialPropertyMinValue<float>)
                ->Method("SetMaterialPropertyMaxValue_int", &LuaMaterialFunctorEditorContext::SetMaterialPropertyMaxValue<int32_t>)
                ->Method("SetMaterialPropertyMaxValue_uint", &LuaMaterialFunctorEditorContext::SetMaterialPropertyMaxValue<uint32_t>)
                ->Method("SetMaterialPropertyMaxValue_float", &LuaMaterialFunctorEditorContext::SetMaterialPropertyMaxValue<float>)
                ->Method("SetMaterialPropertySoftMinValue_int", &LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMinValue<int32_t>)
                ->Method("SetMaterialPropertySoftMinValue_uint", &LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMinValue<uint32_t>)
                ->Method("SetMaterialPropertySoftMinValue_float", &LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMinValue<float>)
                ->Method("SetMaterialPropertySoftMaxValue_int", &LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMaxValue<int32_t>)
                ->Method("SetMaterialPropertySoftMaxValue_uint", &LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMaxValue<uint32_t>)
                ->Method("SetMaterialPropertySoftMaxValue_float", &LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMaxValue<float>)
                ->Method("SetMaterialPropertyGroupVisibility", &LuaMaterialFunctorEditorContext::SetMaterialPropertyGroupVisibility)
                ;
        }

        LuaMaterialFunctorEditorContext::LuaMaterialFunctorEditorContext(MaterialFunctor::EditorContext* editorContextImpl,
            const MaterialPropertyFlags* materialPropertyDependencies,
            const MaterialNameContext& materialNameContext)
            : LuaMaterialFunctorCommonContext(editorContextImpl, materialPropertyDependencies, materialNameContext)
            , m_editorContextImpl(editorContextImpl)
        {
        }

        template<typename Type>
        Type LuaMaterialFunctorEditorContext::GetMaterialPropertyValue(const char* name) const
        {
            return LuaMaterialFunctorCommonContext::GetMaterialPropertyValue<Type>(name);
        }

        template<typename Type>
        bool LuaMaterialFunctorEditorContext::SetMaterialPropertyMinValue(const char* name, Type value)
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
        bool LuaMaterialFunctorEditorContext::SetMaterialPropertyMaxValue(const char* name, Type value)
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
        bool LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMinValue(const char* name, Type value)
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
        bool LuaMaterialFunctorEditorContext::SetMaterialPropertySoftMaxValue(const char* name, Type value)
        {
            const char* functionName = "SetMaterialPropertySoftMaxValue";

            MaterialPropertyIndex index = GetMaterialPropertyIndex(name, functionName);
            if (!index.IsValid())
            {
                return false;
            }

            return m_editorContextImpl->SetMaterialPropertySoftMaxValue(index, value);
        }
        
        bool LuaMaterialFunctorEditorContext::SetMaterialPropertyGroupVisibility(const char* name, MaterialPropertyGroupVisibility visibility)
        {
            if (m_editorContextImpl)
            {
                Name fullName{name};
                m_materialNameContext.ContextualizeProperty(fullName);
                return m_editorContextImpl->SetMaterialPropertyGroupVisibility(fullName, visibility);
            }
            return false;
        }

        bool LuaMaterialFunctorEditorContext::SetMaterialPropertyVisibility(const char* name, MaterialPropertyVisibility visibility)
        {
            if (m_editorContextImpl)
            {
                Name fullName{name};
                m_materialNameContext.ContextualizeProperty(fullName);
                return m_editorContextImpl->SetMaterialPropertyVisibility(fullName, visibility);
            }
            return false;
        }

        bool LuaMaterialFunctorEditorContext::SetMaterialPropertyDescription(const char* name, const char* description)
        {
            if (m_editorContextImpl)
            {
                Name fullName{name};
                m_materialNameContext.ContextualizeProperty(fullName);
                return m_editorContextImpl->SetMaterialPropertyDescription(fullName, description);
            }
            return false;
        }

        template<>
        void LuaMaterialFunctorShaderItem::SetShaderOptionValue(const char* name, const char* value);

        void LuaMaterialFunctorShaderItem::Reflect(AZ::BehaviorContext* behaviorContext)
        {
            behaviorContext->Class<LuaMaterialFunctorShaderItem>()
                ->Method("GetRenderStatesOverride", &LuaMaterialFunctorShaderItem::GetRenderStatesOverride)
                ->Method("SetEnabled", &LuaMaterialFunctorShaderItem::SetEnabled)
                ->Method("SetDrawListTagOverride", &LuaMaterialFunctorShaderItem::SetDrawListTagOverride)
                ->Method("SetShaderOptionValue_bool", &LuaMaterialFunctorShaderItem::SetShaderOptionValue<bool>)
                ->Method("SetShaderOptionValue_uint", &LuaMaterialFunctorShaderItem::SetShaderOptionValue<uint32_t>)
                ->Method("SetShaderOptionValue_enum", &LuaMaterialFunctorShaderItem::SetShaderOptionValue<const char*>)
                ;
        }

        LuaMaterialFunctorRenderStates LuaMaterialFunctorShaderItem::GetRenderStatesOverride()
        {
            if (m_context && m_context->CheckPsoChangesAllowed() && m_shaderItem)
            {
                return LuaMaterialFunctorRenderStates{m_shaderItem->GetRenderStatesOverlay()};
            }
            else
            {
                static RHI::RenderStates dummyRenderStates;
                return LuaMaterialFunctorRenderStates{&dummyRenderStates};
            }
        }

        void LuaMaterialFunctorShaderItem::SetEnabled(bool enable)
        {
            if (m_shaderItem)
            {
                m_shaderItem->SetEnabled(enable);
            }
        }

        void LuaMaterialFunctorShaderItem::SetDrawListTagOverride(const char* drawListTag)
        {
            if (m_shaderItem)
            {
                m_shaderItem->SetDrawListTagOverride(Name{drawListTag});
            }
        }

        void LuaMaterialFunctorShaderItem::SetShaderOptionValue(
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
        void LuaMaterialFunctorShaderItem::SetShaderOptionValue(const char* name, const char* value)
        {
            if (m_shaderItem)
            {
                SetShaderOptionValue(Name{name}, [value](ShaderOptionGroup* optionGroup, ShaderOptionIndex optionIndex) {
                    return optionGroup->SetValue(optionIndex, Name{value});
                });
            }
        }

        template<typename Type>
        void LuaMaterialFunctorShaderItem::SetShaderOptionValue(const char* name, Type value)
        {
            if (m_shaderItem)
            {
                SetShaderOptionValue(Name{name}, [value](ShaderOptionGroup* optionGroup, ShaderOptionIndex optionIndex) {
                    return optionGroup->SetValue(optionIndex, ShaderOptionValue{value});
                });
            }
        }

        void LuaMaterialFunctorRenderStates::Reflect(AZ::BehaviorContext* behaviorContext)
        {
            RHI::ReflectRenderStateEnums(behaviorContext);

            auto classBuilder = behaviorContext->Class<LuaMaterialFunctorRenderStates>();

            #define TEMP_REFLECT_RENDERSTATE_METHODS(PropertyName) \
                classBuilder->Method("Set" AZ_STRINGIZE(PropertyName), AZ_JOIN(&LuaMaterialFunctorRenderStates::Set, PropertyName)); \
                classBuilder->Method("Clear" AZ_STRINGIZE(PropertyName), AZ_JOIN(&LuaMaterialFunctorRenderStates::Clear, PropertyName));

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
            void AZ_JOIN(LuaMaterialFunctorRenderStates::Set, PropertyName)(DataType value)         \
            {                                                                                       \
                Field = value;                                                                      \
            }                                                                                       \
            void AZ_JOIN(LuaMaterialFunctorRenderStates::Clear, PropertyName)()                     \
            {                                                                                       \
                Field = InvalidValue;                                                               \
            }

        #define TEMP_DEFINE_RENDERSTATE_METHODS_BLENDSTATETARGET(PropertyName, DataType, Field, InvalidValue)           \
            void AZ_JOIN(LuaMaterialFunctorRenderStates::Set, PropertyName)(AZStd::size_t targetIndex, DataType value)  \
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
            void AZ_JOIN(LuaMaterialFunctorRenderStates::Clear, PropertyName)(AZStd::size_t targetIndex)                \
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

        void LuaMaterialFunctorRenderStates::SetMultisampleCustomPosition(AZStd::size_t multisampleCustomLocationIndex, uint8_t x, uint8_t y)
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

        void LuaMaterialFunctorRenderStates::ClearMultisampleCustomPosition(AZStd::size_t multisampleCustomLocationIndex)
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

        void LuaMaterialFunctorRenderStates::SetMultisampleCustomPositionCount(uint32_t value)
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

        void LuaMaterialFunctorRenderStates::ClearMultisampleCustomPositionCount()
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
