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

// Copyright (c) 2009, Tom Lokovic
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


using System;
using System.Collections.Generic;

namespace Midi
{
    /// <summary>
    /// Utility functions for encoding and decoding short messages.
    /// </summary>
    static class ShortMsg
    {
        /// <summary>
        /// Decodes a Note On short message.
        /// </summary>
        public static void DecodeMsg(UIntPtr dwParam1, UIntPtr dwParam2,
            out int channel, out int mode, out int code, out int velocity, out UInt32 timestamp)
        {
            channel = (int)dwParam1 & 0x0f;
            mode = ((int)dwParam1 & 0xf0) >> 4; 
            code = ((int)dwParam1 & 0xff00) >> 8;
            velocity = ((int)dwParam1 & 0xff0000) >> 16;
            timestamp = (UInt32)dwParam2;
        }

        /// <summary>
        /// Encodes a Note On short message.
        /// </summary>
        public static UInt32 EncodeMsg(int channel, int code, int velocity)
        {
            return (UInt32)(0x90 | (channel) | (code << 8) | (velocity << 16));
        }
    }
}
