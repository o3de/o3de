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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMORPHDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMORPHDATA_H
#pragma once


class IMorphData
{
public:
    virtual void SetHandle(const void* handle) = 0;
    virtual void AddMorph(const void* handle, const char* name, const char* fullName = NULL) = 0;
    virtual const void* GetHandle() const = 0;
    virtual int GetMorphCount() const = 0;
    virtual const void* GetMorphHandle(int morphIndex) const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMORPHDATA_H
