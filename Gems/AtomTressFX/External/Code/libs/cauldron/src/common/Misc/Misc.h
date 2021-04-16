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

#include <string>

double MillisecondsNow();
size_t Hash(const void *ptr, size_t size, size_t result = 2166136261);
std::string format(const char* format, ...);
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary);
bool SaveFile(const char *name, void const*data, size_t size, bool isbinary);
void Trace(const std::string &str);
void Trace(const char* pFormat, ...);
bool LaunchProcess(const std::string &commandLine, const std::string &filenameErr);
void GetXYZ(float *, XMVECTOR v);

// align uLocation to the next multiple of uAlign
inline SIZE_T AlignOffset(SIZE_T uOffset, SIZE_T uAlign) { return ((uOffset + (uAlign - 1)) & ~(uAlign - 1)); }

class Profile
{
    double m_time;
    const char *m_label;
public:
    Profile(const char *label) {
        m_time = MillisecondsNow(); m_label = label;
    }
    ~Profile() {
        Trace(format("*** %s  %f ms\n", m_label, (MillisecondsNow() - m_time)));
    }
};


