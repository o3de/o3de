/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/ShaderUtils.h>
#include <RHI/DX12.h>
#include <openssl/md5.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>

namespace AZ::DX12
{
    static const uint32_t FOURCC_DXBC = MAKE_FOURCC('D', 'X', 'B', 'C');

    // Modify the bits in the bytecode with a new value following the VBR rules of encoding
    uint64_t TamperBits(uint8_t* byteCode, uint32_t patchVal, uint64_t offset)
    {
        constexpr uint32_t BitsPerByte = 8;
        // LSB is used for encoding signed/unsigned
        // VBR left shift all values to leave space for the sign bit
        patchVal <<= 1;

        uint64_t original = 0;
        uint64_t currentOffset = offset;

        auto SetBitFunc = [&](uint64_t pos, bool value)
        {
            if (value)
            {
                byteCode[pos / BitsPerByte] |= (uint8_t)1 << pos % BitsPerByte;
            }
            else
            {
                byteCode[pos / BitsPerByte] &= ~((uint8_t)1 << pos % BitsPerByte);
            }
        };
        uint32_t originalValueBitIndex = 0;
        // 32 bits take 5 full bytes in VBR.
        const uint32_t sentinelBytes = 5;
        for (uint32_t i = 0; i < sentinelBytes; i++)
        {
            // Patch all bits except the continuation bit
            for (uint32_t j = 0; j < (BitsPerByte - 1); j++)
            {
                bool currentValue = ((byteCode[currentOffset / BitsPerByte] & ((uint8_t)1 << currentOffset % BitsPerByte)) != 0);
                bool newValue = (patchVal & (uint64_t)1) != 0;
                SetBitFunc(currentOffset, newValue);
                original |= (uint64_t)currentValue << originalValueBitIndex;
                patchVal >>= 1;
                currentOffset++;
                originalValueBitIndex++;
            }
            // Set continuation bit
            SetBitFunc(currentOffset, true);
            currentOffset++;
        }
        // MSB in VBR doesn't have a continuation bit (because it's the last byte)
        SetBitFunc(currentOffset - 1, false);
        // VBR left shift values for sign bit, so we right shift the value we found
        return original >> 1;
    }

    ShaderByteCode ShaderUtils::PatchShaderFunction(
        const ShaderStageFunction& shaderFunction, const RHI::PipelineStateDescriptor& descriptor)
    {
        ShaderByteCode patched(shaderFunction.GetByteCode().size());
        ::memcpy(patched.data(), shaderFunction.GetByteCode().data(), patched.size());
        const AZStd::vector<RHI::SpecializationConstant>& specializationConstants = descriptor.m_specializationData;
        for (const auto& element : shaderFunction.GetSpecializationOffsets())
        {
            auto findIter = AZStd::find_if(
                specializationConstants.begin(),
                specializationConstants.end(),
                [&](const RHI::SpecializationConstant& constantData)
                {
                    return constantData.m_id == element.first;
                });

            if (findIter == specializationConstants.end())
            {
                AZ_Error("ShaderUtils", false, "Specialization constant %d doesn't not have a value", element.first);
                continue;
            }

            [[maybe_unused]] uint64_t sentinelFound = TamperBits(patched.data(), findIter->m_value.GetIndex(), element.second);
            AZ_Assert(
                static_cast<uint32_t>(sentinelFound & SCSentinelMask) == SCSentinelValue,
                "Invalid sentinel value found %lu",
                sentinelFound);
        }

        // Re-sign the shader bytecode after we patch it
        if (!SignByteCode(patched))
        {
            AZ_Error("ShaderUtils", false, "Failed to sign container");
            return {};
        }

        return patched;
    }

    ShaderByteCodeView ShaderUtils::PatchShaderFunction(
        const ShaderStageFunction& shaderFunction,
        const RHI::PipelineStateDescriptor& descriptor,
        AZStd::vector<ShaderByteCode>& patchedShaderContainer)
    {
        if (!shaderFunction.UseSpecializationConstants())
        {
            // No need to patch anything
            return shaderFunction.GetByteCode();
        }

        ShaderByteCode patchedShader = PatchShaderFunction(shaderFunction, descriptor);
        patchedShaderContainer.emplace_back(AZStd::move(patchedShader));
        return patchedShaderContainer.back();
    }

    bool ShaderUtils::SignByteCode(ShaderByteCode& bytecode)
    {
        // Original signing code from Renderdoc
        struct FileHeader
        {
            uint32_t fourcc; // "DXBC"
            uint32_t hashValue[4]; // unknown hash function and data
            uint32_t containerVersion;
            uint32_t fileLength;
            uint32_t numChunks;
            // uint32 chunkOffsets[numChunks]; follows
        };

        if (bytecode.size() < sizeof(FileHeader) || bytecode.data() == nullptr)
        {
            return false;
        }

        FileHeader* header = reinterpret_cast<FileHeader*>(bytecode.data());

        if (header->fourcc != FOURCC_DXBC)
        {
            return false;
        }

        if (header->fileLength != static_cast<uint32_t>(bytecode.size()))
        {
            return false;
        }

        _MD5_CTX md5ctx = {};
        _MD5_Init(&md5ctx);

        // the hashable data starts immediately after the hash.
        AZStd::byte* data = reinterpret_cast<AZStd::byte*>(&header->containerVersion);
        uint32_t length = uint32_t(bytecode.size() - offsetof(FileHeader, containerVersion));

        // we need to know the number of bits for putting in the trailing padding.
        uint32_t numBits = length * 8;
        uint32_t numBitsPart2 = (numBits >> 2) | 1;

        // MD5 works on 64-byte chunks, process the first set of whole chunks, leaving 0-63 bytes left
        // over
        uint32_t leftoverLength = length % 64;
        _MD5_Update(&md5ctx, data, length - leftoverLength);

        data += length - leftoverLength;

        uint32_t block[16] = {};
        AZ_Assert(sizeof(block) == 64, "Block is not properly sized for MD5 round");

        // normally MD5 finishes by appending a 1 bit to the bitstring. Since we are only appending bytes
        // this would be an 0x80 byte (the first bit is considered to be the MSB). Then it pads out with
        // zeroes until it has 56 bytes in the last block and appends appends the message length as a
        // 64-bit integer as the final part of that block.
        // in other words, normally whatever is leftover from the actual message gets one byte appended,
        // then if there's at least 8 bytes left we'll append the length. Otherwise we pad that block with
        // 0s and create a new block with the length at the end.
        // Or as the original RFC/spec says: padding is always performed regardless of whether the
        // original buffer already ended in exactly a 56 byte block.
        //
        // The DXBC finalisation is slightly different (previous work suggests this is due to a bug in the
        // original implementation and it was maybe intended to be exactly MD5?):
        //
        // The length provided in the padding block is not 64-bit properly: the second dword with the high
        // bits is instead the number of nybbles(?) with 1 OR'd on. The length is also split, so if it's
        // in
        // a padding block the low bits are in the first dword and the upper bits in the last. If there's
        // no padding block the low dword is passed in first before the leftovers of the message and then
        // the upper bits at the end.

        // if the leftovers uses at least 56, we can't fit both the trailing 1 and the 64-bit length, so
        // we need a padding block and then our own block for the length.
        if (leftoverLength >= 56)
        {
            // pass in the leftover data padded out to 64 bytes with zeroes
            _MD5_Update(&md5ctx, data, leftoverLength);

            block[0] = 0x80; // first padding bit is 1
            _MD5_Update(&md5ctx, block, 64 - leftoverLength);

            // the final block contains the number of bits in the first dword, and the weird upper bits
            block[0] = numBits;
            block[15] = numBitsPart2;

            // process this block directly, we're replacing the call to MD5_Final here manually
            _MD5_Update(&md5ctx, block, 64);
        }
        else
        {
            // the leftovers mean we can put the padding inside the final block. But first we pass the "low"
            // number of bits:
            _MD5_Update(&md5ctx, &numBits, sizeof(numBits));

            if (leftoverLength)
            {
                _MD5_Update(&md5ctx, data, leftoverLength);
            }

            uint32_t paddingBytes = 64 - leftoverLength - 4;

            // prepare the remainder of this block, starting with the 0x80 padding start right after the
            // leftovers and the first part of the bit length above.
            block[0] = 0x80;
            // then add the remainder of the 'length' here in the final part of the block
            ::memcpy(((AZStd::byte*)block) + paddingBytes - 4, &numBitsPart2, 4);

            _MD5_Update(&md5ctx, block, paddingBytes);
        }

        header->hashValue[0] = md5ctx.a;
        header->hashValue[1] = md5ctx.b;
        header->hashValue[2] = md5ctx.c;
        header->hashValue[3] = md5ctx.d;

        return true;
    }
} // namespace AZ
