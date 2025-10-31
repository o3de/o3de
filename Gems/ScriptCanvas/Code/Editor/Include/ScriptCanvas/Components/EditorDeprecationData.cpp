/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Components/EditorDeprecationData.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <Editor/Framework/Configuration.h>

namespace ScriptCanvasEditor
{
    namespace Deprecated
    {
        bool EditorScriptCanvasComponentVersionConverter::Convert(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
        {
            using namespace ScriptCanvasEditor;

            if (rootElement.GetVersion() <= 4)
            {
                int assetElementIndex = rootElement.FindElement(AZ::Crc32("m_asset"));
                if (assetElementIndex == -1)
                {
                    return false;
                }

                auto assetElement = rootElement.GetSubElement(assetElementIndex);
                AZ::Data::Asset<Deprecated::ScriptCanvasAsset> scriptCanvasAsset;
                if (!assetElement.GetData(scriptCanvasAsset))
                {
                    AZ_Error("Script Canvas", false, "Unable to find Script Canvas Asset on a Version %u Editor ScriptCanvas Component", rootElement.GetVersion());
                    return false;
                }

                Deprecated::ScriptCanvasAssetHolder assetHolder;
                assetHolder.m_scriptCanvasAsset = scriptCanvasAsset;

                if (-1 == rootElement.AddElementWithData(serializeContext, "m_assetHolder", assetHolder))
                {
                    AZ_Error("Script Canvas", false, "Unable to add ScriptCanvas Asset Holder element when converting from version %u", rootElement.GetVersion())
                }

                rootElement.RemoveElementByName(AZ_CRC_CE("m_asset"));
                rootElement.RemoveElementByName(AZ_CRC_CE("m_openEditorButton"));
            }

            if (rootElement.GetVersion() <= 6)
            {
                rootElement.RemoveElementByName(AZ_CRC_CE("m_originalData"));
            }

            if (rootElement.GetVersion() <= 7)
            {
                rootElement.RemoveElementByName(AZ_CRC_CE("m_variableEntityIdMap"));
            }

            ScriptCanvasBuilder::BuildVariableOverrides overrides;
            AZ::Uuid sourceId;
            AZStd::string path;
            

            if (rootElement.GetVersion() <= EditorScriptCanvasComponent::Version::PrefabIntegration)
            {
                auto variableDataElementIndex = rootElement.FindElement(AZ_CRC_CE("m_variableData"));
                if (variableDataElementIndex == -1)
                {
                    AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: 'm_variableData' index was missing");
                    return false;
                }

                auto& variableDataElement = rootElement.GetSubElement(variableDataElementIndex);

                ScriptCanvas::EditableVariableData editableData;
                if (!variableDataElement.GetData(editableData))
                {
                    AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_variableData'");
                    return false;
                }

                auto scriptCanvasAssetHolderElementIndex = rootElement.FindElement(AZ_CRC_CE("m_assetHolder"));
                if (scriptCanvasAssetHolderElementIndex == -1)
                {
                    AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: 'm_assetHolder' index was missing");
                    return false;
                }

                auto& scriptCanvasAssetHolderElement = rootElement.GetSubElement(scriptCanvasAssetHolderElementIndex);

                Deprecated::ScriptCanvasAssetHolder assetHolder;
                if (!scriptCanvasAssetHolderElement.GetData(assetHolder))
                {
                    AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_assetHolder'");
                    return false;
                }

                rootElement.RemoveElement(variableDataElementIndex);

                overrides.m_source = SourceHandle(nullptr, assetHolder.m_scriptCanvasAsset.GetId().m_guid);

                for (auto& variable : editableData.GetVariables())
                {
                    overrides.m_overrides.push_back(variable.m_graphVariable);
                }
            }

            auto scriptCanvasAssetHolderElementIndex = rootElement.FindElement(AZ_CRC_CE("m_assetHolder"));
            if (scriptCanvasAssetHolderElementIndex != -1)
            {
                auto& scriptCanvasAssetHolderElement = rootElement.GetSubElement(scriptCanvasAssetHolderElementIndex);
                Deprecated::ScriptCanvasAssetHolder assetHolder;
                if (!scriptCanvasAssetHolderElement.GetData(assetHolder))
                {
                    AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_assetHolder'");
                    return false;
                }

                sourceId = assetHolder.m_scriptCanvasAsset.GetId().m_guid;
                path = assetHolder.m_scriptCanvasAsset.GetHint();
            }

            // all object stream reads must convert add all new data for JSON reads...
            const SourceHandle fromAssetHolder = SourceHandle::FromRelativePath(nullptr, sourceId, path);
            const SourceHandle fromOverrides = overrides.m_source;

            Configuration configuration;
            configuration.m_propertyOverrides = overrides;

            if (fromAssetHolder == fromOverrides || (fromAssetHolder.IsDescriptionValid() && !fromOverrides.IsDescriptionValid()))
            {
                configuration.m_sourceHandle = fromAssetHolder;
            }
            else
            {
                configuration.m_sourceHandle = fromOverrides;
            }

            if (configuration.m_sourceHandle.RelativePath().HasFilename())
            {
                configuration.m_sourceName = configuration.m_sourceHandle.RelativePath().Native();
            }

            if (-1 == rootElement.AddElementWithData(serializeContext, "configuration", overrides))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: failed to add 'configuration'");
                return false;
            }

            return true;
        }

        void ScriptCanvasAssetHolder::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptCanvasAssetHolder>()
                    ->Version(1)
                    ->Field("m_asset", &ScriptCanvasAssetHolder::m_scriptCanvasAsset)
                    ;
            }
        }
    }
}
