

//---------------------------------------------------------------------------------------
// Example code that encapsulates three related objects.
// 1.  The TressFXHairObject
// 2.  An interface to get the current set of bones in world space that drive the hair object.
// 3.  An interface to set up for drawing the strands, such as setting lighting parmeters, etc.
// Normally, you'd probably contain the TressFXObject in the engine wrapper, but we've arranged it this 
// way to focus on the important aspects of integration.
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "AMD_TressFX.h"

#include <memory>

class EngineTressFXObject;
class TressFXSimulation;
class EI_Scene;
class EI_Resource;
class EI_CommandContext;
class TressFXHairObject;

class HairStrands
{
public:
    HairStrands(
        EI_Scene *      scene,
        const char *      tfxFilePath,
        const char *      tfxboneFilePath,
        const char *      hairObjectName,
        int               numFollowHairsPerGuideHair,
        float             tipSeparationFactor,
        int               skinNumber,
        int				  renderIndex);

    TressFXHairObject* GetTressFXHandle() { return m_pStrands.get(); }
    void TransitionSimToRendering(EI_CommandContext& context);
    void TransitionRenderingToSim(EI_CommandContext& context);
    void UpdateBones(EI_CommandContext& context);
private:
    std::unique_ptr<TressFXHairObject> m_pStrands;
    EI_Scene * m_pScene = nullptr;
    int m_skinNumber;
};


