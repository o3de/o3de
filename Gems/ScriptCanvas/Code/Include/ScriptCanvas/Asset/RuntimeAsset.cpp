/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RuntimeAsset.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace ScriptCanvasRuntimeAssetCpp
{
    enum class RuntimeDataVersion
    {
        AddDependencies = 3,
        ChangeScriptRequirementToAsset,

        // add description above
        Current
    };

    enum class RuntimeDataOverridesVersion : unsigned int
    {
        Initial = 0,
        AddRuntimeAsset,

        // add description above
        Current,
    };

    enum class FunctionRuntimeDataVersion
    {
        MergeBackEnd2dotZero,
        AddSubgraphInterface,
        RemoveLegacyData,
        RemoveConnectionToRuntimeData,

        // add description above
        Current
    };
}

namespace ScriptCanvas
{
    RuntimeData::RuntimeData(RuntimeData&& other)
    {
        *this = AZStd::move(other);
    }

    RuntimeData& RuntimeData::operator=(RuntimeData&& other)
    {
        if (this != &other)
        {
            m_input = AZStd::move(other.m_input);
            m_debugMap = AZStd::move(other.m_debugMap);
            m_script = AZStd::move(other.m_script);
            m_requiredAssets = AZStd::move(other.m_requiredAssets);
            m_requiredScriptEvents = AZStd::move(other.m_requiredScriptEvents);
            m_areStaticsInitialized = AZStd::move(other.m_areStaticsInitialized);
        }

        return *this;
    }

    void RuntimeData::Reflect(AZ::ReflectContext* reflectContext)
    {
        Translation::RuntimeInputs::Reflect(reflectContext);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<RuntimeData>()
                ->Version(static_cast<int>(ScriptCanvasRuntimeAssetCpp::RuntimeDataVersion::Current))
                ->Field("input", &RuntimeData::m_input)
                ->Field("debugMap", &RuntimeData::m_debugMap)
                ->Field("script", &RuntimeData::m_script)
                ->Field("requiredAssets", &RuntimeData::m_requiredAssets)
                ->Field("requiredScriptEvents", &RuntimeData::m_requiredScriptEvents)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<RuntimeData>()
                ->Method("GetRequiredAssets", [](const RuntimeData& data) { return data.m_requiredAssets; })
                ;
        }
    }

    bool RuntimeData::RequiresDependencyConstructionParameters() const
    {
        return AZStd::any_of(m_requiredAssets.begin(), m_requiredAssets.end(), [](const AZ::Data::Asset<RuntimeAsset>& asset)
        {
            return RequiresDependencyConstructionParametersRecurse(asset.Get()->m_runtimeData);
        });
    }

    bool RuntimeData::RequiresDependencyConstructionParametersRecurse(const RuntimeData& data)
    {
        return data.m_input.GetConstructorParameterCount() != 0
            || AZStd::any_of(data.m_requiredAssets.begin(), data.m_requiredAssets.end(), [](const AZ::Data::Asset<RuntimeAsset>& asset)
        {
            return RequiresDependencyConstructionParametersRecurse(asset.Get()->m_runtimeData);
        });
    }

    bool RuntimeData::RequiresStaticInitialization() const
    {
        return !m_cloneSources.empty();
    }

    bool RuntimeDataOverrides::IsPreloadBehaviorEnforced(const RuntimeDataOverrides& overrides)
    {
        if (overrides.m_runtimeAsset.GetAutoLoadBehavior() != AZ::Data::AssetLoadBehavior::PreLoad)
        {
            return false;
        }

        for (auto& dependency : overrides.m_dependencies)
        {
            if (!IsPreloadBehaviorEnforced(dependency))
            {
                return false;
            }
        }

        return true;
    }

    void RuntimeDataOverrides::EnforcePreloadBehavior()
    {
        m_runtimeAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);

        for (auto& dependency : m_dependencies)
        {
            dependency.EnforcePreloadBehavior();
        }
    }

    void RuntimeDataOverrides::Reflect(AZ::ReflectContext* context)
    {
        RuntimeVariable::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeDataOverrides>()
                ->Version(static_cast<unsigned int>(ScriptCanvasRuntimeAssetCpp::RuntimeDataOverridesVersion::Current))
                ->Field("runtimeAsset", &RuntimeDataOverrides::m_runtimeAsset)
                ->Field("variables", &RuntimeDataOverrides::m_variables)
                ->Field("variableIndices", &RuntimeDataOverrides::m_variableIndices)
                ->Field("entityIds", &RuntimeDataOverrides::m_entityIds)
                ->Field("dependencies", &RuntimeDataOverrides::m_dependencies)
                ;
        }
    }

    RuntimeVariable::RuntimeVariable(const AZStd::any& source)
        : value(source)
    {
    }

    RuntimeVariable::RuntimeVariable(AZStd::any&& source)
        : value(AZStd::move(source))
    {
    }

    void RuntimeVariable::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeVariable>()
                ->Field("value", &RuntimeVariable::value)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RuntimeVariable>("RuntimeVariable", "RuntimeVariable")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RuntimeVariable::value, "value", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ;
            }
        }
    }

    ////////////////////////
    // SubgraphInterfaceData
    ////////////////////////

    void SubgraphInterfaceData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<SubgraphInterfaceData>()
                ->Version(static_cast<int>(ScriptCanvasRuntimeAssetCpp::FunctionRuntimeDataVersion::Current))
                ->Field("name", &SubgraphInterfaceData::m_name)
                ->Field("interface", &SubgraphInterfaceData::m_interface)
                ;
        }
    }

    SubgraphInterfaceData::SubgraphInterfaceData(SubgraphInterfaceData&& other)
    {
        *this = AZStd::move(other);
    }

    SubgraphInterfaceData& SubgraphInterfaceData::operator=(SubgraphInterfaceData&& other)
    {
        if (this != &other)
        {
            m_name = AZStd::move(other.m_name);
            m_interface = AZStd::move(other.m_interface);
        }

        return *this;
    }
}
