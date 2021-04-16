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

#include "../Misc/Misc.h"

#include <map>

//
// Hash a string of source code and recurse over its #include files
//
size_t HashShaderString(const char *pRootDir, const char *pShader, size_t result = 2166136261);

//
// DefineList, holds pairs of key & value that will be used by the compiler as defines
//
class DefineList : public std::map<std::string, std::string>
{
public:
    bool Has(const std::string &str)
    {
        return find(str) != end();
    }

    size_t Hash(size_t result = 2166136261) const
    {
        for (auto it = begin(); it != end(); it++)
        {
            result = ::Hash(it->first.c_str(), it->first.size(), result);
            result = ::Hash(it->second.c_str(), it->second.size(), result);
        }
        return result;
    }

    friend DefineList operator+(      DefineList   def1,        // passing lhs by value helps optimize chained a+b+c
                                const DefineList & def2) // otherwise, both parameters may be const references
    {
        for (auto it = def2.begin(); it != def2.end(); it++)
            def1[it->first] = it->second;    
        return def1; 
    }
};