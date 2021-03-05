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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_INAVIGATIONPROVIDER_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_INAVIGATIONPROVIDER_H
#pragma once

namespace Serialization
{
    struct SNavigationContext
    {
        string path;
    };

    struct INavigationProvider
    {
        virtual ~INavigationProvider() = default;
        virtual const char* GetIcon(const char* type, const char* path) const = 0;
        virtual const char* GetFileSelectorMaskForType(const char* type) const = 0;
        virtual const char* GetEngineTypeForInputType(const char* extension) const { return extension; }
        virtual bool IsSelected(const char* type, const char* path, int index) const = 0;
        virtual bool IsActive(const char* type, const char* path, int index) const = 0;
        virtual bool IsModified(const char* type, const char* path, int index) const = 0;
        virtual bool Select(const char* type, const char* path, int index) const = 0;
        virtual bool CanSelect([[maybe_unused]] const char* type, [[maybe_unused]] const char* path, [[maybe_unused]] int index) const { return false; }
        virtual bool CanPickFile([[maybe_unused]] const char* type, [[maybe_unused]] int index) const { return true; }
        virtual bool CanCreate([[maybe_unused]] const char* type, [[maybe_unused]] int index) const { return false; }
        virtual bool Create([[maybe_unused]] const char* type, [[maybe_unused]] const char* path, [[maybe_unused]] int index) const { return false; }
        virtual bool IsRegistered([[maybe_unused]] const char* type) const { return false; }
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_INAVIGATIONPROVIDER_H
