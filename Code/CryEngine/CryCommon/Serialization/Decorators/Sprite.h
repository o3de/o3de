//-------------------------------------------------------------------------------
// Copyright (C) Amazon.com, Inc. or its affiliates.
// All Rights Reserved.
//
// Licensed under the terms set out in the LICENSE.HTML file included at the
// root of the distribution; you may not use this file except in compliance
// with the License. 
//
// Do not remove or modify this notice or the LICENSE.HTML file.  This file
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// either express or implied. See the License for the specific language
// governing permissions and limitations under the License.
//-------------------------------------------------------------------------------
#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SPRITE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SPRITE_H
#pragma once

namespace Serialization
{
class IArchive;

struct Sprite
{
    string* m_path;
    string m_filter;
    string m_startFolder;

    // filters are defined in the following format:
    // "All Images (bmp, jpg, tga)|*.bmp;*.jpg;*.tga|Targa (tga)|*.tga"
    explicit Sprite(string& path, const char* filter = "All files|*.*", const char* startFolder = "")
    : m_path(&path)
    , m_filter(filter)
    , m_startFolder(startFolder)
    {
    }
};

bool Serialize(IArchive& ar, Sprite& value, const char* name, const char* label);

} // namespace Serialization

#include "SpriteImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SPRITE_H
