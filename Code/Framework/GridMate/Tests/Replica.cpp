/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <GridMate/Replica/ReplicaFunctions.h>

#include <AzCore/Math/Sfmt.h>
#include <AzCore/std/parallel/thread.h>

#include <GridMate/Carrier/DefaultSimulator.h>
#include <GridMate/Replica/Interpolators.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaMgr.h>

#include <GridMate/Serialize/CompressionMarshal.h>

#define GM_REPLICA_TEST_SESSION_CHANNEL 1

using namespace GridMate;

#if defined(max)
#undef max
#endif

#if defined(min)
#undef min
#endif

namespace UnitTest {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class InterpolatorTest 
    : public GridMateMPTestFixture
{
public:
    float m_zigVals[1000];
    static const int k_actualSampleStart = 100;
    static const int k_offsetBetweenSamples = 10;

    //-----------------------------------------------------------------------------
    InterpolatorTest()
    {
        for (int a = 0; a < 100; ++a)
        {
            m_zigVals[a      ] = static_cast< float >(rand() % 200 - 100);
            m_zigVals[a + 200] = static_cast< float >(rand() % 200 - 100);
            m_zigVals[a + 400] = static_cast< float >(rand() % 200 - 100);
            m_zigVals[a + 600] = static_cast< float >(rand() % 200 - 100);
            m_zigVals[a + 800] = static_cast< float >(rand() % 200 - 100);
        }

        for (int a = 100; a < 200; ++a)
        {
            m_zigVals[a] = 10.f;
        }

        for (int a = 300; a < 400; ++a)
        {
            m_zigVals[a] = static_cast< float >((a - 300) * (a - 300));
        }

        for (int a = 500; a < 600; ++a)
        {
            m_zigVals[a] = static_cast< float >(a - 500) * 0.7f - 20.f;
        }

        for (int a = 700; a < 800; ++a)
        {
            m_zigVals[a] = AZ::Sqrt(static_cast< float >(a));
        }

        for (int a = 900; a < 1000; ++a)
        {
            m_zigVals[a] = static_cast< float >(a - 900) * -5.f + 100.f;
        }
    }

    //-----------------------------------------------------------------------------
    template< typename T >
    void AddSamplesConstant(T& interpolator, int numSamples, const float k_constant)
    {
        for (int a = 0; a < numSamples; ++a)
        {
            interpolator.AddSample(k_constant, k_actualSampleStart + a * k_offsetBetweenSamples);
        }
    }

    //-----------------------------------------------------------------------------
    template< typename T >
    void AddSamplesLinear(T& interpolator, int numSamples, float slope, float yIntercept)
    {
        for (int a = 0; a < numSamples; ++a)
        {
            interpolator.AddSample(slope * static_cast< float >(a) + yIntercept, k_actualSampleStart + a * k_offsetBetweenSamples);
        }
    }

    //-----------------------------------------------------------------------------
    template< typename T >
    void AddSamplesZigZag(T& interpolator, int numSamples)
    {
        for (int a = 0; a < numSamples; ++a)
        {
            interpolator.AddSample(m_zigVals[a % AZ_ARRAY_SIZE(m_zigVals)], k_actualSampleStart + a * k_offsetBetweenSamples);
        }
    }

    void run()
    {
        //////////////////////////////////////////
        // testing point sample
        EpsilonThrottle< float > epsilon;
        epsilon.SetThreshold(0.001f);
        float check = -1.f;
        (void)check;

        // ensure interpolator returns correct value when it only has one sample
        {
            const int k_time = 0;
            const int k_sample = 1337;
            PointSample< int > interpolator;
            interpolator.AddSample(k_sample, k_time);
            AZ_TEST_ASSERT(interpolator.GetInterpolatedValue(k_time) == k_sample);
            AZ_TEST_ASSERT(interpolator.GetLastValue() == k_sample);
            AZ_TEST_ASSERT(interpolator.GetSampleCount() == 1);

            SampleInfo< int > info = interpolator.GetSampleInfo(0);
            AZ_TEST_ASSERT(info.m_t == k_time);
            AZ_TEST_ASSERT(info.m_v == k_sample);
        }

        // sample set partway full (pattern constant)
        {
            const int k_sampleArraySize = 100;
            const int k_numSamples = k_sampleArraySize;
            const float k_constant = 5.f;
            PointSample< float, k_sampleArraySize > interpolator;
            interpolator.Clear();
            AddSamplesConstant(interpolator, k_numSamples, k_constant);
            
            epsilon.SetBaseline(k_constant);

            for (int a = -k_offsetBetweenSamples; a < k_offsetBetweenSamples * (k_numSamples + 2); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set partway full (pattern linear)
        {
            const int k_numSamples = 500;
            const int k_sampleArraySize = 800;
            const float k_slope = 1.f;
            const float k_intercept = 10.f;
            PointSample< float, k_sampleArraySize > interpolator;
            AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
            
            epsilon.SetBaseline(k_intercept);

            // interpolate to value before any samples
            for (int a = k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate after samples
            for (int a = 0; a < k_numSamples * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(a / k_offsetBetweenSamples) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate to value after last sample
            for (int a = k_numSamples * k_offsetBetweenSamples; a < (k_numSamples + 2) * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - 1) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - 1) + k_intercept);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set partway full (pattern zigzag)
        {
            const int k_numSamples = 400;
            const int k_sampleArraySize = 800;
            PointSample< float, k_sampleArraySize > interpolator;
            AddSamplesZigZag(interpolator, k_numSamples);
            epsilon.SetBaseline(m_zigVals[0]);
            // interpolate to before earliest remaining sample record
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate from existing samples
            for (int a = 0; a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                float target = m_zigVals[idxLower];
                epsilon.SetBaseline(target);
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            }

            // interpolate after last known sample
            for (int a = k_offsetBetweenSamples * k_numSamples; a < k_offsetBetweenSamples * (1 + k_numSamples); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set full (pattern constant)
        {
            const int k_numSamples = 860;
            const int k_sampleArraySize = k_numSamples;
            const float k_constant = 5.f;
            PointSample< float, k_sampleArraySize > interpolator;
            AddSamplesConstant(interpolator, k_numSamples, k_constant);
            
            epsilon.SetBaseline(k_constant);

            for (int a = -k_offsetBetweenSamples; a < (k_numSamples + 2) * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set full (pattern linear)
        {
            const int k_sampleArraySize = 600;
            const int k_numSamples = k_sampleArraySize;
            const float k_slope = 1.f;
            const float k_intercept = 10.f;
            PointSample< float, k_sampleArraySize > interpolator;
            interpolator.Clear();
            AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
            
            epsilon.SetBaseline(k_intercept);

            // interpolate to value before any samples
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate after samples
            for (int a = 0; a < k_numSamples * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(a / k_offsetBetweenSamples) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate to value after last sample
            for (int a = k_numSamples * k_offsetBetweenSamples; a < (k_numSamples + 2) * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - 1) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            
            epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - 1) + k_intercept);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set full (pattern zigzag)
        {
            const int k_sampleArraySize = 1200;
            const int k_numSamples = k_sampleArraySize;
            PointSample< float, k_sampleArraySize > interpolator;
            AddSamplesZigZag(interpolator, k_numSamples);
            
            epsilon.SetBaseline(m_zigVals[0]);

            // interpolate to before earliest remaining sample record
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate from existing samples
            for (int a = 0; a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                float target = m_zigVals[idxLower];
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            }

            // interpolate after last known sample
            for (int a = k_offsetBetweenSamples * k_numSamples; a < k_offsetBetweenSamples * (1 + k_numSamples); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            
            epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set wrapped around (pattern constant)
        {
            const int k_sampleArraySize = 80;
            const int k_numSamples = 120;
            const float k_constant = 5.f;
            PointSample< float, k_sampleArraySize > interpolator;
            AddSamplesConstant(interpolator, k_numSamples, k_constant);
            epsilon.SetBaseline(k_constant);

            for (int a = -k_offsetBetweenSamples; a < (k_numSamples + 2) * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set wrapped around (pattern linear)
        {
            const int k_numSamples = 1200;
            const int k_sampleArraySize = 80;
            const float k_slope = 1.f;
            const float k_intercept = 10.f;
            PointSample< float, k_sampleArraySize > interpolator;
            AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);

            // interpolate to value before any samples
            for (int a = -k_offsetBetweenSamples + (k_numSamples - k_sampleArraySize) * k_offsetBetweenSamples; a < (k_numSamples - k_sampleArraySize) * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_intercept + k_slope * static_cast< float >(k_numSamples - k_sampleArraySize));
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate after samples
            for (int a = (k_numSamples - k_sampleArraySize) * k_offsetBetweenSamples; a < k_numSamples * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(a / k_offsetBetweenSamples) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate to value after last sample
            for (int a = k_numSamples * k_offsetBetweenSamples; a < (k_numSamples + 2) * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - 1) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - 1) + k_intercept);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set wrapped around (pattern zig-zag)
        {
            const int k_numSamples = 1500;
            const int k_sampleArraySize = 1000;
            PointSample< float, k_sampleArraySize > interpolator;
            interpolator.Clear();
            AddSamplesZigZag(interpolator, k_numSamples);

            // interpolate to before earliest remaining sample record
            for (int a = -k_offsetBetweenSamples + k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); a < k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - k_sampleArraySize) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));                
                (void)check;
            }

            // interpolate from existing samples
            for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                float target = m_zigVals[idxLower];
                epsilon.SetBaseline(target);
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            }

            // interpolate after last known sample
            for (int a = k_offsetBetweenSamples * k_numSamples; a < k_offsetBetweenSamples * (1 + k_numSamples); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // test Break
        {
            PointSample< float > interpolator;
            interpolator.Break();
        }

        // sample set max size 1
        {
            PointSample< float, 1 > interpolator;

            // with empty set (uncomment to make sure asserts fire)
            //check = interpolator.GetLastValue();
            //check = interpolator.GetInterpolatedValue( 210 );

            // with populated set
            interpolator.AddSample(1.f, 100);
            check = interpolator.GetInterpolatedValue(90);            
            epsilon.SetBaseline(1.f);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(1.f));
            check = interpolator.GetInterpolatedValue(100);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(110);            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetLastValue();            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));

            // with the only sample replaced
            interpolator.AddSample(10.f, 200);
            epsilon.SetBaseline(10.f);
            check = interpolator.GetInterpolatedValue(190);            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(200);            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(210);            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetLastValue();
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));

            // with the only sample replaced at the same time stamp
            interpolator.AddSample(20.f, 200);
            epsilon.SetBaseline(20.f);
            check = interpolator.GetInterpolatedValue(190);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(200);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(210);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetLastValue();
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
        }

        // end testing point-sampling
        //////////////////////////////////////////

        //////////////////////////////////////////
        // testing linear interpolation

        // ensure interpolator returns correct value when it only has one sample
        {
            const int k_time = 0;
            const int k_sample = 1337;
            LinearInterp< int > interpolator;
            interpolator.AddSample(k_sample, k_time);
            AZ_TEST_ASSERT(interpolator.GetInterpolatedValue(k_time) == k_sample);
            AZ_TEST_ASSERT(interpolator.GetLastValue() == k_sample);
            AZ_TEST_ASSERT(interpolator.GetSampleCount() == 1);

            SampleInfo< int > info = interpolator.GetSampleInfo(0);
            AZ_TEST_ASSERT(info.m_t == k_time);
            AZ_TEST_ASSERT(info.m_v == k_sample);
        }

        // sample set partway full (pattern constant)
        {
            const int k_numSamples = 50;
            const int k_sampleArraySize = 100;
            const float k_constant = 10.f;
            LinearInterp< float, k_sampleArraySize > interpolator;
            AddSamplesConstant(interpolator, k_numSamples, k_constant);
            epsilon.SetBaseline(k_constant);

            // interpolate to before samples start / where there are samples to interpolate / past last sample
            // [-10,0) : before samples start
            // [0, 40] : where there are samples to interpolate
            // (40, 50]: past last sample
            for (int a = -k_offsetBetweenSamples; a < k_numSamples * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set partway full (pattern linear)
        {
            const float k_slope = 0.5f;
            const float k_intercept = 5.f;
            const int k_numSamples = 600;
            const int k_sampleArraySize = 800;
            LinearInterp< float, k_sampleArraySize > interpolator;
            interpolator.Clear();
            AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
            epsilon.SetBaseline(k_intercept);

            // interpolate to before samples start
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate where there are samples to interpolate
            for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate past last sample
            float target = k_slope * static_cast< float >(k_numSamples - 1) + k_intercept;
            epsilon.SetBaseline(target);
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1) + 1; a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set partway full (pattern zigzag)
        {
            const int k_numSamples = 400;
            const int k_sampleArraySize = 500;
            LinearInterp< float, k_sampleArraySize > interpolator;
            AddSamplesZigZag(interpolator, k_numSamples);

            // interpolate to before samples start
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[0]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate to where there are samples to interpolate
            for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                int idxUpper = (idxLower + 1) % AZ_ARRAY_SIZE(m_zigVals);
                float target = m_zigVals[idxLower] + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower]) * static_cast< float >(a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples)) / static_cast< float >(k_offsetBetweenSamples);
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)target;
                (void)check;
            }

            // interpolate past last sample
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1); a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            
            epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set full (pattern constant)
        {
            const int k_numSamples = 500;
            const int k_sampleArraySize = k_numSamples;
            const float k_constant = 10.f;
            LinearInterp< float, k_sampleArraySize > interpolator;
            AddSamplesConstant(interpolator, k_numSamples, k_constant);
            epsilon.SetBaseline(k_constant);

            // interpolate to before samples start / where there are samples to interpolate / past last sample
            // [-10,0) : before samples start
            // [0, 40] : where there are samples to interpolate
            // (40, 50]: past last sample
            for (int a = -k_offsetBetweenSamples; a < k_numSamples * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);                
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set full (pattern linear)
        {
            const int k_numSamples = 850;
            const int k_sampleArraySize = k_numSamples;
            const float k_slope = 0.5f;
            const float k_intercept = 5.f;
            LinearInterp< float, k_sampleArraySize > interpolator;
            AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
            epsilon.SetBaseline(k_intercept);

            // interpolate to before samples start
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate where there are samples to interpolate
            for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate past last sample
            float target = k_slope * static_cast< float >(k_numSamples - 1) + k_intercept;
            epsilon.SetBaseline(target);
            
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1) + 1; a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);                
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }            
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        AZ_TracePrintf("GridMate", "this pointer: 0x%p\n", this);

        // sample set full (pattern zig-zag)
        {
            const int k_numSamples = 100;
            const int k_sampleArraySize = k_numSamples;
            LinearInterp< float, k_sampleArraySize > interpolator;
            interpolator.Clear();
            AddSamplesZigZag(interpolator, k_numSamples);
            epsilon.SetBaseline(m_zigVals[0]);

            // interpolate to before samples start
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);                
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate to where there are samples to interpolate
            for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                int idxUpper = (idxLower + 1) % AZ_ARRAY_SIZE(m_zigVals);
                float target = m_zigVals[idxLower]
                    + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                    * static_cast< float >(a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                    / static_cast< float >(k_offsetBetweenSamples);
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)target;
                (void)check;
            }

            // interpolate past last sample
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1); a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            
            epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set wrapped around (pattern constant)
        {
            const int k_sampleArraySize = 80;
            const int k_numSamples = 100;
            const float k_constant = 10.f;
            LinearInterp< float, k_sampleArraySize > interpolator;
            interpolator.Clear();
            AddSamplesConstant(interpolator, k_numSamples, k_constant);
            epsilon.SetBaseline(k_constant);

            // interpolate to before samples start / where there are samples to interpolate / past last sample
            // [-10,0) : before samples start
            // [0, 40] : where there are samples to interpolate
            // (40, 50]: past last sample
            for (int a = -k_offsetBetweenSamples; a < k_numSamples * k_offsetBetweenSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);                
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set wrapped around (pattern linear)
        {
            const int k_numSamples = 140;
            const int k_sampleArraySize = 90;
            const float k_slope = 0.5f;
            const float k_intercept = 5.f;
            LinearInterp< float, k_sampleArraySize > interpolator;
            AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);            

            // interpolate to before samples start
            for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize - 1); a < k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - k_sampleArraySize) + k_intercept);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate where there are samples to interpolate
            for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate past last sample
            float target = k_slope * static_cast< float >(k_numSamples - 1) + k_intercept;
            epsilon.SetBaseline(target);
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1) + 1; a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set wrapped around (pattern zigzag)
        {
            const int k_numSamples = 250;
            const int k_sampleArraySize = 100;
            LinearInterp< float, k_sampleArraySize > interpolator;
            AddSamplesZigZag(interpolator, k_numSamples);

            // interpolate to before samples start
            for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize - 1); a < k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - k_sampleArraySize) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate to where there are samples to interpolate
            for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                int idxUpper = (idxLower + 1) % AZ_ARRAY_SIZE(m_zigVals);
                float target = m_zigVals[idxLower] + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower]) * static_cast< float >(a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples)) / static_cast< float >(k_offsetBetweenSamples);
                epsilon.SetBaseline(target);
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));                
                (void)target;
                (void)check;
            }

            // interpolate past last sample
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1); a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            
            epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // test Break
        {
            const int k_numSamples = 20;
            const int k_sampleArraySize = k_numSamples;
            const int k_break = 10;
            const float k_intercept = 0.f;
            const float k_slope = 1.f;
            LinearInterp< float, k_sampleArraySize > interpolator;            
            for (int a = 0; a < k_numSamples; ++a)
            {
                if (a == k_break)
                {
                    interpolator.Break();
                }
                interpolator.AddSample(k_slope * static_cast< float >(a) + k_intercept, k_actualSampleStart + a * k_offsetBetweenSamples);
            }
            
            epsilon.SetBaseline(k_intercept);

            // interpolate to before samples start
            for (int a = -k_offsetBetweenSamples; a < 0; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate where there are samples to interpolate
            for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                float target;
                if (a / k_offsetBetweenSamples + 1 == k_break)
                {
                    target = k_slope * static_cast< float >(a / k_offsetBetweenSamples)  + k_intercept;
                }
                else
                {
                    target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                }
                
                epsilon.SetBaseline(target);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }

            // interpolate past last sample
            float target = k_slope * static_cast< float >(k_numSamples - 1) + k_intercept;
            epsilon.SetBaseline(target);
            for (int a = k_offsetBetweenSamples * (k_numSamples - 1) + 1; a < k_offsetBetweenSamples * k_numSamples; ++a)
            {
                check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                (void)check;
            }
            AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
        }

        // sample set max size 1
        {
            LinearInterp< float, 1 > interpolator;

            // with empty set (uncomment to make sure asserts fire)
            //interpolator.GetLastValue();
            //interpolator.GetInterpolatedValue( 210 );

            // with populated set
            interpolator.AddSample(1.f, 100);
            epsilon.SetBaseline(1.f);
            check = interpolator.GetInterpolatedValue(90);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(100);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(110);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetLastValue();
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));

            // with the only sample replaced
            interpolator.AddSample(10.f, 200);
            epsilon.SetBaseline(10.f);
            check = interpolator.GetInterpolatedValue(190);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(200);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(210);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetLastValue();
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));

            // with the only sample replaced at the same time stamp
            interpolator.AddSample(20.f, 200);
            epsilon.SetBaseline(20.f);
            check = interpolator.GetInterpolatedValue(190);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(200);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetInterpolatedValue(210);
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            check = interpolator.GetLastValue();
            AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
        }

        // end testing linear interpolation
        //////////////////////////////////////////

        //////////////////////////////////////////
        // testing linear interpolation and extrapolation
        {
            // ensure interpolator returns correct value when it only has one sample
            {
                const int k_time = 0;
                const int k_sample = 1337;
                LinearInterpExtrap< int > interpolator;
                interpolator.AddSample(k_sample, k_time);
                AZ_TEST_ASSERT(interpolator.GetInterpolatedValue(k_time) == k_sample);
                AZ_TEST_ASSERT(interpolator.GetLastValue() == k_sample);
                AZ_TEST_ASSERT(interpolator.GetSampleCount() == 1);

                SampleInfo< int > info = interpolator.GetSampleInfo(0);
                AZ_TEST_ASSERT(info.m_t == k_time);
                AZ_TEST_ASSERT(info.m_v == k_sample);
            }

            // sample set partway full (pattern constant)
            {
                const int k_numSamples = 35;
                const int k_sampleArraySize = 80;
                const float k_constant = 15.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                AddSamplesConstant(interpolator, k_numSamples, k_constant);
                epsilon.SetBaseline(k_constant);

                // interpolate to before samples start / where there are samples to interpolate / past last sample
                // [-10,0) : before samples start
                // [0, 70] : where there are samples to interpolate
                // (70, 80]: past last sample
                for (int a = -k_offsetBetweenSamples; a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);                    
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
                AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
            }

            // sample set partway full (pattern linear)
            {
                const int k_numSamples = 600;
                const int k_sampleArraySize = 800;
                const float k_slope = 3.f;
                const float k_intercept = -15.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
                epsilon.SetBaseline(k_intercept);

                // interpolate to before samples start
                for (int a = -k_offsetBetweenSamples; a < 0; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate where there are samples to interpolate
                for (int a = 0; a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) * 1.f / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate past last sample
                for (int a = k_offsetBetweenSamples * (k_numSamples) + 1; a < k_offsetBetweenSamples * (k_numSamples + 1); ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
            }

            // sample set partway full (pattern zig-zag)
            {
                const int k_numSamples = 750;
                const int k_sampleArraySize = 1000;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                interpolator.Clear();
                AddSamplesZigZag(interpolator, k_numSamples);
                epsilon.SetBaseline(m_zigVals[0]);

                // interpolate to before samples start
                for (int a = -k_offsetBetweenSamples; a < 0; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate to where there are samples to interpolate
                for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
                {
                    int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                    int idxUpper = (idxLower + 1) % AZ_ARRAY_SIZE(m_zigVals);
                    float target = m_zigVals[idxLower]
                        + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                        * static_cast< float >(a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                        / static_cast< float >(k_offsetBetweenSamples);
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)target;
                    (void)check;
                }

                // interpolate past last sample
                for (int a = k_offsetBetweenSamples * (k_numSamples - 1); a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    int idxUpper = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                    int idxLower = (idxUpper - 1) % AZ_ARRAY_SIZE(m_zigVals);
                    float target = m_zigVals[idxLower]
                        + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                        * static_cast< float >(k_offsetBetweenSamples + a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                        / static_cast< float >(k_offsetBetweenSamples);
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
            }

            // sample set full (pattern constant)
            {
                const int k_numSamples = 350;
                const int k_sampleArraySize = k_numSamples;
                const float k_constant = 15.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                interpolator.Clear();
                AddSamplesConstant(interpolator, k_numSamples, k_constant);
                epsilon.SetBaseline(k_constant);

                // interpolate to before samples start / where there are samples to interpolate / past last sample
                // [-10,0) : before samples start
                // [0, 70] : where there are samples to interpolate
                // (70, 80]: past last sample
                for (int a = -k_offsetBetweenSamples; a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);                    
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
                AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
            }

            // sample set full (pattern linear)
            {
                const int k_numSamples = 700;
                const int k_sampleArraySize = k_numSamples;
                const float k_slope = 3.f;
                const float k_intercept = -15.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
                epsilon.SetBaseline(k_intercept);

                // interpolate to before samples start
                for (int a = -k_offsetBetweenSamples; a < 0; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate where there are samples to interpolate
                for (int a = 0; a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate past last sample
                for (int a = k_offsetBetweenSamples * (k_numSamples) + 1; a < k_offsetBetweenSamples * (k_numSamples + 1); ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
            }

            // sample set full (pattern zigzag)
            {
                const int k_numSamples = 950;
                const int k_sampleArraySize = k_numSamples;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                AddSamplesZigZag(interpolator, k_numSamples);
                epsilon.SetBaseline(m_zigVals[0]);

                // interpolate to before samples start
                for (int a = -k_offsetBetweenSamples; a < 0; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate to where there are samples to interpolate
                for (int a = 0; a <= k_offsetBetweenSamples * (k_numSamples - 1); ++a)
                {
                    int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                    int idxUpper = (idxLower + 1) % AZ_ARRAY_SIZE(m_zigVals);
                    float target = m_zigVals[idxLower]
                        + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                        * static_cast< float >(a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                        / static_cast< float >(k_offsetBetweenSamples);
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)target;
                    (void)check;
                }

                // interpolate past last sample
                int idxUpper = (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals);
                int idxLower = (idxUpper - 1) % AZ_ARRAY_SIZE(m_zigVals);
                for (int a = k_offsetBetweenSamples * (k_numSamples - 1); a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    float target = m_zigVals[idxLower]
                        + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                        * static_cast< float >(k_offsetBetweenSamples + a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                        / static_cast< float >(k_offsetBetweenSamples);
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
                
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
            }

            // sample set wrapped around (pattern constant)
            {
                const int k_numSamples = 150;
                const int k_sampleArraySize = 80;
                const float k_constant = 15.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                AddSamplesConstant(interpolator, k_numSamples, k_constant);
                epsilon.SetBaseline(k_constant);

                // interpolate to before samples start / where there are samples to interpolate / past last sample
                // [-10,0) : before samples start
                // [0, 70] : where there are samples to interpolate
                // (70, 80]: past last sample
                for (int a = -k_offsetBetweenSamples; a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
                AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
            }

            // sample set wrapped around (linear)
            {
                const int k_numSamples = 800;
                const int k_sampleArraySize = 600;
                const float k_slope = 3.f;
                const float k_intercept = -15.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                interpolator.Clear();
                AddSamplesLinear(interpolator, k_numSamples, k_slope, k_intercept);
                epsilon.SetBaseline(k_slope * static_cast< float >(k_numSamples - k_sampleArraySize) + k_intercept);

                // interpolate to before samples start
                for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize - 1); a < k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate where there are samples to interpolate
                for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); a <= k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate past last sample
                for (int a = k_offsetBetweenSamples * (k_numSamples) + 1; a < k_offsetBetweenSamples * (k_numSamples + 1); ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
            }

            // sample set wrapped around (pattern zigzag)
            {
                const int k_numSamples = 1500;
                const int k_sampleArraySize = 1000;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;
                AddSamplesZigZag(interpolator, k_numSamples);
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - k_sampleArraySize) % AZ_ARRAY_SIZE(m_zigVals) ]);

                // interpolate to before samples start
                for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize - 1); a < k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate to where there are samples to interpolate
                for (int a = k_offsetBetweenSamples * (k_numSamples - k_sampleArraySize); a < k_offsetBetweenSamples * (k_numSamples - 1); ++a)
                {
                    int idxLower = (a / k_offsetBetweenSamples) % AZ_ARRAY_SIZE(m_zigVals);
                    int idxUpper = (idxLower + 1) % AZ_ARRAY_SIZE(m_zigVals);
                    float target = m_zigVals[idxLower]
                        + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                        * static_cast< float >(a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                        / static_cast< float >(k_offsetBetweenSamples);
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)target;
                    (void)check;
                }

                // interpolate past last sample
                int idxUpper = (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals);
                int idxLower = (idxUpper - 1) % AZ_ARRAY_SIZE(m_zigVals);
                for (int a = k_offsetBetweenSamples * (k_numSamples - 1); a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    float target = m_zigVals[idxLower]
                        + static_cast< float >(m_zigVals[idxUpper] - m_zigVals[idxLower])
                        * static_cast< float >(k_offsetBetweenSamples + a - k_offsetBetweenSamples * (a / k_offsetBetweenSamples))
                        / static_cast< float >(k_offsetBetweenSamples);
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
                
                epsilon.SetBaseline(m_zigVals[ (k_numSamples - 1) % AZ_ARRAY_SIZE(m_zigVals) ]);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(interpolator.GetLastValue()));
            }

            // test Break
            {
                const int k_numSamples = 20;
                const int k_sampleArraySize = k_numSamples;
                const int k_break = 10;
                const float k_intercept = 0.f;
                const float k_slope = 1.f;
                LinearInterpExtrap< float, k_sampleArraySize > interpolator;                
                
                for (int a = 0; a < k_numSamples; ++a)
                {
                    if (a == k_break)
                    {
                        interpolator.Break();
                    }
                    interpolator.AddSample(k_slope * static_cast< float >(a) + k_intercept, k_actualSampleStart + a * k_offsetBetweenSamples);
                }

                // interpolate to before samples start
                for (int a = -k_offsetBetweenSamples; a < 0; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    epsilon.SetBaseline(k_intercept);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate where there are samples to interpolate
                for (int a = 0; a < k_offsetBetweenSamples * k_numSamples; ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target;
                    if (a / k_offsetBetweenSamples + 1 == k_break)
                    {
                        target = k_slope * static_cast< float >(a / k_offsetBetweenSamples)  + k_intercept;
                    }
                    else
                    {
                        target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    }
                    
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }

                // interpolate past last sample
                for (int a = k_offsetBetweenSamples * (k_numSamples) + 1; a < k_offsetBetweenSamples * (k_numSamples + 1); ++a)
                {
                    check = interpolator.GetInterpolatedValue(k_actualSampleStart + a);
                    float target = k_slope * static_cast< float >(a) / static_cast< float >(k_offsetBetweenSamples) + k_intercept;
                    
                    epsilon.SetBaseline(target);
                    AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                    (void)check;
                }
            }

            // sample set max size 1
            {
                LinearInterpExtrap< float, 1 > interpolator;

                // with empty set (uncomment to make sure asserts fire)
                //interpolator.GetLastValue();
                //interpolator.GetInterpolatedValue( 210 );

                // with populated set
                interpolator.AddSample(1.f, 100);
                epsilon.SetBaseline(1.f);
                check = interpolator.GetInterpolatedValue(90);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetInterpolatedValue(100);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetInterpolatedValue(110);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetLastValue();
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));

                // with the only sample replaced
                interpolator.AddSample(10.f, 200);
                epsilon.SetBaseline(10.f);
                check = interpolator.GetInterpolatedValue(190);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetInterpolatedValue(200);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetInterpolatedValue(210);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetLastValue();
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));

                // with the only sample replaced at the same time stamp
                interpolator.AddSample(20.f, 200);
                epsilon.SetBaseline(20.f);
                check = interpolator.GetInterpolatedValue(190);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetInterpolatedValue(200);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetInterpolatedValue(210);
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
                check = interpolator.GetLastValue();
                AZ_TEST_ASSERT(epsilon.WithinThreshold(check));
            }

            AZ_TracePrintf("GridMate", "this pointer: 0x%p", this);
        }
        // end testing linear interpolation and extrapolation
        //////////////////////////////////////////
    }
};

} // namespace UnitTest

GM_TEST_SUITE(ReplicaSuite)
GM_TEST(InterpolatorTest)
GM_TEST_SUITE_END()
