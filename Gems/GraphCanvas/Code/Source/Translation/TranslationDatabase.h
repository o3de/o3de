/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Translation/TranslationBus.h>
#include <Translation/TranslationAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>

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

        TranslationRequests::Details GetDetails(const AZStd::string& key, const Details& value) override;

        bool Get(const AZStd::string& key, AZStd::string& value) override;

        bool Add(const TranslationFormat& format) override;

    private:

        bool IsDuplicate(const AZStd::string& key);

        AZStd::unordered_map<AZStd::string, AZStd::string> m_database;

        AZStd::recursive_mutex m_mutex;
    };
}
