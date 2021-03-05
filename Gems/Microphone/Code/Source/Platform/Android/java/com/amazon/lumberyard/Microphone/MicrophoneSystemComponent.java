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

package com.amazon.lumberyard.Microphone;

import android.media.AudioRecord;
import android.media.AudioFormat;
import android.media.MediaRecorder;
import android.util.Log;
import java.lang.Thread;
import java.lang.Runnable;
import java.lang.String;
import java.nio.ByteBuffer;

public class MicrophoneSystemComponent implements Runnable
{
    private AudioRecord recorder = null;
    private Thread recordingThread = null;
    private int bufferSize = 0;
    private int format = 0;
    private int sampleRate = 0;
    private int numChannels = 0;
    private static final int guaranteedSampleRate = 44100;
    private static final int desiredChannels = 1;
    private static final int desiredBitRate = 16;
    private static final int bufferSizeMultiplier = 10;
    private static final int frameSize = 1024;

    private static MicrophoneSystemComponent instance = null;

    public static native void SendCurrentData(byte[] data, int size);

    public static MicrophoneSystemComponent getInstance() {
        if(instance == null)
        {
            instance = new MicrophoneSystemComponent();
        }
        return instance;
    }

    public static boolean InitializeDevice()
    {
        return getInstance().InitializeDeviceImpl(guaranteedSampleRate, desiredChannels, desiredBitRate, false);
    }

    public static void ShutdownDevice()
    {
        getInstance().ShutdownDeviceImpl();
    }

    public static boolean StartSession()
    {
        return getInstance().StartSessionImpl();
    }

    public static void EndSession()
    {
        getInstance().EndSessionImpl();
    }

    public static boolean IsCapturing()
    {    
        return getInstance().IsCapturingImpl();
    }

    public static void ProcessData(ByteBuffer buffer, int sizeRead)
    {
        byte[] data = new byte[sizeRead];
        buffer.get(data, 0, sizeRead);
        SendCurrentData(data, sizeRead);
    }

    private boolean InitializeDeviceImpl(int sampleRateIn, int numChannelsIn, int bitsPerSampleIn, boolean isFloatIn)
    {
        sampleRate = sampleRateIn;
        if(numChannelsIn == 1)
        {
            numChannels = AudioFormat.CHANNEL_IN_MONO;
        }
        else if(numChannelsIn == 2)
        {
            numChannels = AudioFormat.CHANNEL_IN_STEREO;
        }
        else
        {
            Log.d("LMBR", String.format("Microphone::InitializeDevice, channel count wasn't 1 or 2. Instead got %d", numChannelsIn));
        }
        if(isFloatIn == true)
        {
            format = AudioFormat.ENCODING_PCM_FLOAT;
        }
        else
        {
            if(bitsPerSampleIn == 8)
            {
                format = AudioFormat.ENCODING_PCM_8BIT;
            }
            else if(bitsPerSampleIn == 16)
            {
                format = AudioFormat.ENCODING_PCM_16BIT;
            }
            else
            {
                Log.d("LMBR", String.format("Microphone::InitializeDevice, bits per sample wasn't 8 or 16. Instead got %d", bitsPerSampleIn));
            }
        }

        if(format != 0 && sampleRate != 0 && numChannels != 0)
        {
            // 44100 is the only guaranteed sample rate, so always use that and then downsample as needed
            bufferSize = AudioRecord.getMinBufferSize(guaranteedSampleRate, numChannels, format) * bufferSizeMultiplier;
            if(bufferSize != AudioRecord.ERROR_BAD_VALUE && bufferSize != AudioRecord.ERROR)
            {
                recorder = new AudioRecord(MediaRecorder.AudioSource.DEFAULT, guaranteedSampleRate, numChannels, format, bufferSize);
                if(recorder != null && recorder.getState() == AudioRecord.STATE_INITIALIZED)
                {
                    return true;
                }
            }
        }
        Log.d("LMBR", "Microphone::InitializeDevice, unable to create AudioRecord");
        return false;
    }

    private void ShutdownDeviceImpl()
    {
        if(recorder != null && recorder.getState() == AudioRecord.STATE_INITIALIZED)
        {
            recorder.stop();
            recorder.release();
        }
        recorder = null;
        recordingThread = null;
    }

    private boolean StartSessionImpl()
    {
        if(recorder != null && recorder.getState() == AudioRecord.STATE_INITIALIZED)
        {
            if(IsCapturingImpl())
            {
                EndSessionImpl();
            }
            recorder.startRecording();
            recordingThread = new Thread(this);
            recordingThread.start();
            return true;
        }        
        return false;
    }

    private void EndSessionImpl()
    {
        if(recorder != null && recorder.getState() == AudioRecord.STATE_INITIALIZED)
        {
            recorder.stop();
        }
    }

    private boolean IsCapturingImpl()
    {
        if(recorder != null)
        {
            return recorder.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING;
        }
        else
        {
            return false;
        }
    }

    @Override
    public void run()
    {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        while(recorder.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING)
        {
            ByteBuffer tempBuf = ByteBuffer.allocateDirect(frameSize);
            int sizeRead = recorder.read(tempBuf, frameSize);
            if(sizeRead >= 1)
            {
                ProcessData(tempBuf, sizeRead);
            }
        }
    }

    @Override
    protected void finalize() throws Throwable
    {
        super.finalize();
        ShutdownDeviceImpl();
    }
}