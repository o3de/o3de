/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <GFxFramework/MaterialIO/Material.h>
TEST(MaterialIO, Material_SetDataFromMtl_TexModAsNextSibling_DoesNotGetStuckInInfiniteLoop)
{
    AZStd::string xmlTextModAsNextSibling(
        "<Material MtlFlags=\"524288\" Shader=\"Illum\" GenMask=\"400000000001\" StringGenMask=\"%ALLOW_SILHOUETTE_POM%SUBSURFACE_SCATTERING\" SurfaceType=\"mat_default\" MatTemplate=\"\" Diffuse=\"0.50196099,0.50196099,0.50196099\" Specular=\"0.50196099,0.50196099,0.50196099\" Emissive=\"0,0,0\" Shininess=\"10\" Opacity=\"1\" LayerAct=\"1\">\
        <Textures>\
        <Texture Map = \"Diffuse\" File=\"Environment_Global\\Props\\piles\\Global_Debris_Gravel_pile_02\\Global_Debris_Gravel_pile_02_ddiff.dds\"/>\
        <TexMod TexMod_RotateType = \"0\" TexMod_TexGenType=\"0\" TexMod_bTexGenProjected=\"0\" TileU=\"4\" TileV=\"4\"/>\
        </Textures>\
        <PublicParams GlossFromDiffuseContrast = \"1\" FresnelScale=\"1\" GlossFromDiffuseOffset=\"0\" FresnelBias=\"1\" GlossFromDiffuseAmount=\"0\" GlossFromDiffuseBrightness=\"0.333\" IndirectColor=\"0.25,0.25,0.25\"/>\
        </Material>"
    );
    AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
    EXPECT_TRUE(xmlDoc->parse<0>(xmlTextModAsNextSibling.data()));

    AZ::rapidxml::xml_node<char>* materialXmlNode = xmlDoc->first_node("Material");

    AZ::GFxFramework::Material material;
    material.SetDataFromMtl(materialXmlNode);

    azdestroy(xmlDoc);
}

TEST(MaterialIO, Material_SetDataFromMtl_TextureMissingMapAndFile_DoesNotGetStuckInInfiniteLoop)
{
    AZStd::string xmlTextureMissingMapAndFile(
        "<Material MtlFlags=\"524288\" Shader=\"Illum\" GenMask=\"400000000001\" StringGenMask=\"%ALLOW_SILHOUETTE_POM%SUBSURFACE_SCATTERING\" SurfaceType=\"mat_default\" MatTemplate=\"\" Diffuse=\"0.50196099,0.50196099,0.50196099\" Specular=\"0.50196099,0.50196099,0.50196099\" Emissive=\"0,0,0\" Shininess=\"10\" Opacity=\"1\" LayerAct=\"1\">\
        <Textures>\
        <texture/>\
        <texture Map = \"Specular\" File=\"z:/amazongdc/artworking/characters/jack/textures/jack_s.tga\"/>\
        </Textures>\
        <PublicParams GlossFromDiffuseContrast = \"1\" FresnelScale=\"1\" GlossFromDiffuseOffset=\"0\" FresnelBias=\"1\" GlossFromDiffuseAmount=\"0\" GlossFromDiffuseBrightness=\"0.333\" IndirectColor=\"0.25,0.25,0.25\"/>\
        </Material>"
    );
    AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
    EXPECT_TRUE(xmlDoc->parse<0>(xmlTextureMissingMapAndFile.data()));

    AZ::rapidxml::xml_node<char>* materialXmlNode = xmlDoc->first_node("Material");

    AZ::GFxFramework::Material material;
    material.SetDataFromMtl(materialXmlNode);

    azdestroy(xmlDoc);
}
