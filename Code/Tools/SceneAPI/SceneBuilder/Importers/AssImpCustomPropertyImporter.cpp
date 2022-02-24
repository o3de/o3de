/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SceneAPI/SceneBuilder/Importers/AssImpCustomPropertyImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpImporterUtilities.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/CustomPropertyData.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <assimp/scene.h>

namespace AZ::SceneAPI::SceneBuilder
{
    const char* s_customPropertiesNodeName = "custom_properties";

    AssImpCustomPropertyImporter::AssImpCustomPropertyImporter()
    {
        BindToCall(&AssImpCustomPropertyImporter::ImportCustomProperty);
    }

    void AssImpCustomPropertyImporter::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AssImpCustomPropertyImporter, SceneCore::LoadingComponent>()->Version(1);
        }
    }

    Events::ProcessingResult AssImpCustomPropertyImporter::ImportCustomProperty(AssImpSceneNodeAppendedContext& context)
    {
        AZ_TraceContext("Importer", "customProperty");
        const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();

        if (!currentNode->mMetaData || currentNode->mMetaData->mNumProperties == 0)
        {
            return Events::ProcessingResult::Ignored;
        }

        AZ::SceneData::GraphData::CustomPropertyData::PropertyMap propertyMap;

        for (size_t index = 0; index < currentNode->mMetaData->mNumProperties; ++index)
        {
            const aiString* key;
            const aiMetadataEntry* data;
            currentNode->mMetaData->Get(index, key, data);

            AZStd::string_view currentNodeName(currentNode->mName.C_Str(), currentNode->mName.length);
            AZStd::string_view keyName(key->C_Str(), key->length);

            if (data->mType == AI_AISTRING)
            {
                const aiString* value = reinterpret_cast<const aiString*>(data->mData);
                propertyMap[keyName] = AZStd::make_any<AZStd::string>(value->C_Str(), value->length);
            }
            else if (data->mType == AI_BOOL)
            {
                const bool* value = reinterpret_cast<const bool*>(data->mData);
                propertyMap[keyName] = AZStd::make_any<bool>(*value);
            }
            else if (data->mType == AI_INT32)
            {
                const int32_t* value = reinterpret_cast<const int32_t*>(data->mData);
                propertyMap[keyName] = AZStd::make_any<int32_t>(*value);
            }
            else if (data->mType == AI_UINT64)
            {
                const uint64_t* value = reinterpret_cast<const uint64_t*>(data->mData);
                propertyMap[keyName] = AZStd::make_any<uint64_t>(*value);
            }
            else if (data->mType == AI_FLOAT)
            {
                const float* value = reinterpret_cast<const float*>(data->mData);
                propertyMap[keyName] = AZStd::make_any<float>(*value);
            }
            else if (data->mType == AI_DOUBLE)
            {
                const double* value = reinterpret_cast<const double*>(data->mData);
                propertyMap[keyName] = AZStd::make_any<double>(*value);
            }
        }

        auto customPropertyMapData = AZStd::make_shared<SceneData::GraphData::CustomPropertyData>(propertyMap);
        if (!customPropertyMapData)
        {
            return Events::ProcessingResult::Failure;
        }

        // If it is non-endpoint data populated node, add a custom property node attribute
        if (context.m_scene.GetGraph().HasNodeContent(context.m_currentGraphPosition))
        {
            if (!context.m_scene.GetGraph().IsNodeEndPoint(context.m_currentGraphPosition))
            {
                AZStd::string nodeName{ s_customPropertiesNodeName };
                RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition);

                auto newIndex = context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());
                AZ_Error(SceneAPI::Utilities::ErrorWindow, newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                if (!newIndex.IsValid())
                {
                    return Events::ProcessingResult::Failure;
                }

                Events::ProcessingResult attributeResult;
                AssImpSceneAttributeDataPopulatedContext dataPopulated(context, customPropertyMapData, newIndex, nodeName);
                attributeResult = Events::Process(dataPopulated);

                if (attributeResult != Events::ProcessingResult::Failure)
                {
                    attributeResult = AddAttributeDataNodeWithContexts(dataPopulated);
                }

                return attributeResult;
            }
        }
        else
        {
            bool addedData = context.m_scene.GetGraph().SetContent(context.m_currentGraphPosition, customPropertyMapData);
            AZ_Error(SceneAPI::Utilities::ErrorWindow, addedData, "Failed to add node data");
            return addedData ? Events::ProcessingResult::Success : Events::ProcessingResult::Failure;
        }

        return Events::ProcessingResult::Ignored;
    }
}
