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
// ResourceCompilerTest.cpp

#include "ResourceCompiler_precompiled.h"
#include "IUnitTestHelper.h"
#include "UnitTestHelper.h"
#include "IResourceCompilerHelper.h"
#include "string.h"

namespace ResourceCompilerTests
{
    void TestFileTypes(UnitTestHelper* pUnitTestHelper)
    {
        //normal cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::GetNumSourceImageFormats() == IResourceCompilerHelper::NUM_SOURCE_IMAGE_TYPE);  //should return the enum
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::GetNumEngineImageFormats() == IResourceCompilerHelper::NUM_ENGINE_IMAGE_TYPE);  //should return the enum

        //normal cases
        pUnitTestHelper->TEST_BOOL(!azstricmp(IResourceCompilerHelper::GetSourceImageFormat(IResourceCompilerHelper::SOURCE_IMAGE_TYPE_TIF, true), ".tif"));  //dot tif
        pUnitTestHelper->TEST_BOOL(!azstricmp(IResourceCompilerHelper::GetSourceImageFormat(IResourceCompilerHelper::SOURCE_IMAGE_TYPE_TIF, false), "tif"));  //without dot tif

        pUnitTestHelper->TEST_BOOL(!azstricmp(IResourceCompilerHelper::GetSourceImageFormat(IResourceCompilerHelper::SOURCE_IMAGE_TYPE_PNG, true), ".png"));  //dot png
        pUnitTestHelper->TEST_BOOL(!azstricmp(IResourceCompilerHelper::GetSourceImageFormat(IResourceCompilerHelper::SOURCE_IMAGE_TYPE_PNG, false), "png"));  //without dot png

        //edge cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::GetSourceImageFormat(IResourceCompilerHelper::NUM_SOURCE_IMAGE_TYPE, true) == nullptr);         //invalid range with dot
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::GetSourceImageFormat(IResourceCompilerHelper::NUM_SOURCE_IMAGE_TYPE, false) == nullptr);        //invalid range without dot

        //normal cases
        pUnitTestHelper->TEST_BOOL(!azstricmp(IResourceCompilerHelper::GetEngineImageFormat(IResourceCompilerHelper::ENGINE_IMAGE_TYPE_DDS, true), ".dds"));  //dot dds
        pUnitTestHelper->TEST_BOOL(!azstricmp(IResourceCompilerHelper::GetEngineImageFormat(IResourceCompilerHelper::ENGINE_IMAGE_TYPE_DDS, false), "dds"));  //without dot dds

        //edge cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::GetEngineImageFormat(IResourceCompilerHelper::NUM_ENGINE_IMAGE_TYPE, true) == nullptr);         //invalid range with dot
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::GetEngineImageFormat(IResourceCompilerHelper::NUM_ENGINE_IMAGE_TYPE, false) == nullptr);        //invalid range without dot

        //normal cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("somefile.tga") == true);          //file name
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("somefile.dds") == false);         //unsupported file name
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("blah/blah/some.png") == true);    //full file name
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("blah/blah/some.dds") == false);   //unsupported full file names
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("tga") == true);                   //no dot
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported(".tga") == true);                  //with dot
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported(".TGA") == true);                  //with dot case insensitive
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("Png") == true);                   //without case insensitive
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("gifs") == false);                 //extra characters not supported
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("Targa") == false);                //names not supported
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("dds") == false);                  //invalid format

        //edge cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported("") == false);                     //empty string
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsSourceImageFormatSupported(nullptr) == false);                //nullptr

        //normal cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("somefile.dds") == true);            //file name
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("somefile.gif") == false);           //unsupported file name
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("blah/blah/some.dds") == true);      //full file name
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("blah/blah/some.png") == false);     //unsupported full file names
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("dds") == true);                     //no dot
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported(".dds") == true);                    //with dot
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported(".DDs") == true);                    //with dot case insensitive
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("ddS") == true);                     //without case insensitive
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("ddSs") == false);                    //extra characters not supported
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("Direct Draw Surface") == false);    //names not supported
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("gif") == false);                    //invalid format

        //edge cases
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported("") == false);                       //empty string
        pUnitTestHelper->TEST_BOOL(IResourceCompilerHelper::IsGameImageFormatSupported(nullptr) == false);                  //nullptr
    }

    void Run(UnitTestHelper* pUnitTestHelper)
    {
        TestFileTypes(pUnitTestHelper);
    }
}
