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
#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SPRITEIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SPRITEIMPL_H
#pragma once

namespace Serialization
{

inline bool Serialize(IArchive& ar, Sprite& value, const char* name, const char* label)
{
    if (ar.IsEdit())
    {
        return ar(SStruct::ForEdit(value), name, label);
    }
    else 
    {
        return ar(*value.m_path, name, label);
    }
}

} // namespace Serialization

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SPRITEIMPL_H
