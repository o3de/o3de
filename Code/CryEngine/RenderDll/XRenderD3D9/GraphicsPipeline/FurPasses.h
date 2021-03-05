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

#pragma once

class FurPasses
{
public:
    static void InstallInstance();
    static void ReleaseInstance();
    static FurPasses& GetInstance();

    enum class RenderMode
    {
        None,
        AlphaBlended,
        AlphaTested,
    };

    // Returns how fur is set up to render
    RenderMode GetFurRenderingMode();

    // Returns whether the current frame contains render items using fur
    bool IsRenderingFur();

    // Returns the render list that fur render objects should be placed in
    int GetFurRenderList();

    void ExecuteZPostPass();
    void ExecuteObliteratePass();
    void ExecuteFinPass();
    void ExecuteShellPrepass();

    void ApplyFurDebugFlags();

    void SetFurShellPassPercent(float percent);
    float GetFurShellPassPercent();

protected:
    FurPasses();
    ~FurPasses();

private:
    static FurPasses* s_pInstance; // This (and related singleton functions) should be removed when there is a system in place for managing passes.

    static void ZPostRenderFunc();
    static void ObliterateRenderFunc();
    static void FinRenderFunc();

    float m_furShellPassPercent;
};
