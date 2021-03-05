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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MATERIALHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MATERIALHELPERS_H
#pragma once


namespace MaterialHelpers
{
    struct MaterialInfo
    {
        MaterialInfo();// : id(-1) { }

        std::string name;
        std::string physicalize;
        int id;

        float diffuseColor[3];
        float specularColor[3];
        float emissiveColor[3];
        std::string diffuseTexture;
    };

    std::string PhysicsIDToString(const int physicsID);
    bool WriteMaterials(const std::string& filename, const std::vector<MaterialHelpers::MaterialInfo>& materialList);
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MATERIALHELPERS_H
