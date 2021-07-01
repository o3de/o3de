/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzFramework/Input/User/LocalUserId.h>
#include "SaveData_Traits_Platform.h"
#include "SaveDataTest.h"

void SaveDataTest::SetupInternal()
{

}

void SaveDataTest::TearDownInternal()
{

}

AzFramework::LocalUserId SaveDataTest::GetDefaultTestUserId()
{
    return AZ_TRAIT_SAVEDATA_TEST_USER_ID;
}

