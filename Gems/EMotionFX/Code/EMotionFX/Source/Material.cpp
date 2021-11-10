/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "Material.h"
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Material, MaterialAllocator, 0)


    // constructor
    Material::Material(const char* name)
    {
        SetName(name);
    }


    // destructor
    Material::~Material()
    {
    }


    // create a material
    Material* Material::Create(const char* name)
    {
        return aznew Material(name);
    }


    // set the material name
    void Material::SetName(const char* name)
    {
        // calculate the ID
        m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // return the material name
    const char* Material::GetName() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId).c_str();
    }


    // return the material name as a string
    const AZStd::string& Material::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId);
    }


    // return a clone
    Material* Material::Clone() const
    {
        Material* newMat = Material::Create(GetName());
        return newMat;
    }
} // namespace EMotionFX
