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

#ifndef CRYINCLUDE_CRYCOMMON_IRESOURCECOLLECTOR_H
#define CRYINCLUDE_CRYCOMMON_IRESOURCECOLLECTOR_H
#pragma once


// used to collect the assets needed for streaming and to gather statistics
struct IResourceCollector
{
    // <interfuscator:shuffle>
    // Arguments:
    //   dwMemSize 0xffffffff if size is unknown
    // Returns:
    //   true=new resource was added, false=resource was already registered
    virtual bool AddResource(const char* szFileName, const uint32 dwMemSize) = 0;

    // Arguments:
    //   szFileName - needs to be registered before with AddResource()
    //   pInstance - must not be 0
    virtual void AddInstance(const char* szFileName, void* pInstance) = 0;
    //
    // Arguments:
    //   szFileName - needs to be registered before with AddResource()
    virtual void OpenDependencies(const char* szFileName) = 0;
    //
    virtual void CloseDependencies() = 0;

    // Resets the internal data structure for the resource collector.
    virtual void Reset() = 0;
    // </interfuscator:shuffle>
protected:
    virtual ~IResourceCollector() {}
};


class NullResCollector
    : public IResourceCollector
{
public:
    virtual bool AddResource([[maybe_unused]] const char* szFileName, [[maybe_unused]] const uint32 dwMemSize) { return true; }
    virtual void AddInstance([[maybe_unused]] const char* szFileName, [[maybe_unused]] void* pInstance) {}
    virtual void OpenDependencies([[maybe_unused]] const char* szFileName) {}
    virtual void CloseDependencies() {}
    virtual void Reset() {}

    virtual ~NullResCollector() {}
};


#endif // CRYINCLUDE_CRYCOMMON_IRESOURCECOLLECTOR_H


