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

#include <TestImpactFramework/TestImpactChangeListException.h>
#include <TestImpactFramework/TestImpactChangeListSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    namespace ChangeListFields
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "createdFiles",
            "updatedFiles",
            "deletedFiles"
        };

        enum
        {
            CreateKey,
            UpdateKey,
            DeleteKey
        };
    } // namespace

    AZStd::string SerializeChangeList(const ChangeList& changeList)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        const auto serializeFileList = [&writer](const char* key, const AZStd::vector<RepoPath>& fileList)
        {
            writer.Key(key);
            writer.StartArray();

            for (const auto& file : fileList)
            {
                writer.String(file.c_str());
            }

            writer.EndArray();
        };

        writer.StartObject();
        serializeFileList(ChangeListFields::Keys[ChangeListFields::CreateKey], changeList.m_createdFiles);
        serializeFileList(ChangeListFields::Keys[ChangeListFields::UpdateKey], changeList.m_updatedFiles);
        serializeFileList(ChangeListFields::Keys[ChangeListFields::DeleteKey], changeList.m_deletedFiles);
        writer.EndObject();

        return stringBuffer.GetString();
    }

    ChangeList DeserializeChangeList(const AZStd::string& changeListString)
    {
        ChangeList changeList;
        rapidjson::Document doc;

        if (doc.Parse<0>(changeListString.c_str()).HasParseError())
        {
            throw ChangeListException("Could not parse change list data");
        }

        const auto deserializeFileList = [&doc](const char* key)
        {
            AZStd::vector<RepoPath> fileList;

            for (const auto& file : doc[key].GetArray())
            {
                fileList.push_back(file.GetString());
            }

            return fileList;
        };

        changeList.m_createdFiles = deserializeFileList(ChangeListFields::Keys[ChangeListFields::CreateKey]);
        changeList.m_updatedFiles = deserializeFileList(ChangeListFields::Keys[ChangeListFields::UpdateKey]);
        changeList.m_deletedFiles = deserializeFileList(ChangeListFields::Keys[ChangeListFields::DeleteKey]);

        return changeList;
    }
} // namespace TestImpact
