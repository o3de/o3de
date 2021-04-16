// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#include "WirePrimitives.h"
#include <DirectXMath.h>
using namespace DirectX;

#include "Misc/Camera.h"

void GenerateSphere(int sides, std::vector<short> &outIndices, std::vector<float> &outVertices)
{
    int i = 0;

    outIndices.clear();
    outVertices.clear();

    for (int roll = 0; roll < sides; roll++)
    {
        for (int pitch = 0; pitch < sides; pitch++)
        {
            outIndices.push_back(i);
            outIndices.push_back(i + 1);
            outIndices.push_back(i);
            outIndices.push_back(i + 2);
            i += 3;

            XMVECTOR v1 = PolarToVector((roll    ) * (2.0f * XM_PI) / sides, (pitch    ) * (2.0f * XM_PI) / sides);
            XMVECTOR v2 = PolarToVector((roll + 1) * (2.0f * XM_PI) / sides, (pitch    ) * (2.0f * XM_PI) / sides);
            XMVECTOR v3 = PolarToVector((roll    ) * (2.0f * XM_PI) / sides, (pitch + 1) * (2.0f * XM_PI) / sides);

            outVertices.push_back(XMVectorGetX(v1)); outVertices.push_back(XMVectorGetY(v1)); outVertices.push_back(XMVectorGetZ(v1));
            outVertices.push_back(XMVectorGetX(v2)); outVertices.push_back(XMVectorGetY(v2)); outVertices.push_back(XMVectorGetZ(v2));
            outVertices.push_back(XMVectorGetX(v3)); outVertices.push_back(XMVectorGetY(v3)); outVertices.push_back(XMVectorGetZ(v3));
        }
    }
}

void GenerateBox(std::vector<short> &outIndices, std::vector<float> &outVertices)
{
    outIndices.clear();
    outVertices.clear();

    std::vector<short> indices =
    {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4,
        1,5,
        2,6,
        3,7
    };

    std::vector<float> vertices =
    {
        -1,  -1,   1,
        1,  -1,   1,
        1,   1,   1,
        -1,   1,   1,
        -1,  -1,  -1,
        1,  -1,  -1,
        1,   1,  -1,
        -1,   1,  -1,
    };

    outIndices = indices;
    outVertices = vertices;
}
