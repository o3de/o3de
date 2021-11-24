/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AWSCore
{
    static constexpr const char AWSChinaRegionPrefix[] = "cn-";

    static constexpr const char AWSFeatureGemRESTApiIdKeyNameSuffix[] = ".RESTApiId";
    static constexpr const char AWSFeatureGemRESTApiStageKeyNameSuffix[] = ".RESTApiStage";

    static constexpr const char ResourceMappingAccountIdKeyName[] = "AccountId";
    static constexpr const char ResourceMappingResourcesKeyName[] = "AWSResourceMappings";
    static constexpr const char ResourceMappingNameIdKeyName[] = "Name/ID";
    static constexpr const char ResourceMappingRegionKeyName[] = "Region";
    static constexpr const char ResourceMappingTypeKeyName[] = "Type";
    static constexpr const char ResourceMappingVersionKeyName[] = "Version";

    static constexpr const char ResourceMapppingJsonSchemaFilePath[] =
        "Gems/AWSCore/resource_mapping_schema.json";
} // namespace AWSCore
