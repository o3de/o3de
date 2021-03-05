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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MORPHDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MORPHDATA_H
#pragma once


#include "IMorphData.h"

class MorphData
    : public IMorphData
{
public:
    MorphData();

    virtual void SetHandle(const void* handle);
    virtual void AddMorph(const void* handle, const char* name, const char* fullname);
    virtual const void* GetHandle() const;
    virtual int GetMorphCount() const;
    virtual const void* GetMorphHandle(int morphIndex) const;

    std::string GetMorphName(int morphIndex) const;
    std::string GetMorphFullName(int morphIndex) const;

private:
    struct Entry
    {
        Entry(const void* handle, const std::string& name, const std::string& fullname)
            : handle(handle)
            , name(name)
            , fullname(fullname) {}
        const void* handle;
        std::string name;
        std::string fullname;
    };

    const void* m_handle;
    std::vector<Entry> m_morphs;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MORPHDATA_H
