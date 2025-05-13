/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 #include <AzFramework/FileTag/FileTagBus.h>

 AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZF_API, AzFramework::FileTag::FileTagsEvent);
 AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(AZF_API, AzFramework::FileTag::QueryFileTagsEvent);
