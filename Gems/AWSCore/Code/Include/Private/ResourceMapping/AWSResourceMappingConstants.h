/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    // TODO: move this into an independent file under AWSCore gem, if resource mapping tool can reuse it
    static constexpr const char ResourceMappingJsonSchema[] =
        R"({
    "$schema": "http://json-schema.org/draft-04/schema",
    "type": "object",
    "title": "The AWS Resource Mapping Root schema",
    "required": ["AWSResourceMappings", "AccountId", "Region", "Version"],
    "properties": {
        "AWSResourceMappings": {
            "type": "object",
            "title": "The AWSResourceMappings schema",
            "patternProperties": {
                "^.+$": {
                    "type": "object",
                    "title": "The AWS Resource Entry schema",
                    "required": ["Type", "Name/ID"],
                    "properties": {
                        "Type": {
                            "$ref": "#/NonEmptyString"
                        },
                        "Name/ID": {
                            "$ref": "#/NonEmptyString"
                        },
                        "AccountId": {
                            "$ref": "#/AccountIdString"
                        },
                        "Region": {
                            "$ref": "#/RegionString"
                        }
                    }
                }
            },
            "additionalProperties": false
        },
        "AccountId": {
            "$ref": "#/AccountIdString"
        },
        "Region": {
            "$ref": "#/RegionString"
        },
        "Version": {
            "pattern": "^[0-9]{1}.[0-9]{1}.[0-9]{1}$"
        }
    },
    "AccountIdString": {
        "type": "string",
        "pattern": "^[0-9]{12}$"
    },
    "NonEmptyString": {
        "type": "string",
        "minLength": 1
    },
    "RegionString": {
        "type": "string",
        "pattern": "^[a-z]{2}-[a-z]{4,9}-[0-9]{1}$"
    },
    "additionalProperties": false
})";

} // namespace AWSCore
