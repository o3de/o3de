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

#include "stdafx.h"
#include "glTFHelpers.h"

int GetFormatSize(int id)
{
    switch (id)
    {
        case 5120: return 1; //(BYTE)
        case 5121: return 1; //(UNSIGNED_BYTE)1
        case 5122: return 2; //(SHORT)2
        case 5123: return 2; //(UNSIGNED_SHORT)2
        case 5124: return 4; //(SIGNED_INT)4
        case 5125: return 4; //(UNSIGNED_INT)4
        case 5126: return 4; //(FLOAT)
    }
    return -1;
}

int GetDimensions(const std::string &str)
{
    if (str == "SCALAR")    return  1;
    else if (str == "VEC2") return  2;
    else if (str == "VEC3") return  3;
    else if (str == "VEC4") return  4;
    else if (str == "MAT4") return  4*4;
    else return  -1;
}

void SplitGltfAttribute(std::string attribute, std::string *semanticName, uint32_t *semanticIndex)
{
    *semanticIndex = 0;

    if (isdigit(attribute.back()))
    {
        *semanticIndex = attribute.back() - '0';

        attribute.pop_back();
        if (attribute.back() == '_')
            attribute.pop_back();
    }

    *semanticName = attribute;
}

XMVECTOR GetVector(const json::array_t &accessor)
{
    return XMVectorSet(accessor[0], accessor[1], accessor[2], (accessor.size() == 4) ? accessor[3] : 0);
}

XMMATRIX GetMatrix(const json::array_t &accessor)
{
    return XMMatrixSet(accessor[0], accessor[1], accessor[2], accessor[3],
        accessor[4], accessor[5], accessor[6], accessor[7],
        accessor[8], accessor[9], accessor[10], accessor[11],
        accessor[12], accessor[13], accessor[14], accessor[15]);
}

template <class type>
type GetElement(const json::object_t *pRoot, char *path, type pDefault)
{
    char *p = path;
    char token[128];
    while (true)
    {
        for (; *p != '/' && *p != 0 && *p != '['; p++);
        memcpy(token, path, p - path);
        token[p - path] = 0;

        auto it = pRoot->find(token);
        if (it == pRoot->end())
            return pDefault;

        if (*p == '[')
        {
            p++;
            int i = atoi(p);
            for (; *p != 0 && *p != ']'; p++);
            pRoot = it->second.at(i).get_ptr<const json::object_t *>();
            p++;
        }
        else
        {
            if (it->second.is_object())
                pRoot = it->second.get_ptr<const json::object_t *>();
            else
            {
                return it->second.get<type>();
            }
        }
        p++;
        path = p;
    }

    return pDefault;
}

std::string GetElementString(const json::object_t &root, char *path, std::string pDefault)
{
    return GetElement<std::string>(&root, path, pDefault);
}

bool GetElementBoolean(const json::object_t &root, char *path, bool default)
{
    return GetElement<bool>(&root, path, default);
}

float GetElementFloat(const json::object_t &root, char *path, float default)
{
    return GetElement<float>(&root, path, default);
}

int GetElementInt(const json::object_t &root, char *path, int default)
{
    return GetElement<int>(&root, path, default);
}

json::array_t GetElementJsonArray(const json::object_t &root, char *path, json::array_t default)
{
    return GetElement<json::array_t>(&root, path, default);
}

XMVECTOR GetElementVector(json::object_t &root, char *path, XMVECTOR default)
{
    if (root.find(path) != root.end() && !root[path].is_null())
    {
        return GetVector(root[path].get<json::array_t>());
    }
    else
        return default;
}

