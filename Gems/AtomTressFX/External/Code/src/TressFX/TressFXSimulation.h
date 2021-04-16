// ----------------------------------------------------------------------------
// Invokes simulation compute shaders.
// ----------------------------------------------------------------------------
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

#ifndef TRESSFXSIMULATION_H_
#define TRESSFXSIMULATION_H_

#include "TressFXCommon.h"
#include "EngineInterface.h"

class TressFXHairObject;

class TressFXSimulation : private TressFXNonCopyable
{
public:
    void Initialize(EI_Device* pDevice);
    void Simulate(EI_CommandContext& commandContext, std::vector<TressFXHairObject*>& hairObjects);
    void UpdateHairSkinning(EI_CommandContext& commandContext, std::vector<TressFXHairObject*>& hairObjects);

private:
    // Maybe these just need to be compute shader references.
    std::unique_ptr<EI_PSO> mVelocityShockPropagationPSO;
    std::unique_ptr<EI_PSO> mIntegrationAndGlobalShapeConstraintsPSO;
    std::unique_ptr<EI_PSO> mCalculateStrandLevelDataPSO;
    std::unique_ptr<EI_PSO> mLocalShapeConstraintsPSO;
    std::unique_ptr<EI_PSO> mLengthConstriantsWindAndCollisionPSO;
    std::unique_ptr<EI_PSO> mUpdateFollowHairVerticesPSO;    
    
    // Adi: only skin the vertices - no physics is applied
    std::unique_ptr<EI_PSO> mSkinHairVerticesTestPSO;
};

#endif  // TRESSFXSIMULATION_H_