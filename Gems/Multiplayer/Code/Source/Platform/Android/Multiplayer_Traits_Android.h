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

#define AZ_TRAIT_MULTIPLAYER_ADDRESS_TYPE GridMate::Driver::BSD_AF_INET
#define AZ_TRAIT_MULTIPLAYER_ASSIGN_NETWORK_FAMILY 0
#define AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_ID static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_ID_DESC static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_SESSION_TEMPLATE static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_SESSION_TEMPLATE_DESC static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_DRIVER_MESSAGE static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_GRID_SYSTEM_CHECK_SECURITY_DATA(...) static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_GRID_SYSTEM_CHECK_SECURITY_DATA_ENABLE 0
#define AZ_TRAIT_MULTIPLAYER_GRID_SYSTEM_CHECK_SECURITY_DATA_MESSAGE static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_GRID_SYSTEM_HAS_PLATFORM_SERVICE_TYPE_CLASS static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_GRID_SYSTEM_HAS_PLATFORM_SERVICE_WRAPPER 0
#define AZ_TRAIT_MULTIPLAYER_GRIDMATE_SERVICE_TYPE_ENUM static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_LOBBY_SERVICE_ASSIGN_DEFAULT_PORT(...) // not implemented // 
#define AZ_TRAIT_MULTIPLAYER_LOBBY_SERVICE_ASSIGN_DEFAULT_PORT_VALUE 0
#define AZ_TRAIT_MULTIPLAYER_LOBBY_SERVICE_ENABLE_PROVO_BUTTON 0
#define AZ_TRAIT_MULTIPLAYER_LOBBY_SERVICE_ENABLE_XENIA_BUTTON 0
#define AZ_TRAIT_MULTIPLAYER_REGISTER_CVAR_SECURITY_DATA_DESC "Security data for session."
#define AZ_TRAIT_MULTIPLAYER_SESSION_NAME static_assert(false, "Unused")
#define AZ_TRAIT_MULTIPLAYER_USE_MATCH_MAKER_CVARS 0
