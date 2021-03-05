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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ICONVERTOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ICONVERTOR_H
#pragma once


#include "ConvertContext.h"


// A Convertor does no actual work, it merely describes the work that a Compiler will do,
// and can create Compiler instances to do the actual processing.

// Compiler interface, all compilers must implement this interface.
struct ICompiler
{
    virtual ~ICompiler() = default;

    // Release memory of interface.
    virtual void Release() = 0;

    // This function is called by RC before starting processing files.
    virtual void BeginProcessing(const IConfig* config) = 0;

    // This function is called by RC after finishing processing files.
    virtual void EndProcessing() = 0;

    // Return a convert context object.
    // RC will fill with compilation parameters right before calling Process().
    virtual IConvertContext* GetConvertContext() = 0;

    // Process a file.
    //
    // The file and processing parameters are provided by RC
    // by calling appropriate functions of a convert context
    // object returned by GetConvertContext().
    //
    // Returns true if succeeded.
    virtual bool Process() = 0;

    virtual bool CreateJobs() { return false; };
};

class RcFile;
struct ConvertorInitContext
{
    const IConfig* config;
    size_t inputFileCount;
    const RcFile* inputFiles;
    const char* appRootPath = nullptr;
};

// Convertor interface, all converters must implement this interface.
struct IConvertor
{
    virtual ~IConvertor() = default;

    // Release memory of interface.
    virtual void Release() = 0;

    //Init is called before any compilers are created.
    virtual void Init([[maybe_unused]] const ConvertorInitContext& context) {}
    virtual void DeInit() {}

    // Return an object that will do actual processing.
    // Called only once since we do not support multi-threading in RC
    virtual ICompiler* CreateCompiler() = 0;

    // Get supported extension by zero-based index.
    // If index is < 0  or >= number of supported extensions,
    // then the function *must* return 0.
    virtual const char* GetExt(int index) const = 0;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ICONVERTOR_H
