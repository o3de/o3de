#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import sys
import os
sys.path.append("..")
sys.path.append("../..")
from clr import *
import testfuncs

def verifyPackingRelaxed(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Storage Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst

        predicates.append(lambda: j["ShaderResourceGroups"][0]["bufferForSRGConstants"]["count"]  ==   1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["bufferForSRGConstants"]["index"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["bufferForSRGConstants"]["space"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["bufferForSRGConstants"]["id"]     ==   "ExampleSRG")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["bufferForSRGConstants"]["usage"]  ==   "Read")

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteOffset"]  ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteSize"]    ==  16) # Complex type. Size & offset match previous entries' sum
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["typeName"]            ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteSize"]    ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteOffset"]  ==  64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteSize"]    ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteOffset"]  ==  96)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteSize"]    ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteOffset"]  ==  112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteSize"]    ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteOffset"]  ==  128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteSize"]    ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteOffset"]  ==  96)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"] == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteSize"]   ==  8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"] == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteSize"]   ==  8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"] == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"] == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"] == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"] == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteSize"]   ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["typeName"]           ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"] == 176)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"] == 192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"] == 224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"] == 240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"] == 256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteOffset"] == 272)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteOffset"] == 288)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteSize"]   ==  36)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteOffset"] == 224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteOffset"] == 240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteOffset"] == 256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteSize"]   ==  64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteOffset"] ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteSize"]   == 272)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["typeName"]           ==  "/ExampleSRG/T")

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<T>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]  == 268)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["type"]    == "ConstantBuffer<T>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]  == 268)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["index"]   == 4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]  == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]    == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]  == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["stride"]  == 12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["type"]    == "Buffer<float2>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["stride"]  == 8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["index"]   == 4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["stride"]  == 12)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["id"] == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["type"] == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["index"]   == 6)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["stride"]  == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["id"] == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["type"] == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["index"]   == 7)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["stride"]  == 12)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingDirectX(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-dx12", "--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches 2 (DX) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteOffset"]  ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteSize"]    ==  12) # Complex type. Size & offset match previous entries' sum
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["typeName"]            ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteSize"]    ==  40) # float2x3 - 2 full registers of 16 bytes each and 2 elements of the last register of 4 bytes each
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteOffset"]  ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteSize"]    ==  28) # row float2x3 - 1 full register of 16 bytes and 3 elements of the last register of 4 bytes each
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteOffset"]  == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteOffset"]  == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteOffset"]  == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteOffset"]  == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteSize"]    ==  36)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteSize"]   ==  8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"] == 176)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteSize"]   ==  8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteSize"]   ==  24)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"] == 192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"] == 192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteSize"]   ==  28)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["typeName"]           ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"] == 224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"] == 240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"] == 256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"] == 272)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"] == 288)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"] == 304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"] == 320)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteOffset"] == 336)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteOffset"] == 352)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteOffset"] == 224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteSize"]   == 132)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteOffset"] == 368)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteOffset"] == 384)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteOffset"] == 400)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteOffset"] == 416)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteOffset"] == 368)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteSize"]   ==  60)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteOffset"] ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteSize"]   == 428)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["typeName"]           ==  "/ExampleSRG/T")

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]   == 428)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]   == 428)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]   == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]     == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]   == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["type"]     == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["stride"]   == 12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["type"]     == "Buffer<float2>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["stride"]   == 8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["type"]     == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["stride"]   == 12)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["id"] == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["type"] == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["stride"]  == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["id"] == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["type"] == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["stride"]  == 12)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingVulkan(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-vulkan", "--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Uniform Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteOffset"]  ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteSize"]    ==  16) # Complex type. Size & offset match previous entries' sum
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["typeName"]            ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteSize"]    ==  48) # float2x3 - 3 full registers of 16 bytes each 
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteOffset"]  ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteSize"]    ==  32) # row float2x3 - 2 full registers of 16 bytes each
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteOffset"]  == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteSize"]    ==  16) # each array element is 16-byte aligned
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteOffset"]  == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteSize"]    ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteOffset"]  == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteSize"]    ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteOffset"]  == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteSize"]    ==  48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"] == 176)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteSize"]   ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"] == 192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"] == 192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteSize"]   ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["typeName"]           ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"] == 224) # Confirmed: SpirV can't pack 4 bytes after an array
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"] == 240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"] == 256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"] == 272)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"] == 288)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"] == 304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"] == 320)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteOffset"] == 336)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteOffset"] == 352)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteOffset"] == 224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteSize"]   == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteOffset"] == 368)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteOffset"] == 384)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteOffset"] == 400)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteOffset"] == 416)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteOffset"] == 368)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteSize"]   ==  64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteOffset"] ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteSize"]   == 432)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["typeName"]           ==  "/ExampleSRG/T")

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]   == 432)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]   == 432)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]   == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]     == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]   == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["type"]     == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["stride"]   == 16) # Note that StructuredBuffers in SpirV follow the base alignment for arrays rule (align-16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["type"]     == "Buffer<float2>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["stride"]   == 16) # Note that Buffers in SpirV follow the base alignment for arrays rule (align-16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["type"]     == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["stride"]   == 16) # Note that StructuredBuffers in SpirV follow the base alignment for arrays rule (align-16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["id"] == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["type"] == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["stride"]  == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["id"] == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["type"] == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["stride"]  == 12)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyStructsPackingVulkan(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-vulkan", "--srg"])

    # Packing vectors in nested structs is trickier.
    # The following values match the dxc's SpirV generation

    if ok:
        predicates = []
        # check all references of func()

        # Matches Uniform Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["qualifiedName"]       ==   "/ExampleSRG/Sab/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["qualifiedName"]       ==   "/ExampleSRG/Sab/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteOffset"]  ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteSize"]    ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["qualifiedName"]       ==   "/ExampleSRG/T/ab") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["typeName"]            ==   "/ExampleSRG/Sab")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteSize"]    ==  16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["qualifiedName"]       ==   "/ExampleSRG/Sba/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteSize"]    ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["qualifiedName"]       ==   "/ExampleSRG/Sba/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteOffset"]  ==  24)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["qualifiedName"]       ==   "/ExampleSRG/T/ba") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["typeName"]            ==   "/ExampleSRG/Sba")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteSize"]    ==  16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["qualifiedName"]       ==   "/ExampleSRG/Sabc/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["qualifiedName"]       ==   "/ExampleSRG/Sabc/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteOffset"]  ==  36)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteSize"]    ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["qualifiedName"]       ==   "/ExampleSRG/Sabc/c")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteOffset"]  ==  48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["qualifiedName"]       ==   "/ExampleSRG/T/abc") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["typeName"]            ==   "/ExampleSRG/Sabc")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteSize"]    ==  32)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["qualifiedName"]      ==   "/ExampleSRG/Sac/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"] ==  64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["qualifiedName"]      ==   "/ExampleSRG/Sac/c")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"] ==  68)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["qualifiedName"]      ==   "/ExampleSRG/T/ac")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["typeName"]           ==   "/ExampleSRG/Sac")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"] == 64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteSize"]   ==  16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["qualifiedName"]      ==   "/ExampleSRG/S/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"] ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["qualifiedName"]      ==   "/ExampleSRG/S/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"] ==  96)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["qualifiedName"]      ==   "/ExampleSRG/T/s")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["typeName"]           ==   "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"] ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteSize"]   ==  32)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["qualifiedName"]      ==   "/ExampleSRG/T/a_float")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"] == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteSize"]   ==   4)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["qualifiedName"]      ==   "/ExampleSRG/U/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"] ==   128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["qualifiedName"]      ==   "/ExampleSRG/U/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"] ==   132)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteSize"]   ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["qualifiedName"]      ==   "/ExampleSRG/U/c")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"] ==   140)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["qualifiedName"]      ==   "/ExampleSRG/U/d")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"] ==   144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteSize"]   ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["qualifiedName"]      ==   "/ExampleSRG/T/u") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["typeName"]           ==   "/ExampleSRG/U")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"] ==   128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteSize"]   ==  32)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["qualifiedName"]      ==   "/ExampleSRG/m_CB")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["typeName"]           ==   "/ExampleSRG/T")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"] ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteSize"]   == 160)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingOpenGL(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-opengl", "--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches std140 (GL) standard here:
        # https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
        # std140 : https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec2.html
        # std430 : https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec3.html
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteOffset"]  ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteSize"]    ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["typeName"]            ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteSize"]    ==  48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteOffset"]  ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteSize"]    ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteOffset"]  == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteOffset"]  == 116)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteOffset"]  == 120)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteOffset"]  == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"] == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteSize"]   ==  8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"] == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteSize"]   ==  8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"] == 128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteSize"]   ==  24)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"] == 192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"] == 160)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteSize"]   ==  48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["typeName"]           ==  "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"] == 212)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"] == 216)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"] == 220)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"] == 224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"] == 228)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"] == 232)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteOffset"] == 236)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteOffset"] == 240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteSize"]   ==  4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteOffset"] == 208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25]["constantByteSize"]   == 36)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteOffset"] == 244)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteOffset"] == 256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteOffset"] == 272)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteOffset"] == 288)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteOffset"] == 256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30]["constantByteSize"]   ==  64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteOffset"] ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["constantByteSize"]   == 320)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31]["typeName"]           ==  "/ExampleSRG/T")

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]   == 320)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]   == 320)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]   == 144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]     == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]   == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["type"]     == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["stride"]   == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["type"]     == "Buffer<float2>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["stride"]   == 8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["type"]     == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["count"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["stride"]   == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["id"] == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["type"] == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["stride"]  == 16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["id"] == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["type"] == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["stride"]  == 12)
        
        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyStructsPackingOpenGL(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-opengl", "--srg"])

    # Packing vectors in nested structs is trickier.
    # The following values match the dxc's OpenGL generation

    if ok:
        predicates = []
        # check all references of func()

        # Matches std340 (GL) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["qualifiedName"]       ==   "/ExampleSRG/Sab/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][0]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["qualifiedName"]       ==   "/ExampleSRG/Sab/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteOffset"]  ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][1]["constantByteSize"]    ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["qualifiedName"]       ==   "/ExampleSRG/T/ab") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["typeName"]            ==   "/ExampleSRG/Sab")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteOffset"]  ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][2]["constantByteSize"]    ==  16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["qualifiedName"]       ==   "/ExampleSRG/Sba/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][3]["constantByteSize"]    ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["qualifiedName"]       ==   "/ExampleSRG/Sba/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteOffset"]  ==  24)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][4]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["qualifiedName"]       ==   "/ExampleSRG/T/ba") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["typeName"]            ==   "/ExampleSRG/Sba")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteOffset"]  ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][5]["constantByteSize"]    ==  16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["qualifiedName"]       ==   "/ExampleSRG/Sabc/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][6]["constantByteSize"]    ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["qualifiedName"]       ==   "/ExampleSRG/Sabc/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteOffset"]  ==  36)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][7]["constantByteSize"]    ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["qualifiedName"]       ==   "/ExampleSRG/Sabc/c")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteOffset"]  ==  48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][8]["constantByteSize"]    ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["qualifiedName"]       ==   "/ExampleSRG/T/abc") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["typeName"]            ==   "/ExampleSRG/Sabc")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteOffset"]  ==  32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][9]["constantByteSize"]    ==  32)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["qualifiedName"]      ==   "/ExampleSRG/Sac/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"] ==  64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["qualifiedName"]      ==   "/ExampleSRG/Sac/c")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"] ==  68)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteSize"]   ==  12)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["qualifiedName"]      ==   "/ExampleSRG/T/ac")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["typeName"]           ==   "/ExampleSRG/Sac")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"] == 64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteSize"]   ==  16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["qualifiedName"]      ==   "/ExampleSRG/S/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"] ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["qualifiedName"]      ==   "/ExampleSRG/S/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"] ==  96)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteSize"]   ==  16)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["qualifiedName"]      ==   "/ExampleSRG/T/s")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["typeName"]           ==   "/ExampleSRG/S")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"] ==  80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteSize"]   ==  32)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["qualifiedName"]      ==   "/ExampleSRG/T/a_float")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"] == 112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteSize"]   ==   4)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["qualifiedName"]      ==   "/ExampleSRG/U/a")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"] ==   116)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["qualifiedName"]      ==   "/ExampleSRG/U/b")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"] ==   120)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteSize"]   ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["qualifiedName"]      ==   "/ExampleSRG/U/c")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"] ==   128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteSize"]   ==   4)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["qualifiedName"]      ==   "/ExampleSRG/U/d")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"] ==   132)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteSize"]   ==   8)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["qualifiedName"]      ==   "/ExampleSRG/T/u") # Complex type
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["typeName"]           ==   "/ExampleSRG/U")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"] ==   128)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteSize"]   ==  28)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["qualifiedName"]      ==   "/ExampleSRG/m_CB")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["typeName"]           ==   "/ExampleSRG/T")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"] ==   0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteSize"]   == 160)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingRelaxedUseSpaces(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Storage Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst

        # Shader Resource Group 0
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["type"]    == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]  == 16)

        # Shader Resource Group 1
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<Light>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["stride"]  == 144)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["type"]    == "Buffer<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["stride"]  == 12)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["stride"]  == 16)

        # Shader Resource Group 2
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["id"]       == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["type"]     == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["stride"]   == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["id"]       == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["type"]     == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["stride"]   == 12)


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingRelaxedNoSpaces(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Storage Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst

        # Shader Resource Group 0
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["type"]    == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]  == 16)

        # Shader Resource Group 1
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<Light>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["stride"]  == 144)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["type"]    == "Buffer<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["stride"]  == 12)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["stride"]  == 16)

        # Shader Resource Group 2
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["id"]       == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["type"]     == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["id"]       == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["type"]     == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["stride"]  == 12)


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingRelaxedUniqueIdx(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg", "--unique-idx"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Storage Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst

        # Note! Bacause AZSLc emits the resource indices in order SRVs/UAVs, then Samplers, then CBVs
        #  the indices (when using unique index) don't necessarily match the order of declaration.
        # Since the data is reflected this should not be a problem!
        # In fact, it verifies the consumer application is data driven and accepts the emitted register indices

        # Shader Resource Group 0
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["index"]   == 3)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["type"]    == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]  == 16)

        # Shader Resource Group 1
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<Light>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["index"]   == 4)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["stride"]  == 144)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["type"]    == "Buffer<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["stride"]  == 12)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["stride"]  == 16)

        # Shader Resource Group 2
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["id"]       == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["type"]     == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["id"]       == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["type"]     == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["stride"]  == 12)


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingRelaxedUniqueIdxUseSpaces(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg", "--unique-idx"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Storage Buffer 1 (VK) standard here:
        # https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst

        # Note! Bacause AZSLc emits the resource indices in order SRVs/UAVs, then Samplers, then CBVs
        #  the indices (when using unique index) don't necessarily match the order of declaration.
        # Since the data is reflected this should not be a problem!
        # In fact, it verifies the consumer application is data driven and accepts the emitted register indices

        # Shader Resource Group 0
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["type"]    == "ConstantBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["index"]   == 3)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["type"]    == "Buffer<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["index"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["space"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]  == 16)

        # Shader Resource Group 1
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["type"]    == "ConstantBuffer<Light>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["count"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["index"]   == 4)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["stride"]  == 144)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["type"]    == "Buffer<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["index"]   == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["stride"]  == 12)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["type"]    == "StructuredBuffer<S>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["count"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["index"]   == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["space"]   == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["stride"]  == 16)

        # Shader Resource Group 2
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["id"]       == "m_texCube1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["type"]     == "TextureCube")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][0]["stride"]  == 16)

        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["id"]       == "m_tex2d1")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["type"]     == "Texture2D<float3>")
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["space"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForImageViews"][1]["stride"]  == 12)


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False
    
def verifyPackingUnboundedSpillSpaces(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg", "--namespace=dx"])

    if ok:
        predicates = []

        # Shader Resource Group 0
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["id"]       == "m_texSRVa")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["space"]    == 1000)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["id"]       == "m_texSRVb")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][1]["space"]    == 1001)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][2]["id"]       == "m_texSRVc")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][2]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][2]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][2]["index"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][2]["space"]    == 0)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][3]["id"]       == "m_texSRVd")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][3]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][3]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][3]["index"]    == 3)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][3]["space"]    == 0)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][4]["id"]       == "m_texUAVa")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][4]["type"]     == "RWTexture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][4]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][4]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][4]["space"]    == 1002)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][5]["id"]       == "m_texUAVb")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][5]["type"]     == "RWTexture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][5]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][5]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][5]["space"]    == 1003)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][0]["id"]       == "m_samplera")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][0]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][0]["space"]    == 1004)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][1]["id"]       == "m_samplerb")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][1]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSamplers"][1]["space"]    == 1005)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["id"]       == "m_structArraya")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["type"]     == "ConstantBuffer<MyStruct>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["space"]    == 1006)
        
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["id"]       == "m_structArrayb")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["type"]     == "ConstantBuffer<MyStruct>")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["space"]    == 1007)

        
        # Shader Resource Group 1
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][0]["id"]       == "m_texSRVa")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][0]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][0]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][0]["space"]    == 1008)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][1]["id"]       == "m_texSRVb")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][1]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][1]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][1]["space"]    == 1009)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][2]["id"]       == "m_texSRVc")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][2]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][2]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][2]["index"]    == 2)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][2]["space"]    == 1)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][3]["id"]       == "m_texSRVd")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][3]["type"]     == "Texture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][3]["count"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][3]["index"]    == 3)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][3]["space"]    == 1)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][4]["id"]       == "m_texUAVa")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][4]["type"]     == "RWTexture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][4]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][4]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][4]["space"]    == 1010)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][5]["id"]       == "m_texUAVb")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][5]["type"]     == "RWTexture2D<float4>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][5]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][5]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForImageViews"][5]["space"]    == 1011)

        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][0]["id"]       == "m_samplera")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][0]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][0]["space"]    == 1012)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][1]["id"]       == "m_samplerb")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][1]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForSamplers"][1]["space"]    == 1013)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["id"]       == "m_structArraya")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["type"]     == "ConstantBuffer<MyStruct>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["index"]    == 0)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["space"]    == 1014)
        
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["id"]       == "m_structArrayb")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["type"]     == "ConstantBuffer<MyStruct>")
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["count"]    == -1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["index"]    == 1)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["space"]    == 1015)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingDirectXMatrices(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-dx12", "--no-alignment-validation", "--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches 2 (DX) emission here:
        # http://shader-playground.timjones.io/206f18e88f838720db8e3415362551df

        # struct T2
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0]["constantByteOffset"]  ==     0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1]["constantByteOffset"]  ==    24)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2]["constantByteOffset"]  ==    32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3]["constantByteOffset"]  ==    72)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4]["constantByteOffset"]  ==    80)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5]["constantByteOffset"]  ==   136)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6]["constantByteOffset"]  ==   144)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7]["constantByteOffset"]  ==   168)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8]["constantByteOffset"]  ==   176)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9]["constantByteOffset"]  ==   204)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"]  ==   208)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"]  ==   240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"]  ==   256)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"]  ==   280)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"]  ==   288)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"]  ==   328)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"]  ==   336)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"]  ==   392)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"]  ==   400)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"]  ==   424)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"]  ==   432)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"]  ==   464)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"]  ==   480)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteOffset"]  ==   512)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteOffset"]  ==     0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteSize"]    ==   520) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["typeName"]            ==  "/ExampleSRG/T2")

        # struct T3
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0 + 25]["constantByteOffset"]  ==     0 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1 + 25]["constantByteOffset"]  ==    28 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2 + 25]["constantByteOffset"]  ==    32 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3 + 25]["constantByteOffset"]  ==    76 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4 + 25]["constantByteOffset"]  ==    80 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5 + 25]["constantByteOffset"]  ==   140 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6 + 25]["constantByteOffset"]  ==   144 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7 + 25]["constantByteOffset"]  ==   184 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8 + 25]["constantByteOffset"]  ==   192 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9 + 25]["constantByteOffset"]  ==   236 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10 + 25]["constantByteOffset"]  ==   240 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11 + 25]["constantByteOffset"]  ==   288 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12 + 25]["constantByteOffset"]  ==   304 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13 + 25]["constantByteOffset"]  ==   336 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14 + 25]["constantByteOffset"]  ==   352 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15 + 25]["constantByteOffset"]  ==   400 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16 + 25]["constantByteOffset"]  ==   416 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17 + 25]["constantByteOffset"]  ==   480 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18 + 25]["constantByteOffset"]  ==   496 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19 + 25]["constantByteOffset"]  ==   536 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20 + 25]["constantByteOffset"]  ==   544 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21 + 25]["constantByteOffset"]  ==   592 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22 + 25]["constantByteOffset"]  ==   608 + 528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23 + 25]["constantByteOffset"]  ==   656 + 528)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][49]["constantByteOffset"]  ==   528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][49]["constantByteSize"]    ==   664) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][49]["typeName"]            ==  "/ExampleSRG/T3")


        # struct T4
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0 + 50]["constantByteOffset"]  ==     0 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1 + 50]["constantByteOffset"]  ==    32 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2 + 50]["constantByteOffset"]  ==    48 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3 + 50]["constantByteOffset"]  ==    96 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4 + 50]["constantByteOffset"]  ==   112 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5 + 50]["constantByteOffset"]  ==   176 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6 + 50]["constantByteOffset"]  ==   192 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7 + 50]["constantByteOffset"]  ==   248 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8 + 50]["constantByteOffset"]  ==   256 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9 + 50]["constantByteOffset"]  ==   316 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10 + 50]["constantByteOffset"]  ==   320 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11 + 50]["constantByteOffset"]  ==   384 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12 + 50]["constantByteOffset"]  ==   400 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13 + 50]["constantByteOffset"]  ==   432 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14 + 50]["constantByteOffset"]  ==   448 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15 + 50]["constantByteOffset"]  ==   496 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16 + 50]["constantByteOffset"]  ==   512 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17 + 50]["constantByteOffset"]  ==   576 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18 + 50]["constantByteOffset"]  ==   592 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19 + 50]["constantByteOffset"]  ==   648 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20 + 50]["constantByteOffset"]  ==   656 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21 + 50]["constantByteOffset"]  ==   720 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22 + 50]["constantByteOffset"]  ==   736 + 1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23 + 50]["constantByteOffset"]  ==   800 + 1200)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][74]["constantByteOffset"]  ==  1200)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][74]["constantByteSize"]    ==   808) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][74]["typeName"]            ==  "/ExampleSRG/T4")

        # struct TU
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0 + 75]["constantByteOffset"]  ==     0 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1 + 75]["constantByteOffset"]  ==     4 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2 + 75]["constantByteOffset"]  ==    16 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3 + 75]["constantByteOffset"]  ==    36 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4 + 75]["constantByteOffset"]  ==    48 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5 + 75]["constantByteOffset"]  ==    84 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6 + 75]["constantByteOffset"]  ==    96 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7 + 75]["constantByteOffset"]  ==   148 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8 + 75]["constantByteOffset"]  ==   152 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9 + 75]["constantByteOffset"]  ==   156 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10 + 75]["constantByteOffset"]  ==   160 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11 + 75]["constantByteOffset"]  ==   168 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12 + 75]["constantByteOffset"]  ==   176 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13 + 75]["constantByteOffset"]  ==   188 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14 + 75]["constantByteOffset"]  ==   192 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15 + 75]["constantByteOffset"]  ==   208 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16 + 75]["constantByteOffset"]  ==   212 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17 + 75]["constantByteOffset"]  ==   216 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18 + 75]["constantByteOffset"]  ==   224 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19 + 75]["constantByteOffset"]  ==   232 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20 + 75]["constantByteOffset"]  ==   240 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21 + 75]["constantByteOffset"]  ==   256 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22 + 75]["constantByteOffset"]  ==   272 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23 + 75]["constantByteOffset"]  ==   288 + 2016)
        # Special cases packing tests
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24 + 75]["constantByteOffset"]  ==   296 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25 + 75]["constantByteOffset"]  ==   304 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26 + 75]["constantByteOffset"]  ==   312 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27 + 75]["constantByteOffset"]  ==   320 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28 + 75]["constantByteOffset"]  ==   332 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29 + 75]["constantByteOffset"]  ==   336 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30 + 75]["constantByteOffset"]  ==   344 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31 + 75]["constantByteOffset"]  ==   352 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][32 + 75]["constantByteOffset"]  ==   368 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][33 + 75]["constantByteOffset"]  ==   384 + 2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][34 + 75]["constantByteOffset"]  ==   392 + 2016)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][110]["constantByteOffset"]  ==  2016)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][110]["constantByteSize"]    ==   400) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][110]["typeName"]            ==  "/ExampleSRG/TU")


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingVulkanMatrices(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-vulkan", "--no-alignment-validation", "--srg"])

    if ok:
        predicates = []
        # check all references of func()

        # Matches Uniform Buffer 1 (VK) emission here:
        # http://shader-playground.timjones.io/206f18e88f838720db8e3415362551df

        # struct T2
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0]["constantByteOffset"]  ==     0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1]["constantByteOffset"]  ==    32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2]["constantByteOffset"]  ==    48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3]["constantByteOffset"]  ==    96)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4]["constantByteOffset"]  ==   112)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5]["constantByteOffset"]  ==   176)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6]["constantByteOffset"]  ==   192)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7]["constantByteOffset"]  ==   224)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8]["constantByteOffset"]  ==   240)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9]["constantByteOffset"]  ==   272)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10]["constantByteOffset"]  ==   288)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11]["constantByteOffset"]  ==   320)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12]["constantByteOffset"]  ==   336)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13]["constantByteOffset"]  ==   368)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14]["constantByteOffset"]  ==   384)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15]["constantByteOffset"]  ==   432)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16]["constantByteOffset"]  ==   448)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17]["constantByteOffset"]  ==   512)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18]["constantByteOffset"]  ==   528)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19]["constantByteOffset"]  ==   560)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20]["constantByteOffset"]  ==   576)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21]["constantByteOffset"]  ==   608)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22]["constantByteOffset"]  ==   624)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23]["constantByteOffset"]  ==   656)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteOffset"]  ==     0)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["constantByteSize"]    ==   672) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24]["typeName"]            ==  "/ExampleSRG/T2")

        # struct T3
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0 + 25]["constantByteOffset"]  ==     0 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1 + 25]["constantByteOffset"]  ==    32 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2 + 25]["constantByteOffset"]  ==    48 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3 + 25]["constantByteOffset"]  ==    96 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4 + 25]["constantByteOffset"]  ==   112 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5 + 25]["constantByteOffset"]  ==   176 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6 + 25]["constantByteOffset"]  ==   192 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7 + 25]["constantByteOffset"]  ==   240 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8 + 25]["constantByteOffset"]  ==   256 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9 + 25]["constantByteOffset"]  ==   304 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10 + 25]["constantByteOffset"]  ==   320 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11 + 25]["constantByteOffset"]  ==   368 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12 + 25]["constantByteOffset"]  ==   384 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13 + 25]["constantByteOffset"]  ==   416 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14 + 25]["constantByteOffset"]  ==   432 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15 + 25]["constantByteOffset"]  ==   480 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16 + 25]["constantByteOffset"]  ==   496 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17 + 25]["constantByteOffset"]  ==   560 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18 + 25]["constantByteOffset"]  ==   576 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19 + 25]["constantByteOffset"]  ==   624 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20 + 25]["constantByteOffset"]  ==   640 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21 + 25]["constantByteOffset"]  ==   688 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22 + 25]["constantByteOffset"]  ==   704 + 672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23 + 25]["constantByteOffset"]  ==   752 + 672)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][49]["constantByteOffset"]  ==   672)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][49]["constantByteSize"]    ==   768) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][49]["typeName"]            ==  "/ExampleSRG/T3")


        # struct T4
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0 + 50]["constantByteOffset"]  ==     0 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1 + 50]["constantByteOffset"]  ==    32 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2 + 50]["constantByteOffset"]  ==    48 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3 + 50]["constantByteOffset"]  ==    96 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4 + 50]["constantByteOffset"]  ==   112 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5 + 50]["constantByteOffset"]  ==   176 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6 + 50]["constantByteOffset"]  ==   192 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7 + 50]["constantByteOffset"]  ==   256 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8 + 50]["constantByteOffset"]  ==   272 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9 + 50]["constantByteOffset"]  ==   336 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10 + 50]["constantByteOffset"]  ==   352 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11 + 50]["constantByteOffset"]  ==   416 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12 + 50]["constantByteOffset"]  ==   432 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13 + 50]["constantByteOffset"]  ==   464 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14 + 50]["constantByteOffset"]  ==   480 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15 + 50]["constantByteOffset"]  ==   528 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16 + 50]["constantByteOffset"]  ==   544 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17 + 50]["constantByteOffset"]  ==   608 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18 + 50]["constantByteOffset"]  ==   624 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19 + 50]["constantByteOffset"]  ==   688 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20 + 50]["constantByteOffset"]  ==   704 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21 + 50]["constantByteOffset"]  ==   768 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22 + 50]["constantByteOffset"]  ==   784 + 1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23 + 50]["constantByteOffset"]  ==   848 + 1440)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][74]["constantByteOffset"]  ==  1440)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][74]["constantByteSize"]    ==   864) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][74]["typeName"]            ==  "/ExampleSRG/T4")

        # struct TU
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 0 + 75]["constantByteOffset"]  ==     0 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 1 + 75]["constantByteOffset"]  ==     4 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 2 + 75]["constantByteOffset"]  ==     8 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 3 + 75]["constantByteOffset"]  ==    16 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 4 + 75]["constantByteOffset"]  ==    20 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 5 + 75]["constantByteOffset"]  ==    32 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 6 + 75]["constantByteOffset"]  ==    48 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 7 + 75]["constantByteOffset"]  ==    64 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 8 + 75]["constantByteOffset"]  ==    68 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][ 9 + 75]["constantByteOffset"]  ==    72 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][10 + 75]["constantByteOffset"]  ==    80 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][11 + 75]["constantByteOffset"]  ==    88 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][12 + 75]["constantByteOffset"]  ==    96 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][13 + 75]["constantByteOffset"]  ==   108 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][14 + 75]["constantByteOffset"]  ==   112 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][15 + 75]["constantByteOffset"]  ==   128 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][16 + 75]["constantByteOffset"]  ==   132 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][17 + 75]["constantByteOffset"]  ==   136 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][18 + 75]["constantByteOffset"]  ==   144 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][19 + 75]["constantByteOffset"]  ==   152 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][20 + 75]["constantByteOffset"]  ==   160 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][21 + 75]["constantByteOffset"]  ==   176 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][22 + 75]["constantByteOffset"]  ==   192 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][23 + 75]["constantByteOffset"]  ==   208 + 2304)
        # Special cases packing tests
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][24 + 75]["constantByteOffset"]  ==   216 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][25 + 75]["constantByteOffset"]  ==   224 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][26 + 75]["constantByteOffset"]  ==   232 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][27 + 75]["constantByteOffset"]  ==   240 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][28 + 75]["constantByteOffset"]  ==   252 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][29 + 75]["constantByteOffset"]  ==   256 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][30 + 75]["constantByteOffset"]  ==   264 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][31 + 75]["constantByteOffset"]  ==   272 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][32 + 75]["constantByteOffset"]  ==   288 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][33 + 75]["constantByteOffset"]  ==   304 + 2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][34 + 75]["constantByteOffset"]  ==   312 + 2304)

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][110]["constantByteOffset"]  ==  2304)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][110]["constantByteSize"]    ==   320) # Complex type. Size & offset match previous entries' sum. Confirmed in Dxc
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForSRGConstants"][110]["typeName"]            ==  "/ExampleSRG/TU")


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingDirectXStride(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-dx12", "--no-alignment-validation", "--srg"])

    if ok:
        predicates = []

        # Column major Constant Buffers
        # Note that the stride here means size. ConstantBuffers should have stride of 0
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["stride"]  ==    32)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["stride"]  ==    48)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][2]["stride"]  ==    36)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][3]["stride"]  ==    64)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][4]["stride"]  ==    40)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][5]["stride"]  ==    52)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][6]["stride"]  ==    68)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][7]["stride"]  ==    56)
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][8]["stride"]  ==    72)

        # Row major Constant Buffers
        # Note that the stride here means size. ConstantBuffers should have stride of 0
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][0]["stride"]  ==    32)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][1]["stride"]  ==    36)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][2]["stride"]  ==    48)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][3]["stride"]  ==    40)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][4]["stride"]  ==    64)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][5]["stride"]  ==    52)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][6]["stride"]  ==    56)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][7]["stride"]  ==    68)
        predicates.append(lambda: j["ShaderResourceGroups"][1]["inputsForBufferViews"][8]["stride"]  ==    72)

        # Column major Structured Buffers
        # Note that the stride is the size of a single element in the buffer. For DirectX packing layout majorness is ignored
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][0]["stride"]  ==    16 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][1]["stride"]  ==    24 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][2]["stride"]  ==    24 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][3]["stride"]  ==    32 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][4]["stride"]  ==    32 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][5]["stride"]  ==    36 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][6]["stride"]  ==    48 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][7]["stride"]  ==    48 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][2]["inputsForBufferViews"][8]["stride"]  ==    64 + 4)

        # Row major Structured Buffers
        # Note that the stride is the size of a single element in the buffer. For DirectX packing layout majorness is ignored
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][0]["stride"]  ==    16 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][1]["stride"]  ==    24 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][2]["stride"]  ==    24 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][3]["stride"]  ==    32 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][4]["stride"]  ==    32 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][5]["stride"]  ==    36 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][6]["stride"]  ==    48 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][7]["stride"]  ==    48 + 4)
        predicates.append(lambda: j["ShaderResourceGroups"][3]["inputsForBufferViews"][8]["stride"]  ==    64 + 4)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingDirectXInlineConstants(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--pack-dx12", "--srg", "--root-const=52"])

    if ok:
        predicates = []

        # Inline constant buffer reflection data validation
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["count"]                    ==    1)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["index"]                    ==    0)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["space"]                    ==    1)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["usage"]                    ==    "Read")
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["sizeInBytes"]              ==    60)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["id"]                       ==    "Root_Constants")

        # Inline constant structure members validation
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["constantByteOffset"]    ==    0)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["constantByteSize"]      ==    16)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["constantId"]            ==    "varFloat4")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["qualifiedName"]         ==    "/Root_Constants/varFloat4")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["typeDimensions"]        ==    [])
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["typeKind"]              ==    "Predefined")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["typeName"]              ==    "?float4")

        # Inline constant structure members validation
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["constantByteOffset"]    ==    16)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["constantByteSize"]      ==    44)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["constantId"]            ==    "mat3x3")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["qualifiedName"]         ==    "/Root_Constants/mat3x3")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["typeDimensions"]        ==    [])
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["typeKind"]              ==    "Predefined")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["typeName"]              ==    "?float3x3")

        if not silent: print (fg.CYAN+ style.BRIGHT+ "inline constant layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyPackingMetalInlineConstants(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--namespace=mt", "--srg"])

    if ok:
        predicates = []

        # Inline constant buffer reflection data validation
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["count"]                    ==    1)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["index"]                    ==    0)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["space"]                    ==    1)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["usage"]                    ==    "Read")
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["sizeInBytes"]              ==    64)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["id"]                       ==    "Root_Constants")

        # Inline constant structure members validation
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["constantByteOffset"]    ==    0)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["constantByteSize"]      ==    16)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["constantId"]            ==    "varFloat4")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["qualifiedName"]         ==    "/Root_Constants/varFloat4")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["typeDimensions"]        ==    [])
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["typeKind"]              ==    "Predefined")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][0]["typeName"]              ==    "?float4")

        # Inline constant structure members validation
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["constantByteOffset"]    ==    16)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["constantByteSize"]      ==    44)
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["constantId"]            ==    "mat3x3")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["qualifiedName"]         ==    "/Root_Constants/mat3x3")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["typeDimensions"]        ==    [])
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["typeKind"]              ==    "Predefined")
        predicates.append(lambda: j["RootConstantBuffer"]["inputsForRootConstants"][1]["typeName"]              ==    "?float3x3")

        if not silent: print (fg.CYAN+ style.BRIGHT+ "inline constant layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False


result = 0  # to define for sub-tests
resultFailed = 0
def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    # Relaxed packing needs to be reviewed
    #if verifyPackingRelaxed(os.path.join(workDir, "srg-layouts.azsl"), compiler, silent) : result += 1
    #else: resultFailed += 1

    if verifyPackingDirectX(os.path.join(workDir, "srg-layouts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingVulkan(os.path.join(workDir, "srg-layouts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyStructsPackingVulkan(os.path.join(workDir, "srg-layouts-structs.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingOpenGL(os.path.join(workDir, "srg-layouts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyStructsPackingOpenGL(os.path.join(workDir, "srg-layouts-structs.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingRelaxedUseSpaces(os.path.join(workDir, "srg-layouts-spaces.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingRelaxedNoSpaces(os.path.join(workDir, "srg-layouts-spaces.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingRelaxedUniqueIdx(os.path.join(workDir, "srg-layouts-spaces.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingRelaxedUniqueIdxUseSpaces(os.path.join(workDir, "srg-layouts-spaces.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1
    
    if verifyPackingUnboundedSpillSpaces(os.path.join(workDir, "srg-layouts-multiple-unbounded-arrays.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingDirectXMatrices(os.path.join(workDir, "srg-layouts-matrices.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingVulkanMatrices(os.path.join(workDir, "srg-layouts-matrices.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingDirectXStride(os.path.join(workDir, "srg-layouts-strides.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingDirectXInlineConstants(os.path.join(workDir, "inline-constant-layouts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyPackingMetalInlineConstants(os.path.join(workDir, "inline-constant-layouts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1
    

if __name__ == "__main__":
    print ("please call from testapp.py")
