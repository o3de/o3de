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

#define AZ_TRAIT_AUDIOSYSTEM_ATL_POOL_SIZE 8 << 10 /* 8 MiB (re-evaluate this size!) */
#define AZ_TRAIT_AUDIOSYSTEM_ATL_POOL_SIZE_DEFAULT_TEXT "8192 (8 MiB)"
#define AZ_TRAIT_AUDIOSYSTEM_AUDIO_EVENT_POOL_SIZE 64
#define AZ_TRAIT_AUDIOSYSTEM_AUDIO_EVENT_POOL_SIZE_DEFAULT_TEXT "64"
#define AZ_TRAIT_AUDIOSYSTEM_AUDIO_OBJECT_POOL_SIZE 128
#define AZ_TRAIT_AUDIOSYSTEM_AUDIO_OBJECT_POOL_SIZE_DEFAULT_TEXT "128"
#define AZ_TRAIT_AUDIOSYSTEM_AUDIO_THREAD_AFFINITY AFFINITY_MASK_ALL
#define AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_ALLOCATION_POLICY IMemoryManager::eapCustomAlignment
#define AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_SIZE 2 << 10 /* 2 MiB (re-evaluate this size!) */
#define AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_SIZE_DEFAULT_TEXT "2048 (2 MiB)"