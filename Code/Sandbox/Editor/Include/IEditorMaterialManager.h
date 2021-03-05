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
#ifndef CRYINCLUDE_EDITOR_MATERIAL_IEDITORMATERIALMANAGER_H
#define CRYINCLUDE_EDITOR_MATERIAL_IEDITORMATERIALMANAGER_H
#pragma once

#define MATERIAL_FILE_EXT ".mtl"
#define DCC_MATERIAL_FILE_EXT ".dccmtl"
#define MATERIALS_PATH "materials/"

#include <Include/IBaseLibraryManager.h>
#include <IMaterial.h>


struct IEditorMaterialManager
{
    virtual void GotoMaterial(_smart_ptr<IMaterial> pMaterial) = 0;
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALMANAGER_H
