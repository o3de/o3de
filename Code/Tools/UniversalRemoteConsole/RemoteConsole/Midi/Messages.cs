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

namespace Midi
{    
    /// <summary>
    /// Base class for messages relevant to a specific device.
    /// </summary>
    public abstract class DeviceMessage 
    {
        /// <summary>
        /// Protected constructor.
        /// </summary>
        protected DeviceMessage(DeviceBase device)//, float time) : base(time)
        {
            if (device == null)
            {
                throw new ArgumentNullException("device");
            }
						Device = device;                    
        }

        /// <summary>
        /// The device from which this message originated, or for which it is destined.
        /// </summary>
        public DeviceBase Device { get; private set; }
    }

    /// <summary>
    /// Note On message.
    /// </summary>
    public class MidiMessage : DeviceMessage
    {
        /// <summary>
        /// Constructs a Midi message.
        /// </summary>
        public MidiMessage(DeviceBase device, int channel, int mode, int code, int velocity)
         :base(device)
        {
					Channel = channel; 
          Mode = mode; 
          Code = code; 
          Velocity = velocity;
        }

        /// <summary>
        /// Channel.
        /// </summary>
				public int Channel { get; private set; }

        /// <summary>
        /// Mode.
        /// </summary>
				public int Mode { get; private set; }

        /// <summary>
        /// Code.
        /// </summary>
				public int Code { get; private set; }

        /// <summary>
        /// Velocity.
        /// </summary>
				public int Velocity { get; private set; }
    }
}
