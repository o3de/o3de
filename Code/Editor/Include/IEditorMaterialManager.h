/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_MATERIAL_IEDITORMATERIALMANAGER_H
#define CRYINCLUDE_EDITOR_MATERIAL_IEDITORMATERIALMANAGER_H
#pragma once

#include <Include/IBaseLibraryManager.h>
#include <IMaterial.h>


struct IEditorMaterialManager
{
    virtual void GotoMaterial(IMaterial* pMaterial) = 0;
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALMANAGER_H
