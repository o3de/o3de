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

#pragma once

#include <Translation/TranslationBus.h>
#include <Translation/TranslationAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>

namespace GraphCanvas
{

    // This does not get stored as-is, this is built up during editor load
    // time and becomes the one source of truth for translated or "pretty named" things.
    class TranslationDatabase
        : protected TranslationRequestBus::Handler
        , protected AZ::Data::AssetBus::MultiHandler
        , protected AZ::Data::AssetCatalogRequestBus::Handler
        , protected AzFramework::AssetCatalogEventBus::Handler
    {
    public:

        TranslationDatabase();

        ~TranslationDatabase();

        void Init();

        void Restore() override;

        void DumpDatabase(const AZStd::string& filename) override;

        bool HasKey(const AZStd::string& key) override;

        TranslationRequests::Details GetDetails(const AZStd::string& key) override;

        const char* Get(const AZStd::string& key) override;

        bool Add(const TranslationFormat& format) override;

    private:

        bool IsDuplicate(const AZStd::string& key);

        AZStd::unordered_map<AZStd::string, AZStd::string> m_database;

        AZStd::recursive_mutex m_mutex;
    };
}
