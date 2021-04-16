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
#include "ShaderCompiler.h"
#include "../Misc/Misc.h"

//
// Hash a string of source code and recurse over its #include files
//
size_t HashShaderString(const char *pRootDir, const char *pShader, size_t hash)
{
    hash = Hash(pShader, strlen(pShader), hash);

    const char *pch = pShader;
    while (*pch != 0)
    {        
        if (*pch == '/') // parse comments
        {            
            pch++;
            if (*pch != 0 && *pch == '/')
            {
                pch++;
                while (*pch != 0 && *pch != '\n')
                    pch++;
            }
            else if (*pch != 0 && *pch == '*')
            {
                pch++;
                while ( (*pch != 0 && *pch != '*') && (*(pch+1) != 0 && *(pch+1) != '/'))
                    pch++;
            }
        }
        else if (*pch == '#') // parse #include
        {
            *pch++;
            const char include[] = "include";
            int i = 0;
            while ((*pch!= 0) && *pch == include[i])
            {
                pch++;
                i++;
            }

            if (i == strlen(include))
            {
                while (*pch != 0 && *pch == ' ')
                    pch++;
                
                if (*pch != 0 && *pch == '\"')
                {
                    pch++;
                    const char *pName = pch;
                    
                    while (*pch != 0 && *pch != '\"')
                        pch++;

                    char includeName[1024];
                    strcpy_s<1024>(includeName, pRootDir);
                    strncat_s<1024>(includeName, pName, pch - pName);

                    pch++;

                    char *pShaderCode = NULL;
                    if (ReadFile(includeName, &pShaderCode, NULL, false))
                    {
                        hash = HashShaderString(pRootDir, pShaderCode, hash);
                        free(pShaderCode);
                    }
                }
            }            
        }
        else
        {
            pch++;
        }
    }

    return hash;
}