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
package com.amazon.lumberyard.input;


import android.app.Activity;
import android.content.Context;
import android.hardware.input.InputManager;
import android.view.InputDevice;

import java.util.HashSet;
import java.util.Set;


public class MouseDevice
    implements InputManager.InputDeviceListener
{
    public native void OnMouseConnected();
    public native void OnMouseDisconnected();


    public MouseDevice(Activity activity)
    {
        m_inputManager = (InputManager)activity.getSystemService(Context.INPUT_SERVICE);

        int[] devices = m_inputManager.getInputDeviceIds();
        for (int deviceId : devices)
        {
            if (IsMouseDevice(deviceId))
            {
                m_mouseDeviceIds.add(deviceId);
            }
        }

        final InputManager.InputDeviceListener listener = this;
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // run the registration on the main thread to use it's looper as the handler
                // instead of creating one specifically for listening to mouse [dis]connections
                m_inputManager.registerInputDeviceListener(listener, null);
            }
        });
    }

    @Override
    public void onInputDeviceAdded(int deviceId)
    {
        if (IsMouseDevice(deviceId))
        {
            m_mouseDeviceIds.add(deviceId);

            // only inform the native code if we change from having no mice connected, extra
            // are effectively ignored and folded into one "master" device
            if (m_mouseDeviceIds.size() == 1)
            {
                OnMouseConnected();
            }
        }
    }

    @Override
    public void onInputDeviceChanged(int deviceId)
    {
        // do nothing
    }

    @Override
    public void onInputDeviceRemoved(int deviceId)
    {
        if (m_mouseDeviceIds.contains(deviceId))
        {
            m_mouseDeviceIds.remove(deviceId);

            // only inform the native code if we change to having no mice connected
            if (m_mouseDeviceIds.size() == 0)
            {
                OnMouseDisconnected();
            }
        }
    }

    public boolean IsConnected()
    {
        return (m_mouseDeviceIds.size() > 0);
    }


    private boolean IsMouseDevice(int deviceId)
    {
        InputDevice device = m_inputManager.getInputDevice(deviceId);
        if (device == null)
        {
            return false;
        }
        int sources = device.getSources();
        return (sources == InputDevice.SOURCE_MOUSE);
    }


    private InputManager m_inputManager = null;
    private Set<Integer> m_mouseDeviceIds = new HashSet<>();
}
