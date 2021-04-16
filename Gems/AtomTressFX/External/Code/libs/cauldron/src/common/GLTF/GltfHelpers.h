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

#pragma once
#include "../json/json.h"

using json = nlohmann::json;

int GetFormatSize(int id);
int GetDimensions(const std::string &str);
void SplitGltfAttribute(std::string attribute, std::string *semanticName, uint32_t *semanticIndex);

XMVECTOR GetVector(const json::array_t &accessor);
XMMATRIX GetMatrix(const json::array_t &accessor);
std::string GetElementString(const json::object_t &root, char *path, std::string pDefault);
float GetElementFloat(const json::object_t &root, char *path, float pDefault);
int GetElementInt(const json::object_t &root, char *path, int pDefault);
bool GetElementBoolean(const json::object_t &root, char *path, bool pDefault);
json::array_t GetElementJsonArray(const json::object_t &root, char *path, json::array_t pDefault);
XMVECTOR GetElementVector(json::object_t &root, char *path, XMVECTOR default);

