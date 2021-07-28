/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
package com.amazon.lumberyard.input;


import android.app.Activity;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.view.Display;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import java.lang.Math;


////////////////////////////////////////////////////////////////////////////////////////////////////
public class MotionSensorManager implements SensorEventListener
{
    private class MotionSensorData
    {
        private class SensorData3D
        {
            private float x;
            private float y;
            private float z;
            private boolean updated;
        }

        private class SensorData4D
        {
            private float x;
            private float y;
            private float z;
            private float w;
            private boolean updated;
        }

        private SensorData3D accelerationRaw = new SensorData3D();
        private SensorData3D accelerationUser = new SensorData3D();
        private SensorData3D accelerationGravity = new SensorData3D();
        private SensorData3D rotationRateRaw = new SensorData3D();
        private SensorData3D rotationRateUnbiased = new SensorData3D();
        private SensorData3D magneticFieldRaw = new SensorData3D();
        private SensorData3D magneticFieldUnbiased = new SensorData3D();
        private SensorData4D orientation = new SensorData4D();
    }

    private static final float METRES_PER_SECOND_SQUARED_TO_GFORCE = -1.0f / SensorManager.GRAVITY_EARTH;
    private static final int MOTION_SENSOR_DATA_PACKED_LENGTH = 34;
    private MotionSensorData m_motionSensorData = new MotionSensorData();
    private float[] m_motionSensorDataPacked = new float[MOTION_SENSOR_DATA_PACKED_LENGTH];
    private SensorManager m_sensorManager = null;
    private Display m_defaultDisplay = null;
    private float m_orientationAdjustmentRadiansZ = 0.0f;
    private int m_orientationSensorToUse = Sensor.TYPE_GAME_ROTATION_VECTOR;

    public MotionSensorManager(Activity activity)
    {
        m_sensorManager = (SensorManager)activity.getSystemService(Context.SENSOR_SERVICE);
        m_defaultDisplay = ((WindowManager)activity.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();

        // If the game rotation vector is not available, default to the regular rotation vector.
        if ((m_sensorManager != null) && (m_sensorManager.getDefaultSensor(m_orientationSensorToUse) == null))
        {
            m_orientationSensorToUse = Sensor.TYPE_ROTATION_VECTOR;
        }
    }

    // Called when a motion sensor's accuracy changes
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
    }

    // Called when a motion sensor event is dispatched
    @Override
    public void onSensorChanged(SensorEvent event)
    {
        int currentDisplayRotation = m_defaultDisplay != null ?
                                     m_defaultDisplay.getRotation() :
                                     Surface.ROTATION_0;

        Sensor sensor = event.sensor;
        switch (sensor.getType())
        {
            case Sensor.TYPE_ACCELEROMETER:
            {
                // Convert to the same unit of measurement as returned natively by iOS,
                // which is (arguably) more useful to use directly for game development.
                AlignWithDisplay(m_motionSensorData.accelerationRaw,
                                 event.values[0] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 event.values[1] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 event.values[2] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_LINEAR_ACCELERATION:
            {
                // Convert to the same unit of measurement as returned natively by iOS,
                // which is (arguably) more useful to use directly for game development.
                AlignWithDisplay(m_motionSensorData.accelerationUser,
                                 event.values[0] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 event.values[1] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 event.values[2] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_GRAVITY:
            {
                // Convert to the same unit of measurement as returned natively by iOS,
                // which is (arguably) more useful to use directly for game development.
                AlignWithDisplay(m_motionSensorData.accelerationGravity,
                                 event.values[0] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 event.values[1] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 event.values[2] * METRES_PER_SECOND_SQUARED_TO_GFORCE,
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_GYROSCOPE_UNCALIBRATED:
            {
                AlignWithDisplay(m_motionSensorData.rotationRateRaw,
                                 event.values[0],
                                 event.values[1],
                                 event.values[2],
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_GYROSCOPE:
            {
                AlignWithDisplay(m_motionSensorData.rotationRateUnbiased,
                                 event.values[0],
                                 event.values[1],
                                 event.values[2],
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED:
            {
                AlignWithDisplay(m_motionSensorData.magneticFieldRaw,
                                 event.values[0],
                                 event.values[1],
                                 event.values[2],
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_MAGNETIC_FIELD:
            {
                AlignWithDisplay(m_motionSensorData.magneticFieldUnbiased,
                                 event.values[0],
                                 event.values[1],
                                 event.values[2],
                                 currentDisplayRotation);
            }
            break;
            case Sensor.TYPE_GAME_ROTATION_VECTOR:
            case Sensor.TYPE_ROTATION_VECTOR:
            {
                m_motionSensorData.orientation.x = event.values[0];
                m_motionSensorData.orientation.y = event.values[1];
                m_motionSensorData.orientation.z = event.values[2];
                m_motionSensorData.orientation.w = event.values[3];

                // Android doesn't provide us with any quaternion math,
                // so we do the alignment in MotionSensorinputDevice.cpp
                m_orientationAdjustmentRadiansZ = GetOrientationAdjustmentRadiansZ(currentDisplayRotation);
                m_motionSensorData.orientation.updated = true;
            }
            break;
        }
    }

    private void AlignWithDisplay(MotionSensorData.SensorData3D o_sensorData, float x, float y, float z, int displayRotation)
    {
        switch (displayRotation)
        {
            case Surface.ROTATION_90:
            {
                o_sensorData.x = -y;
                o_sensorData.y = -z;
                o_sensorData.z = x;
            }
            break;
            case Surface.ROTATION_180:
            {
                o_sensorData.x = -x;
                o_sensorData.y = -z;
                o_sensorData.z = -y;
            }
            break;
            case Surface.ROTATION_270:
            {
                o_sensorData.x = y;
                o_sensorData.y = -z;
                o_sensorData.z = -x;
            }
            break;
            case Surface.ROTATION_0:
            default:
            {
                o_sensorData.x = x;
                o_sensorData.y = -z;
                o_sensorData.z = y;
            }
            break;
        }

        o_sensorData.updated = true;
    }

    private float GetOrientationAdjustmentRadiansZ(int displayRotation)
    {
        switch (displayRotation)
        {
            case Surface.ROTATION_90:
            {
                return (float)(-Math.PI * 0.5d);
            }
            case Surface.ROTATION_180:
            {
                return (float)Math.PI;
            }
            case Surface.ROTATION_270:
            {
                return (float)(Math.PI * 0.5d);
            }
            case Surface.ROTATION_0:
            default:
            {
                return 0.0f;
            }
        }
    }

    // Called from native code to query availability of motion sensor data.
    public boolean IsMotionSensorDataAvailable(boolean accelerometerRaw,
                                               boolean accelerometerUser,
                                               boolean accelerometerGravity,
                                               boolean rotationRateRaw,
                                               boolean rotationRateUnbiased,
                                               boolean magneticFieldRaw,
                                               boolean magneticFieldUnbiased,
                                               boolean orientation)
    {
        if (m_sensorManager == null)
        {
            return false;
        }

        if (accelerometerRaw && m_sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER) == null)
        {
            return false;
        }

        if (accelerometerUser && m_sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION) == null)
        {
            return false;
        }

        if (accelerometerGravity && m_sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY) == null)
        {
            return false;
        }

        if (rotationRateRaw && m_sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED) == null)
        {
            return false;
        }

        if (rotationRateUnbiased && m_sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE) == null)
        {
            return false;
        }

        if (magneticFieldRaw && m_sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED) == null)
        {
            return false;
        }

        if (magneticFieldUnbiased && m_sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD) == null)
        {
            return false;
        }

        if (orientation && m_sensorManager.getDefaultSensor(m_orientationSensorToUse) == null)
        {
            return false;
        }

        return false;
    }

    // Called from native code to refresh motion sensors.
    // state: -1 = disable, 0 = unchanged, 1 = enable
    public void RefreshMotionSensors(float updateIntervalSeconds,
                                     int accelerometerRawState,
                                     int accelerometerUserState,
                                     int accelerometerGravityState,
                                     int rotationRateRawState,
                                     int rotationRateUnbiasedState,
                                     int magneticFieldRawState,
                                     int magneticFieldUnbiasedState,
                                     int orientationState)
    {
        int updateIntervalMicroeconds = (int)(updateIntervalSeconds * 1000000);
        RefreshMotionSensor(Sensor.TYPE_ACCELEROMETER, accelerometerRawState, updateIntervalMicroeconds);
        RefreshMotionSensor(Sensor.TYPE_LINEAR_ACCELERATION, accelerometerUserState, updateIntervalMicroeconds);
        RefreshMotionSensor(Sensor.TYPE_GRAVITY, accelerometerGravityState, updateIntervalMicroeconds);
        RefreshMotionSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED, rotationRateRawState, updateIntervalMicroeconds);
        RefreshMotionSensor(Sensor.TYPE_GYROSCOPE, rotationRateUnbiasedState, updateIntervalMicroeconds);
        RefreshMotionSensor(Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED, magneticFieldRawState, updateIntervalMicroeconds);
        RefreshMotionSensor(Sensor.TYPE_MAGNETIC_FIELD, magneticFieldUnbiasedState, updateIntervalMicroeconds);
        RefreshMotionSensor(m_orientationSensorToUse, orientationState, updateIntervalMicroeconds);
    }

    private void RefreshMotionSensor(int sensorType, int state, int updateIntervalMicroeconds)
    {
        if (m_sensorManager == null)
        {
            return;
        }

        // state: -1 = disable, 0 = unchanged, 1 = enable
        switch (state)
        {
            case 1:
            {
                Sensor defaultSensor = m_sensorManager.getDefaultSensor(sensorType);
                if (defaultSensor != null)
                {
                    m_sensorManager.registerListener(this, defaultSensor, updateIntervalMicroeconds);
                }
            }
            break;
            case -1:
            {
                Sensor defaultSensor = m_sensorManager.getDefaultSensor(sensorType);
                if (defaultSensor != null)
                {
                    m_sensorManager.unregisterListener(this, defaultSensor);
                }
            }
            break;
        }
    }

    // Called from native code to retrieve the latest motion sensor data.
    public float[] RequestLatestMotionSensorData()
    {
        // While we would ideally like to just return m_motionSensorData directly,
        // the native C++ code would then need to 'reach back' through the JNI
        // for each field just to access the raw data. Simply returning a float
        // array packed with all the required values is far more efficient, at
        // the expense of the native C++ code having to access the data through
        // 'magic' array indices instead of explcitly naming the class fields.
        //
        // Additionally, while explicitly calling out the class fields provides
        // a modicum of safety, lots of boilerplate code is needed, and while we
        // may in future append additional sensor data the existing elements are
        // unlikely to ever change. Combined with the above mentioned performance
        // considerations, this approach seems preferable on most fronts.

        m_motionSensorDataPacked[0] = m_motionSensorData.accelerationRaw.updated ? 1 : 0;
        m_motionSensorDataPacked[1] = m_motionSensorData.accelerationRaw.x;
        m_motionSensorDataPacked[2] = m_motionSensorData.accelerationRaw.y;
        m_motionSensorDataPacked[3] = m_motionSensorData.accelerationRaw.z;
        m_motionSensorData.accelerationRaw.updated = false;

        m_motionSensorDataPacked[4] = m_motionSensorData.accelerationUser.updated ? 1 : 0;
        m_motionSensorDataPacked[5] = m_motionSensorData.accelerationUser.x;
        m_motionSensorDataPacked[6] = m_motionSensorData.accelerationUser.y;
        m_motionSensorDataPacked[7] = m_motionSensorData.accelerationUser.z;
        m_motionSensorData.accelerationUser.updated = false;

        m_motionSensorDataPacked[8] = m_motionSensorData.accelerationGravity.updated ? 1 : 0;
        m_motionSensorDataPacked[9] = m_motionSensorData.accelerationGravity.x;
        m_motionSensorDataPacked[10] = m_motionSensorData.accelerationGravity.y;
        m_motionSensorDataPacked[11] = m_motionSensorData.accelerationGravity.z;
        m_motionSensorData.accelerationGravity.updated = false;

        m_motionSensorDataPacked[12] = m_motionSensorData.rotationRateRaw.updated ? 1 : 0;
        m_motionSensorDataPacked[13] = m_motionSensorData.rotationRateRaw.x;
        m_motionSensorDataPacked[14] = m_motionSensorData.rotationRateRaw.y;
        m_motionSensorDataPacked[15] = m_motionSensorData.rotationRateRaw.z;
        m_motionSensorData.rotationRateRaw.updated = false;

        m_motionSensorDataPacked[16] = m_motionSensorData.rotationRateUnbiased.updated ? 1 : 0;
        m_motionSensorDataPacked[17] = m_motionSensorData.rotationRateUnbiased.x;
        m_motionSensorDataPacked[18] = m_motionSensorData.rotationRateUnbiased.y;
        m_motionSensorDataPacked[19] = m_motionSensorData.rotationRateUnbiased.z;
        m_motionSensorData.rotationRateUnbiased.updated = false;

        m_motionSensorDataPacked[20] = m_motionSensorData.magneticFieldRaw.updated ? 1 : 0;
        m_motionSensorDataPacked[21] = m_motionSensorData.magneticFieldRaw.x;
        m_motionSensorDataPacked[22] = m_motionSensorData.magneticFieldRaw.y;
        m_motionSensorDataPacked[23] = m_motionSensorData.magneticFieldRaw.z;
        m_motionSensorData.magneticFieldRaw.updated = false;

        m_motionSensorDataPacked[24] = m_motionSensorData.magneticFieldUnbiased.updated ? 1 : 0;
        m_motionSensorDataPacked[25] = m_motionSensorData.magneticFieldUnbiased.x;
        m_motionSensorDataPacked[26] = m_motionSensorData.magneticFieldUnbiased.y;
        m_motionSensorDataPacked[27] = m_motionSensorData.magneticFieldUnbiased.z;
        m_motionSensorData.magneticFieldUnbiased.updated = false;

        m_motionSensorDataPacked[28] = m_motionSensorData.orientation.updated ? 1 : 0;
        m_motionSensorDataPacked[29] = m_motionSensorData.orientation.x;
        m_motionSensorDataPacked[30] = m_motionSensorData.orientation.y;
        m_motionSensorDataPacked[31] = m_motionSensorData.orientation.z;
        m_motionSensorDataPacked[32] = m_motionSensorData.orientation.w;
        m_motionSensorDataPacked[33] = m_orientationAdjustmentRadiansZ;
        m_motionSensorData.orientation.updated = false;

        return m_motionSensorDataPacked;
    }
}
