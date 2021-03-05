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

// forward declare
class ICompressor;

// Common helper methods needed for both TLS and DTLS transport implementations
namespace AzNetworking
{
    //! Helper function to create a compressor, uses enabled Gems that supply compressors
    //! @param compressorName The string name of the compressor type to create, this is Crc32'd to select by CompressorType
    //! @return A unique_ptr to a Compressor implementation or nullptr on failure to match
    AZStd::unique_ptr<ICompressor> CreateCompressor(AZStd::string_view compressorName);
}
