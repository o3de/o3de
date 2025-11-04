/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>

namespace ScriptCanvas
{
    enum class FunctionRuntimeDataVersion
    {
        MergeBackEnd2dotZero,
        AddSubgraphInterface,
        RemoveLegacyData,
        RemoveConnectionToRuntimeData,

        // add description above
        Current
    };

    SubgraphInterfaceAssetDescription::SubgraphInterfaceAssetDescription()
        : AssetDescription(
              azrtti_typeid<SubgraphInterfaceAsset>(),
              "Script Canvas Function Interface",
              "Script Canvas Function Interface",
              "@projectroot@/scriptcanvas",
              ".scriptcanvas_fn_compiled",
              "Script Canvas Function Interface",
              "Untitled-Function-%i",
              "Script Canvas Compiled Function Interfaces (*.scriptcanvas_fn_compiled)",
              "Script Canvas Function Interface",
              "Script Canvas Function Interface",
              "Icons/ScriptCanvas/Viewport/ScriptCanvas_Function.png",
              AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
              false)
    {
    }

    void SubgraphInterfaceData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<SubgraphInterfaceData>()
                ->Version(static_cast<int>(FunctionRuntimeDataVersion::Current))
                ->Field("name", &SubgraphInterfaceData::m_name)
                ->Field("interface", &SubgraphInterfaceData::m_interface);
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
} // namespace ScriptCanvas
