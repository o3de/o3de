/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ProductInfoCreator.h"

#include <CryPath.h>

#include <QFileInfo>

namespace AZ
{
    namespace RC
    {
        ProductInfoCreator::ProductInfoCreator()
        {
            m_productAssetType = AZ::Data::AssetType::CreateNull();
        }

        AssetBuilderSDK::JobProduct ProductInfoCreator::GenerateProductInfo(const AZStd::string& sourceFile, const AZStd::string& fullPath)
        {
            AZStd::unique_ptr<AssetParser> legacyAssetParser = CreateLegacyAssetParser(sourceFile);
            AZStd::string fileName = QFileInfo(sourceFile.c_str()).fileName().toUtf8().constData();

            AssetBuilderSDK::JobProduct product(fileName, m_productAssetType);
            product.m_pathDependencies = legacyAssetParser->GetProductDependencies(fullPath);
            product.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies

            return product;

        }

        AZStd::unique_ptr<AssetParser> ProductInfoCreator::CreateLegacyAssetParser(const AZStd::string& sourceFile)
        {
            AZStd::string fileExtension(PathUtil::GetExt(sourceFile.c_str()));

            // Use the file extension to select the correct asset parser.

            /*Sample code:
            if (fileExtension == "font" || fileExtension == "fontfamily")
            {
                m_productAssetType = fontAssetType;
                return AZStd::unique_ptr<AssetParser>(new FontAssetParser(sourceFile));
            }*/
            
            return AZStd::unique_ptr<AssetParser>(new AssetParser(sourceFile));
        }
    } // namespace RC
} // namespace AZ
