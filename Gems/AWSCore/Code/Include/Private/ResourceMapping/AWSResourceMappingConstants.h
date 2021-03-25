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

namespace AWSCore
{
    static constexpr const char AWS_CHINA_REGION_PREFIX[] = "cn-";

    static constexpr const char AWS_FEATURE_GEM_RESTAPI_ID_KEYNAME_SUFFIX[] = ".RESTApiId";
    static constexpr const char AWS_FEATURE_GEM_RESTAPI_STAGE_KEYNAME_SUFFIX[] = ".RESTApiStage";

    static constexpr const char RESOURCE_MAPPING_ACCOUNTID_KEYNAME[] = "AccountId";
    static constexpr const char RESOURCE_MAPPING_RESOURCES_KEYNAME[] = "AWSResourceMappings";
    static constexpr const char RESOURCE_MAPPING_NAMEID_KEYNAME[] = "Name/ID";
    static constexpr const char RESOURCE_MAPPING_REGION_KEYNAME[] = "Region";
    static constexpr const char RESOURCE_MAPPING_TYPE_KEYNAME[] = "Type";
    static constexpr const char RESOURCE_MAPPING_VERSION_KEYNAME[] = "Version";

    // TODO: move this into an independent file under AWSCore gem, if resource mapping tool can reuse it
    static constexpr const char RESOURCE_MAPPING_JSON_SCHEMA[] =
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
