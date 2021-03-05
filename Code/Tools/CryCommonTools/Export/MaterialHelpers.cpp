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

#include "StdAfx.h"
#include "MaterialHelpers.h"
#include "StringHelpers.h"
#include "PathHelpers.h"
#include "properties.h"

MaterialHelpers::MaterialInfo::MaterialInfo()
{
    this->id = -1;
    this->name = "";
    this->physicalize = "None";
    this->diffuseTexture = "";
    this->diffuseColor[0] = this->diffuseColor[1] = this->diffuseColor[2] = 1.0f;
    this->specularColor[0] = this->specularColor[1] = this->specularColor[2] = 1.0f;
    this->emissiveColor[0] = this->emissiveColor[1] = this->emissiveColor[2] = 0.0f;
}

std::string MaterialHelpers::PhysicsIDToString(const int physicsID)
{
    switch (physicsID)
    {
    case 1:
        return "Default";
        break;
    case 2:
        return "ProxyNoDraw";
        break;
    case 3:
        return "NoCollide";
        break;
    case 4:
        return "Obstruct";
        break;
    default:
        return "None";
        break;
    }
}

bool MaterialHelpers::WriteMaterials(const std::string& filename, const std::vector<MaterialInfo>& materialList)
{
    FILE* materialFile = fopen(filename.c_str(), "w");
    if (materialFile)
    {
        fprintf(materialFile, "<Material MtlFlags=\"524544\" >\n");
        fprintf(materialFile, "   <SubMaterials>\n");
        for (int i = 0; i < materialList.size(); i++)
        {
            const MaterialInfo& material = materialList[i];

            fprintf(materialFile, "      <Material Name=\"%s\" ", material.name.c_str());

            if (strcmp(material.physicalize.c_str(), "ProxyNoDraw") == 0)
            {
                fprintf(materialFile, "MtlFlags=\"1152\" Shader=\"Nodraw\" GenMask=\"0\" ");
            }
            else
            {
                fprintf(materialFile, "MtlFlags=\"524416\" Shader=\"Illum\" GenMask=\"100000000\" ");
            }

            fprintf(materialFile, "SurfaceType=\"\" MatTemplate=\"\" ");
            fprintf(materialFile, "Diffuse=\"%f,%f,%f\" ", material.diffuseColor[0], material.diffuseColor[1], material.diffuseColor[2]);
            fprintf(materialFile, "Specular=\"%f,%f,%f\" ", material.specularColor[0], material.specularColor[1], material.specularColor[2]);
            fprintf(materialFile, "Emissive=\"%f,%f,%f\" ", material.emissiveColor[0], material.emissiveColor[1], material.emissiveColor[2]);
            fprintf(materialFile, "Shininess=\"10\" ");
            fprintf(materialFile, "Opacity=\"1\" ");
            fprintf(materialFile, ">\n");

            fprintf(materialFile, "         <Textures>\n");

            // Write out diffuse texture.
            if (material.diffuseTexture.length() > 0)
            {
                //fprintf( materialFile, "            <Texture Map=\"Diffuse\" File=\"%s\" >\n", ProcessTexturePath( material.diffuseTexture ).c_str() );
                fprintf(materialFile, "            <Texture Map=\"Diffuse\" File=\"%s\" >\n", material.diffuseTexture.c_str());
                fprintf(materialFile, "               <TexMod />\n");
                fprintf(materialFile, "            </Texture>\n");
            }

            fprintf(materialFile, "         </Textures>\n");
            fprintf(materialFile, "      </Material>\n");
        }
        fprintf(materialFile, "   </SubMaterials>\n");
        fprintf(materialFile, "</Material>\n");
        fclose(materialFile);

        return true;
    }
    else
    {
        return false;
    }
}