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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEBLOCKCOMPRESSOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEBLOCKCOMPRESSOR_H
#pragma once


class IGeomCacheBlockCompressor
{
public:
    virtual ~IGeomCacheBlockCompressor() = default;
    virtual bool Compress(std::vector<char>& input, std::vector<char>& output) = 0;
};

// This just passes the input to the output vector and does no compression
class GeomCacheStoreBlockCompressor
    : public IGeomCacheBlockCompressor
{
    ~GeomCacheStoreBlockCompressor() override = default;
    virtual bool Compress(std::vector<char>& input, std::vector<char>& output) override
    {
        input.swap(output);
        return true;
    }
};

// This is the deflate compressor
class GeomCacheDeflateBlockCompressor
    : public IGeomCacheBlockCompressor
{
    ~GeomCacheDeflateBlockCompressor() override = default;
    virtual bool Compress(std::vector<char>& input, std::vector<char>& output) override;
};

// This is the LZ4 HC compressor
class GeomCacheLZ4HCBlockCompressor
    : public IGeomCacheBlockCompressor
{
    ~GeomCacheLZ4HCBlockCompressor() override = default;
    virtual bool Compress(std::vector<char>& input, std::vector<char>& output) override;
};

// This is the ZStandard compressor
class GeomCacheZStdBlockCompressor
    : public IGeomCacheBlockCompressor
{
    virtual bool Compress(std::vector<char>& input, std::vector<char>& output) override;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEBLOCKCOMPRESSOR_H
