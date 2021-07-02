/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <LmbrCentral_precompiled.h>
#include "XmlFormattedAssetBuilderWorker.h"

#include <AzFramework/IO/LocalFileIO.h>

namespace CopyDependencyBuilder
{
    XmlFormattedAssetBuilderWorker::XmlFormattedAssetBuilderWorker(AZStd::string jobKey, bool critical, bool skipServer)
        : CopyDependencyBuilderWorker(jobKey, critical, skipServer)
    {
    }

    bool XmlFormattedAssetBuilderWorker::ParseProductDependencies(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            return false;
        }

        AZ::IO::SizeType length = fileStream.GetLength();
        if (length == 0)
        {
            return false;
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length + 1);
        fileStream.Read(length, charBuffer.data());
        charBuffer.back() = 0;

        // Get the XML root node
        AZ::rapidxml::xml_document<char> xmlDoc;
        if (!xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(charBuffer.data()))
        {
            return false;
        }

        AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc.first_node();
        return xmlRootNode && ParseXmlFile(xmlRootNode, request.m_fullPath, request.m_sourceFile, request.m_platformInfo.m_identifier, productDependencies, pathDependencies);
    }

    bool XmlFormattedAssetBuilderWorker::ParseXmlFile(
        const AZ::rapidxml::xml_node<char>* node,
        const AZStd::string& fullPath,
        const AZStd::string& sourceFile,
        const AZStd::string& platformIdentifier,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies)
    {
        if (!node)
        {
            return false;
        }

        AddProductDependencies(node, fullPath, sourceFile, platformIdentifier, productDependencies, productPathDependencies);

        // Check all the child nodes
        for (AZ::rapidxml::xml_node<char>* childNode = node->first_node(); childNode; childNode = childNode->next_sibling())
        {
            if (!ParseXmlFile(childNode, fullPath, sourceFile, platformIdentifier, productDependencies, productPathDependencies))
            {
                return false;
            }
        }

        return true;
    }
}
