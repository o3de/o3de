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
#include "CrySystem_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include "MaterialUtils.h"

#include <IConsole.h>


TEST(CrySystemMaterialUtilsTests, MaterialUtilsTestBasics)
{
    char tempBuffer[AZ_MAX_PATH_LEN] = { 0 };
    // call to ensure that it handles nullptr without crashing
    MaterialUtils::UnifyMaterialName(nullptr);
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(tempBuffer[0] == 0);
}

TEST(CrySystemMaterialUtilsTests, MaterialUtilsTestExtensions)
{
    char tempBuffer[AZ_MAX_PATH_LEN];
    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, "blahblah.mtl");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "blahblah") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, "blahblah.mat.mat.abc.test.mtl");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "blahblah.mat.mat.abc.test") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, "test/.mat.mat/blahblah.mat.mat.abc.test.mtl");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "test/.mat.mat/blahblah.mat.mat.abc.test") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, ".mat.mat.blahblah.mat.mat.abc.test.mtl");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, ".mat.mat.blahblah.mat.mat.abc.test") == 0);
}

TEST(CrySystemMaterialUtilsTests, MaterialUtilsTestPrefixes)
{
    char tempBuffer[AZ_MAX_PATH_LEN];
    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, ".\\blahblah.mat");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "blahblah") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, "./materials/blahblah.mat.mat.abc.test");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "materials/blahblah.mat.mat.abc") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, ".\\engine\\materials\\blahblah.mat.mat.abc.test");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "materials/blahblah.mat.mat.abc") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, "engine/materials/blahblah.mat.mat.abc.test");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "materials/blahblah.mat.mat.abc") == 0);

    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, "materials/blahblah.mat");
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "materials/blahblah") == 0);
}

TEST(CrySystemMaterialUtilsTests, MaterialUtilsTestGameName)
{
    char tempBuffer[AZ_MAX_PATH_LEN];
    
    ICVar* pGameNameCVar = nullptr;
    if ((gEnv)&&(gEnv->pConsole))
    {
        pGameNameCVar = gEnv->pConsole->GetCVar("sys_game_folder");
    }

    azsnprintf(tempBuffer, AZ_MAX_PATH_LEN, ".\\%s\\materials\\blahblah.mat.mat.abc.test", pGameNameCVar ? pGameNameCVar->GetString() : "SamplesProject");
 
    MaterialUtils::UnifyMaterialName(tempBuffer);
    EXPECT_TRUE(strcmp(tempBuffer, "materials/blahblah.mat.mat.abc") == 0);
}