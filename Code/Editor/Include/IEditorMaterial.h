/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef CRYINCLUDE_EDITOR_INCLUDE_IEDITORMATERIAL_H
#define CRYINCLUDE_EDITOR_INCLUDE_IEDITORMATERIAL_H


#include "BaseLibraryItem.h"
#include <IMaterial.h>

struct IEditorMaterial
    : public CBaseLibraryItem
{
    virtual int GetFlags() const = 0;
    virtual _smart_ptr<IMaterial> GetMatInfo(bool bUseExistingEngineMaterial = false) = 0;
    virtual void DisableHighlightForFrame() = 0;
};

#endif
