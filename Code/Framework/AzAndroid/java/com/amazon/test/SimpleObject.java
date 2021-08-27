/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
package com.amazon.test;


import java.util.Random;


public class SimpleObject
{
    public class Foo
    {
        public Foo(int seed)
        {
            Random rand = new Random(seed);
            m_value = rand.nextInt();
        }


        // ----

        public int m_value;
    }


    // ----

    public SimpleObject() {}

    public boolean GetBool() { return true; }
    public boolean[] GetBoolArray() { return new boolean[] { true, false, true, false }; }

    public char GetChar() { return 'L'; }
    public char[] GetCharArray() { return new char[] { 'L', 'u', 'm', 'b', 'e', 'r', 'y', 'a', 'r', 'd' }; }

    public byte GetByte() { return 1; }
    public byte[] GetByteArray() { return new byte[] { 1, 2, 4, 8, 16, 32 }; }

    public short GetShort() { return 128; }
    public short[] GetShortArray() { return new short[] { 128, 256, 512, 1024, 2048, 8192 }; }

    public int GetInt() { return 32768; }
    public int[] GetIntArray() { return new int[] { 32768, 65536, 131072, 262144, 524288, 1048576 }; }

    public float GetFloat() { return (float)Math.PI; }
    public float[] GetFloatArray()
    {
        float[] result = new float[6];
        for (int i = 0; i < 6; ++i)
        {
            result[i] = (float)(i + 1) / 7.0f;
        }
        return result;
    }

    public double GetDouble() { return Math.PI; }
    public double[] GetDoubleArray()
    {
        double[] result = new double[6];
        for (int i = 0; i < 6; ++i)
        {
            result[i] = (double)(i + 1) / 7.0;
        }
        return result;
    }

    public Class GetClass() { return this.getClass(); }

    public String GetString() { return "Amazon Lumberyard"; }

    public Foo GetObject() { return new Foo(1); }
    public Foo[] GetObjectArray() { return new Foo[] { new Foo(1), new Foo(2), new Foo(3), new Foo(4) }; }


    // ----

    private static final String TAG = "SimpleObject";
}
