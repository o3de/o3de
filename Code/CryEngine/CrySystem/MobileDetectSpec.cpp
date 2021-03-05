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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySystem_precompiled.h"
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <IXml.h>

#include "MobileDetectSpec.h"

namespace MobileSysInspect
{
    struct GpuApiPair
    {
        AZStd::string gpuDescription;
        AZStd::string apiDescription;
    };

    AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> deviceSpecMapping;
    AZStd::vector<AZStd::pair<GpuApiPair, AZStd::string>> gpuSpecMapping;
    const float LOW_SPEC_RAM = 1.0f;
    const float MEDIUM_SPEC_RAM = 2.0f;
    const float HIGH_SPEC_RAM = 3.0f;

    bool GetSpecForGPUAndAPI(const AZStd::string& gpuName, const AZStd::string& apiDescription, AZStd::string& specName)
    {
        for (const auto& descriptionSpecPair : gpuSpecMapping)
        {
            const GpuApiPair& currentPair = descriptionSpecPair.first;
            AZStd::regex currentRegex(currentPair.gpuDescription.c_str());
            if (!AZStd::regex_search(gpuName, currentRegex))
            {
                continue;
            }

            currentRegex.assign(currentPair.apiDescription.c_str());
            if (!currentRegex.Empty() && !AZStd::regex_search(apiDescription, currentRegex))
            {
                continue;
            }

            specName = descriptionSpecPair.second;
            return true;
        }

        return false;
    }

    namespace Internal
    {
        void LoadDeviceSpecMapping_impl(const char* filename)
        {
            XmlNodeRef xmlNode = GetISystem()->LoadXmlFromFile(filename);

            if (!xmlNode)
            {
                return;
            }

            const int fileCount = xmlNode->getChildCount();

            for (int i = 0; i < fileCount; ++i)
            {
                XmlNodeRef fileNode = xmlNode->getChild(i);
                AZStd::string file = fileNode->getAttr("file");

                if (!file.empty())
                {
                    const int mappingCount = fileNode->getChildCount();

                    deviceSpecMapping.reserve(mappingCount);

                    for (int j = 0; j < mappingCount; ++j)
                    {
                        XmlNodeRef modelNode = fileNode->getChild(j);
                        AZStd::string model = modelNode->getAttr("model");

                        if (!model.empty())
                        {
                            deviceSpecMapping.push_back(AZStd::make_pair(model, file));
                        }
                    }
                }
            }
        }

        void LoadGpuSpecMapping_impl(const char* filename)
        {
            XmlNodeRef xmlNode = GetISystem()->LoadXmlFromFile(filename);

            if (!xmlNode)
            {
                return;
            }

            const int fileCount = xmlNode->getChildCount();

            for (int i = 0; i < fileCount; ++i)
            {
                XmlNodeRef fileNode = xmlNode->getChild(i);
                AZStd::string file = fileNode->getAttr("file");

                if (!file.empty())
                {
                    const int mappingCount = fileNode->getChildCount();

                    gpuSpecMapping.reserve(mappingCount);

                    for (int j = 0; j < mappingCount; ++j)
                    {
                        XmlNodeRef modelNode = fileNode->getChild(j);
                        GpuApiPair gpuApiPair;
                        gpuApiPair.gpuDescription = modelNode->getAttr("gpuName");
                        gpuApiPair.apiDescription = modelNode->getAttr("apiVersion");

                        if (!gpuApiPair.gpuDescription.empty() || !gpuApiPair.apiDescription.empty())
                        {
                            gpuSpecMapping.push_back(AZStd::make_pair(gpuApiPair, file));
                        }
                    }
                }
            }
        }
        
        bool GetSpecForModelName(const AZStd::string& modelName, AZStd::string& specName)
        {
            for (const auto& descriptionSpecPair : deviceSpecMapping)
            {
                AZStd::regex currentRegex(descriptionSpecPair.first.c_str());
                if (AZStd::regex_search(modelName, currentRegex))
                {
                    specName = descriptionSpecPair.second;
                    return true;
                }
            }

            return false;
        }
        
    } // namespace Internal
} // namespace MobileSysInspect
