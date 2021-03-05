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
using System.Collections.ObjectModel;
using System.Text;

namespace Midi
{
    /// <summary>
    /// A MIDI input device.
    /// </summary>
    public class InputDevice : DeviceBase
    {
        #region Delegates

        /// <summary>
        /// Delegate called when an input device receives a midi message.
        /// </summary>
        public delegate void MidiHandler(MidiMessage msg);

        #endregion

        #region Events

        /// <summary>
        /// Event called when an input device receives a midi message.
        /// </summary>
        public event MidiHandler MidiTrigger;

        /// <summary>
        /// Removes all event handlers from the input events on this device.
        /// </summary>
        public void RemoveAllEventHandlers()
        {
            MidiTrigger = null;
        }

        #endregion

        #region Public Methods and Properties

        /// <summary>
        /// List of input devices installed on this system.
        /// </summary>
        public static ReadOnlyCollection<InputDevice> InstalledDevices
        {
            get
            {
                lock (staticLock)
                {
                    if (installedDevices == null)
                    {
                        installedDevices = MakeDeviceList();
                    }
                    return new ReadOnlyCollection<InputDevice>(installedDevices);
                }
            }
        }

        /// <summary>
        /// True if this device has been successfully opened.
        /// </summary>
        public bool IsOpen
        {
            get
            {
                if (isInsideInputHandler)
                {
                    return true;
                }
                lock (this)
                {
                    return isOpen;
                }
            }
        }

        /// <summary>
        /// Opens this input device.
        /// </summary>
        /// <exception cref="InvalidOperationException">The device is already open.</exception>
        /// <exception cref="DeviceException">The device cannot be opened.</exception>
        /// <remarks>Note that Open() establishes a connection to the device, but no messages will
        /// be received until <see cref="StartReceiving"/> is called.</remarks>
        public void Open()
        {
            if (isInsideInputHandler)
            {
                throw new InvalidOperationException("Device is open.");
            }
            lock (this)
            {
                CheckNotOpen();
                CheckReturnCode(Win32API.midiInOpen(out handle, deviceId,
                    inputCallbackDelegate, (UIntPtr)0));
                isOpen = true;
            }
        }

        /// <summary>
        /// Closes this input device.
        /// </summary>
        /// <exception cref="InvalidOperationException">The device is not open or is still
        /// receiving.</exception>
        /// <exception cref="DeviceException">The device cannot be closed.</exception>
        public void Close()
        {
            if (isInsideInputHandler)
            {
                throw new InvalidOperationException("Device is receiving.");
            }
            lock (this)
            {
                CheckOpen();
                CheckReturnCode(Win32API.midiInClose(handle));
                isOpen = false;
            }
        }

        /// <summary>
        /// True if this device is receiving messages.
        /// </summary>
        public bool IsReceiving
        {
            get
            {
                if (isInsideInputHandler)
                {
                    return true;
                }
                lock (this)
                {
                    return isReceiving;
                }
            }
        }

        /// <summary>
        /// Starts this input device receiving messages.
        /// </summary>
        public void StartReceiving()//Clock clock)
        {
            if (isInsideInputHandler)
            {
                throw new InvalidOperationException("Device is receiving.");
            }
            lock (this)
            {
                CheckOpen();
                CheckNotReceiving();
                CheckReturnCode(Win32API.midiInStart(handle));
                isReceiving = true;
                //this.clock = clock;
            }
        }
        
        /// <summary>
        /// Stops this input device from receiving messages.
        /// </summary>
        /// <remarks>
        /// <para>This method waits for all in-progress input event handlers to finish, and then
        /// joins (shuts down) the background thread that was created in
        /// <see cref="StartReceiving"/>.  Thus, when this function returns you can be sure that no
        /// more event handlers will be invoked.</para>
        /// <para>It is illegal to call this method from an input event handler (ie, from the
        /// background thread), and doing so throws an exception. If an event handler really needs
        /// to call this method, consider using BeginInvoke to schedule it on another thread.</para>
        /// </remarks>
        /// <exception cref="InvalidOperationException">The device is not open; is not receiving;
        /// or called from within an event handler (ie, from the background thread).</exception>
        /// <exception cref="DeviceException">The device cannot start receiving.</exception>
        public void StopReceiving()
        {
            if (isInsideInputHandler)
            {
                throw new InvalidOperationException(
                    "Can't call StopReceiving() from inside an input handler.");
            }
            lock (this)
            {
                CheckReceiving();
                CheckReturnCode(Win32API.midiInStop(handle));
                //clock = null;
                isReceiving = false;
            }
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Makes sure rc is MidiWin32Wrapper.MMSYSERR_NOERROR.  If not, throws an exception with an
        /// appropriate error message.
        /// </summary>
        /// <param name="rc"></param>
        private static void CheckReturnCode(Win32API.MMRESULT rc)
        {
            if (rc != Win32API.MMRESULT.MMSYSERR_NOERROR)
            {
                StringBuilder errorMsg = new StringBuilder(128);
                rc = Win32API.midiInGetErrorText(rc, errorMsg);
                if (rc != Win32API.MMRESULT.MMSYSERR_NOERROR)
                {
                    throw new DeviceException("no error details");
                }
                throw new DeviceException(errorMsg.ToString());
            }
        }

        /// <summary>
        /// Throws a MidiDeviceException if this device is not open.
        /// </summary>
        private void CheckOpen()
        {
            if (!isOpen)
            {
                throw new InvalidOperationException("Device is not open.");
            }
        }

        /// <summary>
        /// Throws a MidiDeviceException if this device is open.
        /// </summary>
        private void CheckNotOpen()
        {
            if (isOpen)
            {
                throw new InvalidOperationException("Device is open.");
            }
        }

        /// <summary>
        /// Throws a MidiDeviceException if this device is not receiving.
        /// </summary>
        private void CheckReceiving()
        {
            if (!isReceiving)
            {
                throw new DeviceException("device not receiving");
            }
        }

        /// <summary>
        /// Throws a MidiDeviceException if this device is receiving.
        /// </summary>
        private void CheckNotReceiving()
        {
            if (isReceiving)
            {
                throw new DeviceException("device receiving");
            }
        }

        /// <summary>
        /// Private Constructor, only called by the getter for the InstalledDevices property.
        /// </summary>
        /// <param name="deviceId">Position of this device in the list of all devices.</param>
        /// <param name="caps">Win32 Struct with device metadata</param>
        private InputDevice(UIntPtr deviceId, Win32API.MIDIINCAPS caps)
            : base(caps.szPname)
        {
            this.deviceId = deviceId;
            this.caps = caps;
            this.inputCallbackDelegate = new Win32API.MidiInProc(this.InputCallback);
            this.isOpen = false;
            //this.clock = null;
        }

        /// <summary>
        /// Private method for constructing the array of MidiInputDevices by calling the Win32 api.
        /// </summary>
        /// <returns></returns>
        private static InputDevice[] MakeDeviceList()
        {
			#if __MonoCS__
			return new InputDevice[0];
			#else
            uint inDevs = Win32API.midiInGetNumDevs();
            InputDevice[] result = new InputDevice[inDevs];
            for (uint deviceId = 0; deviceId < inDevs; deviceId++)
            {
                Win32API.MIDIINCAPS caps = new Win32API.MIDIINCAPS();
                Win32API.midiInGetDevCaps((UIntPtr)deviceId, out caps);
                result[deviceId] = new InputDevice((UIntPtr)deviceId, caps);
            }
            return result;
			#endif
        }

        /// <summary>
        /// The input callback for midiOutOpen.
        /// </summary>
        private void InputCallback(Win32API.HMIDIIN hMidiIn, Win32API.MidiInMessage wMsg,
            UIntPtr dwInstance, UIntPtr dwParam1, UIntPtr dwParam2)
        {
            isInsideInputHandler = true;
            try
            {
                if (wMsg == Win32API.MidiInMessage.MIM_DATA)
                {
                    int channel;
                    int mode; 
                    int code;
                    int velocity;
                    UInt32 win32Timestamp;
                    
                    if (MidiTrigger != null)
                    {
                        ShortMsg.DecodeMsg(dwParam1, dwParam2, out channel, out mode, out code, out velocity, out win32Timestamp);
                        MidiTrigger(new MidiMessage(this, channel, mode, code, velocity));
                    }
                }
            }
            finally
            {
                isInsideInputHandler = false;
            }
        }

        #endregion

        #region Private Fields

        // Access to the global state is guarded by lock(staticLock).
        private static Object staticLock = new Object();
        private static InputDevice[] installedDevices = null;

        // These fields initialized in the constructor never change after construction,
        // so they don't need to be guarded by a lock.  We keep a reference to the
        // callback delegate because we pass it to unmanaged code (midiInOpen) and unmanaged code
        // cannot prevent the garbage collector from collecting the delegate.
        private UIntPtr deviceId;
        private Win32API.MIDIINCAPS caps;
        private Win32API.MidiInProc inputCallbackDelegate;

        // Access to the Open/Close state is guarded by lock(this).
        private bool isOpen;
        private bool isReceiving;
        //private Clock clock;
        private Win32API.HMIDIIN handle;

        /// <summary>
        /// Thread-local, set to true when called by an input handler, false in all other threads.
        /// </summary>
        [ThreadStatic]
        static bool isInsideInputHandler = false;

        #endregion
    }
}
