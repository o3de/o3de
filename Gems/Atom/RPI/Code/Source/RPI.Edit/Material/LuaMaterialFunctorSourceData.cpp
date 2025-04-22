/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/LuaMaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {
        void LuaMaterialFunctorSourceData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<LuaMaterialFunctorSourceData>()
                    ->Version(3)
                    ->Field("file", &LuaMaterialFunctorSourceData::m_luaSourceFile)
                    ->Field("propertyNamePrefix", &LuaMaterialFunctorSourceData::m_propertyNamePrefix)
                    ->Field("srgNamePrefix", &LuaMaterialFunctorSourceData::m_srgNamePrefix)
                    ->Field("optionsNamePrefix", &LuaMaterialFunctorSourceData::m_optionsNamePrefix)
                    //[GFX TODO][ATOM-6011] Add support for inline script. Needs a custom "multiline string" json serializer.
                    //->Field("script", &LuaMaterialFunctorSourceData::m_luaScript)
                    ;
            }
        }

        AZStd::vector<LuaMaterialFunctorSourceData::AssetDependency> LuaMaterialFunctorSourceData::GetAssetDependencies() const
        {
            if (!m_luaSourceFile.empty())
            {
                AssetDependency dependency;
                dependency.m_jobKey = "Lua Compile";
                dependency.m_sourceFilePath = m_luaSourceFile;

                return AZStd::vector<AssetDependency>{dependency};
            }
            else
            {
                return {};
            }
        }

        Outcome<AZStd::vector<Name>, void> LuaMaterialFunctorSourceData::GetNameListFromLuaScript(AZ::ScriptContext& scriptContext, const char* luaFunctionName) const
        {
            AZStd::vector<Name> result;

            AZ::ScriptDataContext call;
            if (scriptContext.Call(luaFunctionName, call))
            {
                if (!call.CallExecute())
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Failed calling %s().", luaFunctionName);
                    return Failure();
                }

                if (1 != call.GetNumResults() || !call.IsTable(0))
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "%s() must return a table.", luaFunctionName);
                    return Failure();
                }

                AZ::ScriptDataContext table;
                if (!call.InspectTable(0, table))
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Failed to inspect table returned by %s().", luaFunctionName);
                    return Failure();
                }

                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                bool foundPropertyError = false;

                while (table.InspectNextElement(elementIndex, fieldName, fieldIndex))
                {
                    if (fieldIndex != -1)
                    {
                        if (!table.IsString(elementIndex))
                        {
                            AZ_Error("LuaMaterialFunctorSourceData", false, "%s() returned invalid table: element[%d] is not a string", luaFunctionName, fieldIndex);
                            foundPropertyError = true;
                            continue;
                        }

                        const char* materialPropertyName = nullptr;
                        if (!table.ReadValue(elementIndex, materialPropertyName))
                        {
                            AZ_Error("LuaMaterialFunctorSourceData", false, "%s() returned invalid table: element[%d] is invalid", luaFunctionName, fieldIndex);
                            foundPropertyError = true;
                            continue;
                        }

                        result.push_back(Name{materialPropertyName});
                    }
                }

                if (foundPropertyError)
                {
                    return Failure();
                }
            }

            return Success(result);
        }

        RPI::LuaMaterialFunctorSourceData::FunctorResult LuaMaterialFunctorSourceData::CreateFunctor(
                const AZStd::string& materialTypeSourceFilePath,
                const MaterialPropertiesLayout* propertiesLayout,
                const MaterialNameContext* materialNameContext
            ) const
        {
            using namespace RPI;

            RPI::Ptr<LuaMaterialFunctor> functor = aznew LuaMaterialFunctor;

            if (materialNameContext->IsDefault())
            {
                // This is a legacy feature that was used for a while to support reusing the same functor for multiple layers in StandardMultilayerPbr.materialtype.
                // Now that we have support for nested property groups, this functionality is only supported for functors at the top level, for backward compatibility.
                functor->m_materialNameContext.ExtendPropertyIdContext(m_propertyNamePrefix, false);
                functor->m_materialNameContext.ExtendSrgInputContext(m_srgNamePrefix);
                functor->m_materialNameContext.ExtendShaderOptionContext(m_optionsNamePrefix);
            }
            else
            {
                functor->m_materialNameContext = *materialNameContext;
            }

            if (!m_luaScript.empty() && !m_luaSourceFile.empty())
            {
                AZ_Error("LuaMaterialFunctor", m_luaSourceFile.empty(), "Lua material functor has both a built-in script and an external script file.");
                return Failure();
            }
            else if (!m_luaScript.empty())
            {
                functor->m_scriptBuffer.assign(m_luaScript.begin(), m_luaScript.end());
            }
            else if (!m_luaSourceFile.empty())
            {
                auto loadOutcome =
                    RPI::AssetUtils::LoadAsset<ScriptAsset>(materialTypeSourceFilePath, m_luaSourceFile, ScriptAsset::CompiledAssetSubId);
                if (!loadOutcome)
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Could not load script file '%s'", m_luaSourceFile.c_str());
                    return Failure();
                }

                functor->m_scriptAsset = loadOutcome.GetValue();
            }
            else
            {
                AZ_Error("LuaMaterialFunctor", false, "Lua material functor has no script data.");
                return Failure();
            }

            ScriptContext* scriptContext{};
            ScriptSystemRequestBus::BroadcastResult(scriptContext, &ScriptSystemRequests::GetContext, ScriptContextIds::DefaultScriptContextId);
            if (scriptContext == nullptr)
            {
                AZ_ErrorOnce("LuaMaterialFunctorSourceData", false, "Global script context is not available. Cannot execute script");
                return Failure();
            }

            auto scriptBuffer = functor->GetScriptBuffer();
            // Remove any GetMaterialPropertyDependencies and GetShaderOptionDependencies functions on the global table
            scriptContext->RemoveGlobal("GetMaterialPropertyDependencies");
            scriptContext->RemoveGlobal("GetShaderOptionDependencies");
            if (!scriptContext->Execute(scriptBuffer.data(), functor->GetScriptDescription(), scriptBuffer.size()))
            {
                AZ_Error("LuaMaterialFunctorSourceData", false, "Error initializing script '%s'.", functor->m_scriptAsset.ToString<AZStd::string>().c_str());
                return Failure();
            }

            // [GFX TODO][ATOM-6012]: Figure out how to make shader option dependencies and material property dependencies get automatically reported

            auto materialPropertyDependencies = GetNameListFromLuaScript(*scriptContext, "GetMaterialPropertyDependencies");
            auto shaderOptionDependencies = GetNameListFromLuaScript(*scriptContext, "GetShaderOptionDependencies");

            if (!materialPropertyDependencies.IsSuccess() || !shaderOptionDependencies.IsSuccess())
            {
                return Failure();
            }
            
            if (materialPropertyDependencies.GetValue().empty())
            {
                AZ_Error("LuaMaterialFunctorSourceData", false, "Material functor must use at least one material property.");
                return Failure();
            }

            m_shaderOptionDependencies = shaderOptionDependencies.GetValue();
            for (auto& shaderOption : m_shaderOptionDependencies)
            {
                shaderOption = Name{m_optionsNamePrefix + shaderOption.GetCStr()};
            }

            for (const Name& materialProperty : materialPropertyDependencies.GetValue())
            {
                Name propertyName{materialProperty};
                functor->m_materialNameContext.ContextualizeProperty(propertyName);

                MaterialPropertyIndex index = propertiesLayout->FindPropertyIndex(propertyName);
                if (index.IsValid())
                {
                    AddMaterialPropertyDependency(functor, index);
                }
                // This allows missing dependencies to make scripts more flexible - they can depend on properties that may
                // or may not exist, and it's up to the script to call HasMaterialProperty() before accessing a property
                // if necessary.
            }

            return Success(RPI::Ptr<MaterialFunctor>(functor));
        }

        RPI::LuaMaterialFunctorSourceData::FunctorResult LuaMaterialFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            return CreateFunctor(
                context.GetMaterialTypeSourceFilePath(),
                context.GetMaterialPropertiesLayout(),
                context.GetNameContext());
        }

        RPI::LuaMaterialFunctorSourceData::FunctorResult LuaMaterialFunctorSourceData::CreateFunctor(const EditorContext& context) const
        {
            return CreateFunctor(
                context.GetMaterialTypeSourceFilePath(),
                context.GetMaterialPropertiesLayout(),
                context.GetNameContext());
        }
    }
}
