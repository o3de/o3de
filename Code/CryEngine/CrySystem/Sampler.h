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

#ifndef CRYINCLUDE_CRYSYSTEM_SAMPLER_H
#define CRYINCLUDE_CRYSYSTEM_SAMPLER_H
#pragma once

#ifdef WIN32

class CSamplingThread;

//////////////////////////////////////////////////////////////////////////
// Sampler class is running a second thread which is at regular intervals
// eg 1ms samples main thread and stores current IP in the samples buffers.
// After sampling finishes it can resolve collected IP buffer info to
// the function names and calculated where most of the execution time spent.
//////////////////////////////////////////////////////////////////////////
class CSampler
{
public:
    struct SFunctionSample
    {
        string function;
        uint32 nSamples; // Number of samples per function.
    };

    CSampler();
    ~CSampler();

    void Start();
    void Stop();
    void Update();

    // Adds a new sample to the ip buffer, return false if no more samples can be added.
    bool AddSample(uint64 ip);
    void SetMaxSamples(int nMaxSamples);

    int GetSamplePeriod() const { return m_samplePeriodMs; }
    void SetSamplePeriod(int millis) { m_samplePeriodMs = millis; }

private:
    void ProcessSampledData();
    void LogSampledData();

    // Buffer for IP samples.
    std::vector<uint64> m_rawSamples;
    std::vector<SFunctionSample> m_functionSamples;
    int m_nMaxSamples;
    bool m_bSampling;
    bool m_bSamplingFinished;

    int m_samplePeriodMs;

    CSamplingThread* m_pSamplingThread;
};

#else //WIN32

// Dummy sampler.
class CSampler
{
public:
    void Start() {}
    void Stop() {}
    void Update() {}
    void SetMaxSamples(int) {}
    void SetSamplePeriod(int) {}
};

#endif // WIN32

#endif // CRYINCLUDE_CRYSYSTEM_SAMPLER_H
