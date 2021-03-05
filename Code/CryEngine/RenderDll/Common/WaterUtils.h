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

#ifndef __WATERUTILS_H__
#define __WATERUTILS_H__

class CWaterSim;

// todo. refactor me - should be more general

class CWater
{
public:

    CWater()
        : m_pWaterSim(0)
    {
    }

    ~CWater()
    {
        Release();
    }

    // Create/Initialize simulation
    void Create(float fA, float fWorldSizeX, float fWorldSizeY);
    void Release();
    void SaveToDisk(const char* pszFileName);

    // Update water simulation
    void Update(int nFrameID, float fTime, bool bOnlyHeight = false, void* pRawPtr = NULL);

    // Get water simulation data
    Vec3 GetPositionAt(int x, int y) const;
    float GetHeightAt(int x, int y) const;
    Vec4* GetDisplaceGrid() const;

    const int GetGridSize() const;

    void GetMemoryUsage(ICrySizer* pSizer) const;

    bool NeedInit() const { return m_pWaterSim == NULL; }
private:

    CWaterSim* m_pWaterSim;
};

CWater* WaterSimMgr();

#endif
