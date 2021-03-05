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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMATERIALDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMATERIALDATA_H
#pragma once


class IMaterialData
{
public:
    // the handle represents an implementation specific underlying handle (like a maya pointer to a string dag name).
    virtual int AddMaterial(const char* name, int id, const void* handle, const char* properties) = 0;
    virtual int AddMaterial(const char* name, int id, const char* subMatName, const void* handle, const char* properties) = 0;
    virtual int GetMaterialCount() const = 0;
    virtual const char* GetName(int materialIndex) const = 0;
    virtual int GetID(int materialIndex) const = 0;
    virtual const char* GetSubMatName(int materialIndex) const = 0;
    virtual const void* GetHandle(int materialIndex) const = 0;
    virtual const char* GetProperties(int materialIndex) const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IMATERIALDATA_H
