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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class MPSession
    : public CarrierEventBus::Handler
{
public:
    ReplicaManager& GetReplicaMgr()                         { return m_rm; }
    void            SetTransport(Carrier* transport)        { m_pTransport = transport; CarrierEventBus::Handler::BusConnect(transport->GetGridMate()); }
    Carrier*        GetTransport()                          { return m_pTransport; }
    void            SetClient(bool isClient)                { m_client = isClient; }
    void            AcceptConn(bool accept)                 { m_acceptConn = accept; }

    ~MPSession()
    {
        CarrierEventBus::Handler::BusDisconnect();
    }

    void Update()
    {
        char buf[1500];
        for (ConnectionSet::iterator iConn = m_connections.begin(); iConn != m_connections.end(); ++iConn)
        {
            ConnectionID conn = *iConn;
            Carrier::ReceiveResult result = m_pTransport->Receive(buf, 1500, conn, GM_REPLICA_TEST_SESSION_CHANNEL);
            if (result.m_state == Carrier::ReceiveResult::RECEIVED)
            {
                if (strcmp(buf, "IM_A_CLIENT") == 0)
                {
                    m_rm.AddPeer(conn, Mode_Client);
                }
                else if (strcmp(buf, "IM_A_PEER") == 0)
                {
                    m_rm.AddPeer(conn, Mode_Peer);
                }
            }
        }
    }

    template<typename T>
    typename T::Ptr GetChunkFromReplica(ReplicaId id)
    {
        ReplicaPtr replica = GetReplicaMgr().FindReplica(id);
        if (!replica)
        {
            return nullptr;
        }
        return replica->FindReplicaChunk<T>();
    }

    //////////////////////////////////////////////////////////////////////////
    // CarrierEventBus
    void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override
    {
        if (carrier != m_pTransport)
        {
            return; // not for us
        }
        m_connections.insert(id);
        if (m_client)
        {
            m_pTransport->Send("IM_A_CLIENT", 12, id, Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, GM_REPLICA_TEST_SESSION_CHANNEL);
        }
        else
        {
            m_pTransport->Send("IM_A_PEER", 10, id, Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, GM_REPLICA_TEST_SESSION_CHANNEL);
        }
    }

    void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason /*reason*/) override
    {
        if (carrier != m_pTransport)
        {
            return; // not for us
        }
        m_rm.RemovePeer(id);
        m_connections.erase(id);
    }

    void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) override
    {
        (void)error;
        if (carrier != m_pTransport)
        {
            return; // not for us
        }
        m_pTransport->Disconnect(id);
    }

    void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) override
    {
        (void)carrier;
        (void)id;
        (void)error;
        //Ignore security warnings in unit tests
    }
    //////////////////////////////////////////////////////////////////////////

    ReplicaManager              m_rm;
    Carrier*                    m_pTransport;
    typedef unordered_set<ConnectionID> ConnectionSet;
    ConnectionSet               m_connections;
    bool                        m_client;
    bool                        m_acceptConn;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class MyObj
{
public:
    GM_CLASS_ALLOCATOR(MyObj);
    MyObj()
        : m_f1(0.f)
        , m_b1(false)
        , m_i1(0) {}

    float   m_f1;
    bool    m_b1;
    int     m_i1;
};

//-----------------------------------------------------------------------------
class MyCtorContext
    : public CtorContextBase
{
public:
    CtorDataSet<float, Float16Marshaler>    m_f;

    MyCtorContext()
        : m_f(Float16Marshaler(0.f, 1.f))
    {}
};

//-----------------------------------------------------------------------------
class MigratableReplica
    : public ReplicaChunk
{
public:
    class Descriptor
        : public ReplicaChunkDescriptor
    {
    public:
        Descriptor()
            : ReplicaChunkDescriptor(MigratableReplica::GetChunkName(), sizeof(MigratableReplica))
        {
        }

        ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
        {
            MyCtorContext cc;
            cc.Unmarshal(*mc.m_iBuf);

            // Important hooks. Pre/Post construct allows us to detect all datasets.
            if (mc.m_rm->GetUserContext(12345))
            {
                AZ_TracePrintf("GridMate", "Create with UserData:%p\n", mc.m_rm->GetUserContext(12345));
            }
            ReplicaChunk* chunk = aznew MigratableReplica;
            return chunk;
        }

        void DiscardCtorStream(UnmarshalContext& mc) override
        {
            MyCtorContext cc;
            cc.Unmarshal(*mc.m_iBuf);
        }

        void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }

        void MarshalCtorData(ReplicaChunkBase*, WriteBuffer& wb) override
        {
            MyCtorContext cc;
            cc.m_f.Set(0.5f);
            cc.Marshal(wb);
        }
    };

    class MigratableReplicaDebugMsgs
        : public AZ::Debug::DrillerEBusTraits
    {
    public:
        typedef AZ::EBus<MigratableReplicaDebugMsgs> EBus;

        virtual void OnNewOwner(ReplicaId repId, ReplicaManager* repMgr) = 0;
    };

    typedef AZStd::intrusive_ptr<MigratableReplica> Ptr;

    GM_CLASS_ALLOCATOR(MigratableReplica);
    static const char* GetChunkName() {return "MigratableReplica"; }

    MigratableReplica(MyObj* pObj = nullptr)
        : MyHandler123Rpc("MyHandler123Rpc")
        , m_data1("Data1")
        , m_data2("Data2")
        , m_data3("Data3", 3.0f, Float16Marshaler(0.0f, 10.0f))
        , m_data4("Data4")

    {
        Bind(pObj);
    }

    bool IsReplicaMigratable() override
    {
        return true;
    }

    bool MyHandler123(const float& f, const RpcContext& rc)
    {
        (void)f;
        (void)rc;
        AZ_TracePrintf("GridMate", "Executed MyHandler123 requested at %u with %g on %s at %u.\n", rc.m_timestamp, f, GetReplica()->IsPrimary() ? "Primary" : "Proxy", rc.m_realTime);
        return true;
    }

    Rpc<RpcArg<const float&> >::BindInterface<MigratableReplica, & MigratableReplica::MyHandler123> MyHandler123Rpc;

    void UpdateChunk(const ReplicaContext& rc) override
    {
        if (m_pLocalObj)
        {
            m_data1.Set(m_pLocalObj->m_f1);
            m_data1Interpolated.AddSample(m_pLocalObj->m_f1, rc.m_localTime);

            m_data2.Set(m_pLocalObj->m_i1);
            m_data3.Set(m_pLocalObj->m_f1);
        }
        AZStd::bitset<25> bits = m_data4.Get();
        m_data4.Set(bits.flip());
    }

    void UpdateFromChunk(const ReplicaContext& rc) override
    {
        //      AZ_TracePrintf("GridMate", "Updating proxy 0x%x on peer %d coming from peer %d %s\n", GetRepId(), rc.rm->GetLocalPeerId(), rc.myPeer->GetId(), rc.myPeer->GetConnectionId() == InvalidConnectionID ? "(orphan)" : "");
        if (m_pLocalObj)
        {
            m_data1Interpolated.AddSample(m_data1.Get(), m_data1.GetLastUpdateTime());
            m_pLocalObj->m_f1 = m_data1Interpolated.GetInterpolatedValue(rc.m_localTime);

            m_pLocalObj->m_i1 = m_data2.Get();
        }
        m_dummy = m_data3.Get();
    }

    void OnReplicaActivate(const ReplicaContext& rc) override
    {
        (void)rc;
        if (rc.m_rm->GetUserContext(12345))
        {
            AZ_TracePrintf("GridMate", "Activate %s with UserData:%p\n", GetReplica()->IsPrimary() ? "primary" : "proxy", rc.m_rm->GetUserContext(12345));
        }
        if (IsProxy())
        {
            Bind(aznew MyObj());
        }

        if (IsPrimary())
        {
            EBUS_EVENT(MigratableReplicaDebugMsgs::EBus, OnNewOwner, GetReplicaId(), rc.m_rm);
        }
    }

    void OnReplicaDeactivate(const ReplicaContext& rc) override
    {
        (void)rc;
        if (m_pLocalObj)
        {
            delete m_pLocalObj;
            m_pLocalObj = NULL;
        }
    }

    void OnReplicaChangeOwnership(const ReplicaContext& rc) override
    {
        (void)rc;
        AZ_TracePrintf("GridMate", "Migratable replica 0x%x became %s on Peer %d\n", (int) GetReplicaId(), IsPrimary() ? "primary" : "proxy", (int) rc.m_rm->GetLocalPeerId());

        if (IsPrimary())
        {
            EBUS_EVENT(MigratableReplicaDebugMsgs::EBus, OnNewOwner, GetReplicaId(), rc.m_rm);
        }
    }

    void Bind(MyObj* pObj)
    {
        m_pLocalObj = pObj;
    }
private:
    DataSet<float> m_data1;
    LinearInterpExtrap<float> m_data1Interpolated;

    DataSet<int> m_data2;
    DataSet<float, Float16Marshaler> m_data3;
    DataSet<AZStd::bitset<25> > m_data4;

    MyObj* m_pLocalObj;
    float m_dummy;
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class NonMigratableReplica
    : public ReplicaChunk
{
public:
    enum EBla : AZ::u8
    {
        e_Bla0,
        e_Bla1,
    };
    typedef vector<int> IntVectorType;
    bool m_unreliableCheck;
protected:
    MyObj* m_pLocalObj;
    int m_prevUnreliableValue;

    bool MyHandler123(const float& f, const RpcContext& rc)
    {
        (void)f;
        (void)rc;
        AZ_TracePrintf("GridMate", "Executed MyHandler123 requested at %u with %g on %s at %u.\n", rc.m_timestamp, f, IsPrimary() ? "Primary" : "Proxy", rc.m_realTime);
        return true;
    }
    bool MyHandler2(const float& f, int p2, const RpcContext& rc)
    {
        (void)f;
        (void)p2;
        (void)rc;
        AZ_TracePrintf("GridMate", "Executed MyHandler2 requested at %u with %g,%d on %s at %u.\n", rc.m_timestamp, f, p2, IsPrimary() ? "Primary" : "Proxy", rc.m_realTime);
        return true;
    }
    bool MyHandler3(const float& f, int p2, EBla p3, const RpcContext& rc)
    {
        (void)f;
        (void)p2;
        (void)p3;
        (void)rc;
        AZ_TracePrintf("GridMate", "Executed MyHandler3 requested at %u with %g,%d,%d on %s at %u.\n", rc.m_timestamp, f, p2, p3, IsPrimary() ? "Primary" : "Proxy", rc.m_realTime);
        return true;
    }
    bool MyHandler4(const float& f, int p2, EBla p3, const IntVectorType& p4, const RpcContext& rc)
    {
        (void)f;
        (void)p2;
        (void)p3;
        (void)p4;
        (void)rc;
        AZ_TracePrintf("GridMate", "Executed MyHandler4 requested at %u with %g,%d,%d,%d,%d on %s at %u.\n", rc.m_timestamp, f, p2, p3, p4[0], p4[1], IsPrimary() ? "Primary" : "Proxy", rc.m_realTime);
        return true;
    }
    bool MyHandlerUnreliable(const int& i, const RpcContext& rc)
    {
        (void)rc;
        AZ_TracePrintf("GridMate", "Executed MyHandlerUnreliable requested at %u with %d on %s at %u.\n", rc.m_timestamp, i, IsPrimary() ? "Primary" : "Proxy", rc.m_realTime);
        AZ_TEST_ASSERT(i > m_prevUnreliableValue);
        if ((i - m_prevUnreliableValue) > 1)
        {
            m_unreliableCheck = true;
        }
        m_prevUnreliableValue = i;
        return true;
    }
public:
    GM_CLASS_ALLOCATOR(NonMigratableReplica);
    typedef AZStd::intrusive_ptr<NonMigratableReplica> Ptr;
    static const char* GetChunkName() { return "NonMigratableReplica"; }

    Rpc<RpcArg<const float&> >::BindInterface<NonMigratableReplica, & NonMigratableReplica::MyHandler123> MyHandler123Rpc;
    Rpc<RpcArg<const float&>, RpcArg<int> >::BindInterface<NonMigratableReplica, & NonMigratableReplica::MyHandler2> MyHandler2Rpc;
    Rpc<RpcArg<const float&>, RpcArg<int>, RpcArg<EBla> >::BindInterface<NonMigratableReplica, & NonMigratableReplica::MyHandler3> MyHandler3Rpc;
    Rpc<RpcArg<const float&>, RpcArg<int>, RpcArg<EBla>, RpcArg<const IntVectorType&> >::BindInterface<NonMigratableReplica, & NonMigratableReplica::MyHandler4> MyHandler4Rpc;

    Rpc<RpcArg<const int&> >::BindInterface<NonMigratableReplica, & NonMigratableReplica::MyHandlerUnreliable, RpcUnreliable> MyHandlerUnreliableRpc;

    NonMigratableReplica(MyObj* pObj = NULL)
        : m_unreliableCheck(false)
        , m_prevUnreliableValue(0)
        , MyHandler123Rpc("MyHandler123Rpc")
        , MyHandler2Rpc("MyHandler2Rpc")
        , MyHandler3Rpc("MyHandler3Rpc")
        , MyHandler4Rpc("MyHandler4Rpc")
        , MyHandlerUnreliableRpc("MyHandlerUnreliableRpc")
        , m_data1("Data1")
        , m_data2("Data2")
    {
        Bind(pObj);
    }

    bool IsReplicaMigratable() override
    {
        return false;
    }

    ~NonMigratableReplica()
    {
        AZ_Assert(!m_pLocalObj, "Local object should be cleared");
    }

    void UpdateChunk(const ReplicaContext& rc) override
    {
        m_data1.Set(m_pLocalObj->m_f1);
        m_data1Interpolated.AddSample(m_pLocalObj->m_f1, rc.m_localTime);

        m_data2.Set(m_pLocalObj->m_i1);
    }

    void UpdateFromChunk(const ReplicaContext& rc) override
    {
        m_data1Interpolated.AddSample(m_data1.Get(), m_data1.GetLastUpdateTime());
        m_pLocalObj->m_f1 = m_data1Interpolated.GetInterpolatedValue(rc.m_localTime);

        m_pLocalObj->m_i1 = m_data2.Get();
    }

    void OnReplicaActivate(const ReplicaContext& rc) override
    {
        (void)rc;
        if (rc.m_rm->GetUserContext(12345))
        {
            AZ_TracePrintf("GridMate", "Activate %s with UserData:%p\n", IsPrimary() ? "primary" : "proxy", rc.m_rm->GetUserContext(12345));
        }
        if (IsProxy())
        {
            Bind(aznew MyObj());
        }
    }

    void OnReplicaDeactivate(const ReplicaContext& rc) override
    {
        (void)rc;
        if (m_pLocalObj)
        {
            delete m_pLocalObj;
            m_pLocalObj = NULL;
        }
    }

    void OnReplicaChangeOwnership(const ReplicaContext& rc) override
    {
        (void)rc;
        AZ_TracePrintf("GridMate", "NonMigratable replica 0x%x became %s on Peer %d\n", (int) GetReplicaId(), IsPrimary() ? "primary" : "proxy", (int) rc.m_rm->GetLocalPeerId());
    }

    void Bind(MyObj* pObj)
    {
        m_pLocalObj = pObj;
    }

protected:
    DataSet<float> m_data1;
    LinearInterpExtrap<float> m_data1Interpolated;

    DataSet<int> m_data2;
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class MyDerivedReplica
    : public NonMigratableReplica
{
public:
    GM_CLASS_ALLOCATOR(MyDerivedReplica);

    MyDerivedReplica()
        : m_data3("Data3") { }

    typedef AZStd::intrusive_ptr<MyDerivedReplica> Ptr;
    static const char* GetChunkName() { return "MyDerivedReplica"; }

    virtual void UpdateChunk(const ReplicaContext& rc) override
    {
        NonMigratableReplica::UpdateChunk(rc);
        m_data3.Set(m_pLocalObj->m_b1);
    }

    virtual void UpdateFromChunk(const ReplicaContext& rc) override
    {
        NonMigratableReplica::UpdateFromChunk(rc);
        m_pLocalObj->m_b1 = m_data3.Get();
    }

protected:
    DataSet<bool> m_data3;
};
//-----------------------------------------------------------------------------

class ReplicaGMTest
    : public UnitTest::GridMateMPTestFixture
    , public ::testing::Test
{};

TEST_F(ReplicaGMTest, DISABLED_ReplicaTest)
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<MigratableReplica, MigratableReplica::Descriptor>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<NonMigratableReplica>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<MyDerivedReplica>();

        AZ_TracePrintf("GridMate", "\n");
        enum
        {
            s1,
            s2,
            s3,
            nSessions
        };
        const int k_delay = 100;

        // Setting up simulator with outgoing packet loss
        DefaultSimulator clientSimulator;
        clientSimulator.SetOutgoingPacketLoss(1, 1);

        MPSession       sessions[nSessions];

        MyObj* s1obj1 = NULL, * s1obj2 = NULL, * s2obj1 = NULL, * s3obj1 = NULL;
        MigratableReplica::Ptr s1rep1, s3rep1;
        NonMigratableReplica::Ptr s1rep2;
        MyDerivedReplica::Ptr s2rep1;
        ReplicaId s1rep1id = 0, s1rep2id = 0, s2rep1id = 0, s3rep1id = 0;

        // initialize transport
        int basePort = 4427;
        for (int i = 0; i < nSessions; ++i)
        {
        TestCarrierDesc desc;
            desc.m_port = basePort + i;
            desc.m_enableDisconnectDetection = false;
            if (i == s2)
            {
                desc.m_simulator = &clientSimulator;
            }

            // initialize replica managers
            // s2(p)<-->(p)s1(p)<-->(c)s3
            sessions[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            sessions[i].AcceptConn(true);
            sessions[i].SetClient(i == s3);
            sessions[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1, sessions[i].GetTransport(), 0, i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0));
            sessions[i].GetReplicaMgr().RegisterUserContext(12345, reinterpret_cast<void*>(static_cast<size_t>(i + 1)));
        }
        sessions[0].GetReplicaMgr().SetLocalLagAmt(50);

        // put something on s1 to get it going
        auto rep = Replica::CreateReplica(nullptr);
        s1rep1 = CreateAndAttachReplicaChunk<MigratableReplica>(rep);
        s1rep1id = sessions[s1].GetReplicaMgr().AddPrimary(rep);
        s1rep1->Bind(s1obj1 = aznew MyObj());

        // connect s2 to s1
        sessions[s2].GetTransport()->Connect("127.0.0.1", basePort);

        // main test loop
        static bool keepRunning = true;
        int tick = 0;
        while (keepRunning)
        {
            // perform some random actions on a timeline
            switch (tick)
            {
            case 5:
            {
                // connect s3 to s1
                sessions[s3].GetTransport()->Connect("127.0.0.1", basePort);
                break;
            }
            case 25:
                // remove s1rep1
                AZ_TEST_ASSERT(s1rep1id);
                s1rep1->GetReplica()->Destroy();
                s1obj1 = NULL;
                break;
            case 35:
                //AZ_TracePrintf("GridMate", "No more updates.\n");
                break;
            case 70:
                //AZ_TracePrintf("GridMate", "Restart updates.\n");
                break;
            case 90:
                keepRunning = false;
            }

            // add an object on s2
            if (sessions[s2].GetReplicaMgr().IsReady())
            {
                if (!s2rep1id)
                {
                    auto newReplica = Replica::CreateReplica(nullptr);
                    s2rep1 = CreateAndAttachReplicaChunk<MyDerivedReplica>(newReplica);
                    s2rep1id = sessions[s2].GetReplicaMgr().AddPrimary(newReplica);
                    s2rep1->Bind(s2obj1 = aznew MyObj());
                }
                else
                {
                    static bool sends2rep1rpc = true;
                    if (sends2rep1rpc && tick >= 20)
                    {
                        s2rep1->MyHandler123Rpc(5.f);
                        s2rep1->MyHandler2Rpc(6.0f, 1);
                        s2rep1->MyHandler3Rpc(7.0f, 2, NonMigratableReplica::e_Bla0);
                        NonMigratableReplica::IntVectorType v;
                        v.push_back(10);
                        v.push_back(13);
                        s2rep1->MyHandler4Rpc(8.0f, 3, NonMigratableReplica::e_Bla1, v);
                        sends2rep1rpc = false;
                    }
                }
            }

            // add object on s1
            if (sessions[s1].GetReplicaMgr().IsReady())
            {
                if (!s1rep2)
                {
                    auto newReplica = Replica::CreateReplica(nullptr);
                    s1rep2 = CreateAndAttachReplicaChunk<NonMigratableReplica>(newReplica);
                    s1rep2id = sessions[s1].GetReplicaMgr().AddPrimary(newReplica);
                    s1rep2->Bind(s1obj2 = aznew MyObj);
                }
                else
                {
                    if (s1rep2id && tick >= 40)
                    {
                        s1rep2->GetReplica()->Destroy();
                        s1obj2 = NULL;
                        s1rep2id = 0;
                    }
                }
            }

            // add object on s3
            if (sessions[s3].GetReplicaMgr().IsReady())
            {
                if (!s3rep1)
                {
                    auto newReplica = Replica::CreateReplica(nullptr);
                    s3rep1 = CreateAndAttachReplicaChunk<MigratableReplica>(newReplica);
                    s3rep1id = sessions[s3].GetReplicaMgr().AddPrimary(newReplica);
                    s3rep1->Bind(s3obj1 = aznew MyObj());
                }
                else
                {
                    if (s3rep1id && tick >= 45)
                    {
                        s3rep1->MyHandler123Rpc(-1.f);
                        s3rep1->GetReplica()->Destroy();
                        s3obj1 = NULL;
                        s3rep1id = 0;
                    }
                }
            }

            { // Testing unreliable rpcs: enabling network simulator with outgoing packetloss,
              // calling 10 rpcs with 1..10 int argument, checking if replicas got rpcs in an order, and have missing calls
                static bool requestrpc = true;
                if (s3rep1id && requestrpc)
                {
                    if (ReplicaPtr pObj = sessions[s2].GetReplicaMgr().FindReplica(s3rep1id))
                    {
                        pObj->FindReplicaChunk<MigratableReplica>()->MyHandler123Rpc(2.0f);
                        requestrpc = false;
                    }
                }

                static bool unreliableRequest = true;
                static int numUnreliableRequests = 0;
                if (sessions[s2].GetReplicaMgr().IsReady() && s2rep1 && tick > 15 && unreliableRequest)
                {
                    // Starting packet loss
                    unreliableRequest = false;
                }

                if (!unreliableRequest && numUnreliableRequests < 10)
                {
                    if (numUnreliableRequests == 4)
                    {
                        clientSimulator.Enable();
                    }
                    else if (numUnreliableRequests == 5)
                    {
                        clientSimulator.Disable();
                    }
                    s2rep1->MyHandlerUnreliableRpc(++numUnreliableRequests);
                }

                static bool checkUnreliableDelivery = true;
                if (checkUnreliableDelivery && tick >= 25)
                {
                    // Stopping packet loss
                    ReplicaPtr rep1 = sessions[s1].GetReplicaMgr().FindReplica(s2rep1->GetReplicaId());
                    ReplicaPtr rep3 = sessions[s3].GetReplicaMgr().FindReplica(s2rep1->GetReplicaId());
                    AZ_TEST_ASSERT(rep1);
                    AZ_TEST_ASSERT(rep3);
                    AZ_TEST_ASSERT(rep1->FindReplicaChunk<MyDerivedReplica>()->m_unreliableCheck);
                    AZ_TEST_ASSERT(rep3->FindReplicaChunk<MyDerivedReplica>()->m_unreliableCheck);
                    checkUnreliableDelivery = false;
                }
            }

            // modify local objects
            if (tick < 20 || tick > 70)
            {
                if (s1obj1)
                {
                    s1obj1->m_f1 += 0.5f;
                    s1obj1->m_i1 += 1;
                    s1obj1->m_b1 = !s1obj1->m_b1;
                }
                if (s1obj2)
                {
                    s1obj2->m_f1 += 1.0f;
                    s1obj2->m_i1 -= 1;
                    s1obj2->m_b1 = !s1obj2->m_b1;
                }
                if (s2obj1)
                {
                    s2obj1->m_f1 += 0.1f;
                    s2obj1->m_i1 += 2;
                    s2obj1->m_b1 = !s2obj1->m_b1;
                }
                if (s3obj1)
                {
                    s3obj1->m_f1 += 0.3f;
                    s3obj1->m_i1 += 3;
                    s3obj1->m_b1 = !s3obj1->m_b1;
                }
            }
            ++tick;
            // tick everything
            for (int i = 0; i < nSessions; ++i)
            {
                sessions[i].Update();
                sessions[i].GetReplicaMgr().Unmarshal();
            }
            for (int i = 0; i < nSessions; ++i)
            {
                sessions[i].GetReplicaMgr().UpdateReplicas();
            }
            for (int i = 0; i < nSessions; ++i)
            {
                sessions[i].GetReplicaMgr().UpdateFromReplicas();
                sessions[i].GetReplicaMgr().Marshal();
            }
            for (int i = 0; i < nSessions; ++i)
            {
                sessions[i].GetTransport()->Update();
            }
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(k_delay));
        }

        for (int i = 0; i < nSessions; ++i)
        {
            sessions[i].GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(sessions[i].GetTransport());
        }
    }

class ForcedReplicaMigrationTest
    : public UnitTest::GridMateMPTestFixture
    , public ReplicaMgrCallbackBus::Handler
    , public MigratableReplica::MigratableReplicaDebugMsgs::EBus::Handler
    , public ::testing::Test
{
    void OnNewHost(bool isHost, ReplicaManager* pMgr) override
    {
        if (isHost)
        {
            AZ_TracePrintf("GridMate", "Peer %d has completed host migration and is now the host.\n", (int)pMgr->GetLocalPeerId());
            pMgr->SetSendTimeInterval(k_hostSendRateMs);
            m_newHostEventOnNewHostCount++;
        }
        else
        {
            AZ_TracePrintf("GridMate", "Peer %d has has received notification that host migration is complete.\n", (int)pMgr->GetLocalPeerId());
            pMgr->SetSendTimeInterval(0);
            m_newHostEventOnPeersCount++;
        }
    }

    void OnNewOwner(ReplicaId repId, ReplicaManager* repMgr) override
    {
        AZ_TracePrintf("GridMate", "Replica 0x%08x got new owner %d on frame %d.\n", repId, (int)repMgr->GetLocalPeerId(), m_frameCount);
        m_replicaOwnership[repId] = repMgr;
    }

public:
    ForcedReplicaMigrationTest()    { ReplicaMgrCallbackBus::Handler::BusConnect(m_gridMate); }
    ~ForcedReplicaMigrationTest()   { ReplicaMgrCallbackBus::Handler::BusDisconnect(); }


    enum
    {
        p1, p2, p3, p4, p5, nPeers
    };

    static const int k_frameTimePerNodeMs = 10;
    static const int k_numFramesToRun = 300;
    static const int k_hostSendRateMs = k_frameTimePerNodeMs * nPeers * 2; // limiting host send rate x2 times

    int m_frameCount;
    int m_newHostEventOnNewHostCount;
    int m_newHostEventOnPeersCount;
    AZStd::unordered_map<ReplicaId, ReplicaManager*> m_replicaOwnership;
};

const int ForcedReplicaMigrationTest::k_frameTimePerNodeMs;
const int ForcedReplicaMigrationTest::k_numFramesToRun;
const int ForcedReplicaMigrationTest::k_hostSendRateMs;

TEST_F(ForcedReplicaMigrationTest, DISABLED_ForcedReplicaMigrationTest)
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<MigratableReplica, MigratableReplica::Descriptor>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<NonMigratableReplica>();

        MPSession                   peers[nPeers];
        MigratableReplica::Ptr      migrRep[nPeers];
        NonMigratableReplica::Ptr   nonMigrRep[nPeers];

        m_newHostEventOnNewHostCount = 0;
        m_newHostEventOnPeersCount = 0;

        MigratableReplica::MigratableReplicaDebugMsgs::EBus::Handler::BusConnect();

        // initialize full-mesh P2P session
        int basePort = 4427;
        for (int i = 0; i < nPeers; ++i)
        {
        TestCarrierDesc desc;
            desc.m_port = basePort + i;
            desc.m_enableDisconnectDetection = /*false*/ true;
            desc.m_threadUpdateTimeMS = k_frameTimePerNodeMs / 2;

            // initialize replica managers
            peers[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            peers[i].AcceptConn(true);
            peers[i].SetClient(false);
            peers[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1
                    , peers[i].GetTransport()
                    , 0
                    , i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0
                    , i == 0 ? k_hostSendRateMs : 0));
        }

        AZ_TracePrintf("GridMate", "\n");
        m_frameCount = 0;
        while (m_frameCount < k_numFramesToRun)
        {
            static bool allReady = false;
            // establish all connections
            if (m_frameCount < nPeers)
            {
                for (int i = 0; i < m_frameCount; ++i)
                {
                    peers[m_frameCount].GetTransport()->Connect("127.0.0.1", basePort + i);
                }
            }

            if (!allReady)
            {
                allReady = true;
                for (int i = 0; i < nPeers; ++i)
                {
                    if (!peers[i].GetReplicaMgr().IsReady())
                    {
                        allReady = false;
                    }
                }
                if (allReady)
                {
                    AZ_TracePrintf("GridMate", "All peers ready at frame %d\n", m_frameCount);
                }
            }

            // perform tests
            if (allReady)
            {
                // add replicas
                static bool addReplicas = true;
                if (addReplicas)
                {
                    for (int i = 0; i < nPeers; ++i)
                    {
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            migrRep[i] = CreateAndAttachReplicaChunk<MigratableReplica>(rep, aznew MyObj());
                            peers[i].GetReplicaMgr().AddPrimary(rep);
                            AZ_TEST_ASSERT(m_replicaOwnership[migrRep[i]->GetReplicaId()] == &peers[i].GetReplicaMgr());
                        }
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            nonMigrRep[i] = CreateAndAttachReplicaChunk<NonMigratableReplica>(rep, aznew MyObj());
                            peers[i].GetReplicaMgr().AddPrimary(rep);
                        }
                    }
                    addReplicas = false;
                    AZ_TracePrintf("GridMate", "Replicas added at frame %d\n", m_frameCount);
                }

                // disconnect p3 and trigger peer migration
                static bool dropP3 = true;
                if (m_frameCount > 50 && dropP3)
                {
                    peers[p3].GetTransport()->Disconnect(AllConnections);
                    dropP3 = false;
                    AZ_TracePrintf("GridMate", "Dropped P3 at frame %d\n", m_frameCount);
                }

                // Check that p3's MigratableReplica has migrated to p1 (host)
                if (m_frameCount == 85)
                {
                    AZ_TEST_ASSERT(m_replicaOwnership[migrRep[p3]->GetReplicaId()] == &peers[p1].GetReplicaMgr());
                }

                // disconnect p1 and trigger host loss
                static bool dropP1 = true;
                if (m_frameCount > 100 && dropP1)
                {
                    peers[p1].GetTransport()->Disconnect(AllConnections);
                    dropP1 = false;
                    AZ_TracePrintf("GridMate", "Dropped P1 at frame %d\n", m_frameCount);
                }

                // promote p2 to host
                static bool promoteP2 = true;
                if (m_frameCount > 150 && promoteP2)
                {
                    peers[p2].GetReplicaMgr().Promote();
                    promoteP2 = false;
                    AZ_TracePrintf("GridMate", "Promoted P2 at frame %d\n", m_frameCount);
                }
            }

            // tick
            int tickPeer = m_frameCount++ % nPeers;
            peers[tickPeer].Update();
            peers[tickPeer].GetReplicaMgr().Unmarshal();
            peers[tickPeer].GetReplicaMgr().UpdateReplicas();
            peers[tickPeer].GetReplicaMgr().UpdateFromReplicas();
            peers[tickPeer].GetReplicaMgr().Marshal();
            peers[tickPeer].GetTransport()->Update();
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(k_frameTimePerNodeMs));
        }

        // Check that p1's MigratableReplicas (including the one from p3) have migrated to p2 (host)
        AZ_TEST_ASSERT(m_replicaOwnership[migrRep[p1]->GetReplicaId()] == &peers[p2].GetReplicaMgr());
        AZ_TEST_ASSERT(m_replicaOwnership[migrRep[p3]->GetReplicaId()] == &peers[p2].GetReplicaMgr());

        AZ_TEST_ASSERT(m_newHostEventOnNewHostCount == 1);  // New host should have received OnNewHost event
        AZ_TEST_ASSERT(m_newHostEventOnPeersCount == 2);    // 2 peers remaining should have received OnNewHost event

        // clean up
        for (int i = 0; i < nPeers; ++i)
        {
            peers[i].GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(peers[i].GetTransport());
        }

        MigratableReplica::MigratableReplicaDebugMsgs::EBus::Handler::BusDisconnect();
    }

class ReplicaMigrationRequestTest
    : public UnitTest::GridMateMPTestFixture
    , public ::testing::Test
{
public:
    enum
    {
        Host,
        Peer1,
        Peer2,
        Client1,
        Client2,
        TotalNodes
    };

    class AlwaysMigratable
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(AlwaysMigratable);
        typedef AZStd::intrusive_ptr<AlwaysMigratable> Ptr;
        static const char* GetChunkName() { return "AlwaysMigratable"; }

        AlwaysMigratable()
            : UpdateControlValue("UpdateControlValue")
            , m_requests(0)
            , m_accepted(0)
            , m_triggerNextTransfer(false)
            , m_owner("Owner")
            , m_control("Control")
        {
        }

        bool IsReplicaMigratable() override
        {
            return true;
        }

        bool AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc) override
        {
            AZ_TracePrintf("GridMate", "Node %d accepted transfer of AlwaysMigratable 0x%x to node %d.\n", rc.m_rm->GetLocalPeerId() - 1, GetReplicaId(), requestor - 1);
            m_requests++;
            m_accepted++;

            if (rc.m_rm->GetLocalPeerId() - 1 == Peer2 && requestor - 1 == Host)
            {
                m_triggerNextTransfer = true;
            }

            return true;
        }

        void OnReplicaActivate(const ReplicaContext& rc) override
        {
            if (IsPrimary())
            {
                m_owner.Set(rc.m_rm->GetLocalPeerId() - 1);
                m_control.Set(rc.m_rm->GetLocalPeerId() - 1);
            }
        }

        void OnReplicaChangeOwnership(const ReplicaContext& rc) override
        {
            if (IsPrimary())
            {
                AZ_TracePrintf("GridMate", "OnChangeOwnership: 0x%04x Became primary on node %d\n", GetReplicaId(), rc.m_rm->GetLocalPeerId() - 1);
                m_owner.Set(rc.m_rm->GetLocalPeerId() - 1);
            }
            else
            {
                AZ_TracePrintf("GridMate", "OnChangeOwnership: 0x%04x Became proxy on node %d\n", GetReplicaId(), rc.m_rm->GetLocalPeerId() - 1);
                if (m_triggerNextTransfer)
                {
                    GetReplica()->RequestChangeOwnership(Client2 + 1);
                    m_triggerNextTransfer = false;
                }
            }
        }

        bool UpdateControlValueFn(const RpcContext& rc)
        {
            (void)rc;
            m_control.Set(GetReplicaManager()->GetLocalPeerId() - 1);
            return false;
        }
        Rpc<>::BindInterface<AlwaysMigratable, & AlwaysMigratable::UpdateControlValueFn> UpdateControlValue;

        int m_requests;
        int m_accepted;
        bool m_triggerNextTransfer;

        DataSet<int> m_owner;
        DataSet<int> m_control;
    };

    class NeverMigratable
        : public AlwaysMigratable
    {
    public:
        GM_CLASS_ALLOCATOR(NeverMigratable);
        typedef AZStd::intrusive_ptr<NeverMigratable> Ptr;
        static const char* GetChunkName() { return "NeverMigratable"; }

        NeverMigratable()
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }
    };

    class SometimesMigratable
        : public AlwaysMigratable
    {
    public:
        GM_CLASS_ALLOCATOR(SometimesMigratable);
        typedef AZStd::intrusive_ptr<SometimesMigratable> Ptr;
        static const char* GetChunkName() { return "SometimesMigratable"; }

        SometimesMigratable()
        {
            m_acceptMigrationRequests = false;
        }

        virtual bool AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc) override
        {
            (void)requestor;
            (void)rc;
            m_requests++;
            if (m_acceptMigrationRequests)
            {
                AZ_TracePrintf("GridMate", "Node %d accepted transfer of SometimesMigratable 0x%x to node %d.\n", rc.m_rm->GetLocalPeerId() - 1, GetReplicaId(), requestor - 1);
                m_accepted++;
                return true;
            }
            return false;
        }

        bool m_acceptMigrationRequests;
    };

    struct Node
    {
        MPSession                   m_session;
        AlwaysMigratable::Ptr       m_always;
        NeverMigratable::Ptr        m_never;
        SometimesMigratable::Ptr    m_sometimes;
    };


    static const int k_frameTimePerNodeMs = 10;
    static const int k_hostSendTimeMs = k_frameTimePerNodeMs * TotalNodes * 4; // limiting host send rate to be x4 times slower than tick
};

TEST_F(ReplicaMigrationRequestTest, DISABLED_ReplicaMigrationRequestTest)
    {
        /*
        Topology:
            P1---P2
             \   /
              \ /
               H
              / \
             /   \
            C1   C2

        Migration pattern:
            AlwaysMigratable:
                P1 -> P2
                P2 -> Host -> C2 (both at same time, with C2 arriving second)
                Host -> C1
                C1 -> C2 -> Host
                C2 -> P1
            NeverMigratable:
                P1 -> C1 (Forbidden)
                C2 -> P2 (Forbidden)
            SometimesMigratable:
                P1 -> Host
                C1 -> P1
                P2 -> C2 (Forbidden)
                C2 -> Host (Forbidden)
        */

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<AlwaysMigratable>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<NeverMigratable>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<SometimesMigratable>();

        Node   nodes[TotalNodes];
        int basePort = 4427;
        for (int i = 0; i < TotalNodes; ++i)
        {
        TestCarrierDesc desc;
            desc.m_port = basePort + i;
            desc.m_connectionTimeoutMS = 15000;
            // initialize replica managers
            nodes[i].m_session.SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            nodes[i].m_session.AcceptConn(true);
            nodes[i].m_session.SetClient(i == Client1 || i == Client2);
            nodes[i].m_session.GetReplicaMgr().Init(ReplicaMgrDesc(i + 1
                    , nodes[i].m_session.GetTransport()
                    , 0
                    , i == Host ? ReplicaMgrDesc::Role_SyncHost : 0
                    , i == Host ? k_hostSendTimeMs : 0));
        }

        // Connect all the nodes
        nodes[Peer1].m_session.GetTransport()->Connect("127.0.0.1", basePort + Host);
        nodes[Peer2].m_session.GetTransport()->Connect("127.0.0.1", basePort + Host);
        nodes[Client1].m_session.GetTransport()->Connect("127.0.0.1", basePort + Host);
        nodes[Client2].m_session.GetTransport()->Connect("127.0.0.1", basePort + Host);
        nodes[Peer1].m_session.GetTransport()->Connect("127.0.0.1", basePort + Peer2);


        int framesToRun = 800;
        for (int iTick = 0; iTick < framesToRun; ++iTick)
        {
            for (int iNode = 0; iNode < TotalNodes; ++iNode)
            {
                if (nodes[iNode].m_session.GetReplicaMgr().IsReady())
                {
                    if (nodes[iNode].m_always == nullptr)
                    {
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            nodes[iNode].m_always = CreateAndAttachReplicaChunk<AlwaysMigratable>(rep);
                            nodes[iNode].m_session.GetReplicaMgr().AddPrimary(rep);
                        }
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            nodes[iNode].m_never = CreateAndAttachReplicaChunk<NeverMigratable>(rep);
                            nodes[iNode].m_session.GetReplicaMgr().AddPrimary(rep);
                        }
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            nodes[iNode].m_sometimes = CreateAndAttachReplicaChunk<SometimesMigratable>(rep);
                            nodes[iNode].m_sometimes->m_acceptMigrationRequests = iNode == Peer1 || iNode == Client1;
                            nodes[iNode].m_session.GetReplicaMgr().AddPrimary(rep);
                        }
                    }
                }
            }

            // First round of migrations
            if (iTick == 200)
            {
                // P1 -> P2
                ReplicaPtr aP1onP2 = nodes[Peer2].m_session.GetReplicaMgr().FindReplica(nodes[Peer1].m_always->GetReplicaId());
                AZ_TEST_ASSERT(aP1onP2);
                aP1onP2->RequestChangeOwnership();

                // P2 -> Host -> C2 (both at same time, with C2 arriving second)
                ReplicaPtr aP2onH = nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Peer2].m_always->GetReplicaId());
                AZ_TEST_ASSERT(aP2onH);
                aP2onH->RequestChangeOwnership();

                // Host -> C1
                ReplicaPtr aHonC1 = nodes[Client1].m_session.GetReplicaMgr().FindReplica(nodes[Host].m_always->GetReplicaId());
                AZ_TEST_ASSERT(aHonC1);
                aHonC1->RequestChangeOwnership();

                // C1 -> C2 -> Host (first migration)
                ReplicaPtr aC1onC2 = nodes[Client2].m_session.GetReplicaMgr().FindReplica(nodes[Client1].m_always->GetReplicaId());
                AZ_TEST_ASSERT(aC1onC2);
                aC1onC2->RequestChangeOwnership();

                // C2 -> P1
                ReplicaPtr aC2onP1 = nodes[Peer1].m_session.GetReplicaMgr().FindReplica(nodes[Client2].m_always->GetReplicaId());
                AZ_TEST_ASSERT(aC2onP1);
                aC2onP1->RequestChangeOwnership();

                // P1 -> C1 (Forbidden)
                ReplicaPtr nP1onC1 = nodes[Client1].m_session.GetReplicaMgr().FindReplica(nodes[Peer1].m_never->GetReplicaId());
                AZ_TEST_ASSERT(nP1onC1);
                nP1onC1->RequestChangeOwnership();

                // C2 -> P2 (Forbidden)
                ReplicaPtr nC2onP2 = nodes[Peer2].m_session.GetReplicaMgr().FindReplica(nodes[Client2].m_never->GetReplicaId());
                AZ_TEST_ASSERT(nC2onP2);
                nC2onP2->RequestChangeOwnership();

                // P1 -> Host
                ReplicaPtr sP1onH = nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Peer1].m_sometimes->GetReplicaId());
                AZ_TEST_ASSERT(sP1onH);
                sP1onH->RequestChangeOwnership();

                // C1 -> P1
                ReplicaPtr sC1onP1 = nodes[Peer1].m_session.GetReplicaMgr().FindReplica(nodes[Client1].m_sometimes->GetReplicaId());
                AZ_TEST_ASSERT(sC1onP1);
                sC1onP1->RequestChangeOwnership();

                // P2 -> C2 (Forbidden)
                ReplicaPtr sP2onC2 = nodes[Client2].m_session.GetReplicaMgr().FindReplica(nodes[Peer2].m_sometimes->GetReplicaId());
                AZ_TEST_ASSERT(sP2onC2);
                sP2onC2->RequestChangeOwnership();

                // C2 -> Host (Forbidden)
                ReplicaPtr sC2onH = nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Client2].m_sometimes->GetReplicaId());
                AZ_TEST_ASSERT(sC2onH);
                sC2onH->RequestChangeOwnership();
            }

            // Second round of migrations
            if (iTick == 400)
            {
                // C1 -> C2 -> Host (1st migration)
                AZ_TEST_ASSERT(nodes[Client1].m_always->m_requests == 1);
                AZ_TEST_ASSERT(nodes[Client1].m_always->m_accepted == 1);
                AZ_TEST_ASSERT(nodes[Client1].m_always->GetReplica()->IsProxy());
                AZ_TEST_ASSERT(nodes[Client1].m_always->m_owner.Get() == Client2);
                AZ_TEST_ASSERT(nodes[Client2].m_session.GetReplicaMgr().FindReplica(nodes[Client1].m_always->GetReplicaId())->IsPrimary());

                // C1 -> C2 -> Host (2nd migration)
                ReplicaPtr aHonC1 = nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Client1].m_always->GetReplicaId());
                AZ_TEST_ASSERT(aHonC1);
                aHonC1->RequestChangeOwnership();
            }

            // Send non-authoritative control RPCs
            if (iTick == 600)
            {
                // P1 -> P2
                AlwaysMigratable::Ptr aP1onH = nodes[Host].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Peer1].m_always->GetReplicaId());
                aP1onH->UpdateControlValue();
                // P2 -> Host -> C2 (both at same time, with C2 arriving second)
                AlwaysMigratable::Ptr aP2onC1 = nodes[Client1].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Peer2].m_always->GetReplicaId());
                aP2onC1->UpdateControlValue();
                // Host -> C1
                AlwaysMigratable::Ptr aHonP1 = nodes[Peer1].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Host].m_always->GetReplicaId());
                aHonP1->UpdateControlValue();
                // C1 -> C2 -> Host
                AlwaysMigratable::Ptr aC1onP2 = nodes[Peer2].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Client1].m_always->GetReplicaId());
                aC1onP2->UpdateControlValue();
                // C2 -> P1
                AlwaysMigratable::Ptr aC2onP2 = nodes[Peer2].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Client2].m_always->GetReplicaId());
                aC2onP2->UpdateControlValue();
                // P1 -> C1 (Forbidden)
                NeverMigratable::Ptr nP1onH = nodes[Host].m_session.GetChunkFromReplica<NeverMigratable>(nodes[Peer1].m_never->GetReplicaId());
                nP1onH->UpdateControlValue();
                // C2 -> P2 (Forbidden)
                NeverMigratable::Ptr nC2onP1 = nodes[Peer1].m_session.GetChunkFromReplica<NeverMigratable>(nodes[Client2].m_never->GetReplicaId());
                nC2onP1->UpdateControlValue();
                // P1 -> Host
                SometimesMigratable::Ptr sP1onH = nodes[Host].m_session.GetChunkFromReplica<SometimesMigratable>(nodes[Peer1].m_sometimes->GetReplicaId());
                sP1onH->UpdateControlValue();
                // C1 -> P1
                SometimesMigratable::Ptr sC1onC2 = nodes[Client2].m_session.GetChunkFromReplica<SometimesMigratable>(nodes[Client1].m_sometimes->GetReplicaId());
                sC1onC2->UpdateControlValue();
                // P2 -> C2 (Forbidden)
                SometimesMigratable::Ptr sP2onC2 = nodes[Client2].m_session.GetChunkFromReplica<SometimesMigratable>(nodes[Peer2].m_sometimes->GetReplicaId());
                sP2onC2->UpdateControlValue();
                // C2 -> Host (Forbidden)
                SometimesMigratable::Ptr sC2onP1 = nodes[Peer1].m_session.GetChunkFromReplica<SometimesMigratable>(nodes[Client2].m_sometimes->GetReplicaId());
                sC2onP1->UpdateControlValue();
            }

            // tick
            int tickNode = iTick % TotalNodes;
            nodes[tickNode].m_session.Update();
            nodes[tickNode].m_session.GetReplicaMgr().Unmarshal();
            nodes[tickNode].m_session.GetReplicaMgr().UpdateFromReplicas();
            nodes[tickNode].m_session.GetReplicaMgr().UpdateReplicas();
            nodes[tickNode].m_session.GetReplicaMgr().Marshal();
            nodes[tickNode].m_session.GetTransport()->Update();
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(k_frameTimePerNodeMs));
        }

        // P1 -> P2
        AZ_TEST_ASSERT(nodes[Peer1].m_always->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Peer1].m_always->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Peer1].m_always->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Peer1].m_always->m_owner.Get() == Peer2);
        AZ_TEST_ASSERT(nodes[Peer2].m_session.GetReplicaMgr().FindReplica(nodes[Peer1].m_always->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Peer1].m_always->m_control.Get() == Peer2);

        // P2 -> Host -> C2 (both at same time, with C2 arriving second)
        AZ_TEST_ASSERT(nodes[Peer2].m_always->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Peer2].m_always->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Peer2].m_always->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Peer2].m_always->m_owner.Get() == Client2);
        AlwaysMigratable::Ptr aP2onH = nodes[Host].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Peer2].m_always->GetReplicaId());
        AZ_TEST_ASSERT(aP2onH->m_requests == 1);
        AZ_TEST_ASSERT(aP2onH->m_accepted == 1);
        AZ_TEST_ASSERT(aP2onH->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(aP2onH->m_owner.Get() == Client2);
        AZ_TEST_ASSERT(nodes[Client2].m_session.GetReplicaMgr().FindReplica(nodes[Peer2].m_always->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Peer2].m_always->m_control.Get() == Client2);

        // Host -> C1
        AZ_TEST_ASSERT(nodes[Host].m_always->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Host].m_always->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Host].m_always->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Host].m_always->m_owner.Get() == Client1);
        AZ_TEST_ASSERT(nodes[Client1].m_session.GetReplicaMgr().FindReplica(nodes[Host].m_always->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Host].m_always->m_control.Get() == Client1);

        // C1 -> C2 -> Host (2nd migration)
        AZ_TEST_ASSERT(nodes[Client1].m_always->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Client1].m_always->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Client1].m_always->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Client1].m_always->m_owner.Get() == Host);
        AlwaysMigratable::Ptr aC1onC2 = nodes[Client2].m_session.GetChunkFromReplica<AlwaysMigratable>(nodes[Client1].m_always->GetReplicaId());
        AZ_TEST_ASSERT(aC1onC2->m_requests == 1);
        AZ_TEST_ASSERT(aC1onC2->m_accepted == 1);
        AZ_TEST_ASSERT(aC1onC2->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(aC1onC2->m_owner.Get() == Host);
        AZ_TEST_ASSERT(nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Client1].m_always->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Client1].m_always->m_control.Get() == Host);

        // C2 -> P1
        AZ_TEST_ASSERT(nodes[Client2].m_always->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Client2].m_always->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Client2].m_always->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Client2].m_always->m_owner.Get() == Peer1);
        AZ_TEST_ASSERT(nodes[Peer1].m_session.GetReplicaMgr().FindReplica(nodes[Client2].m_always->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Client2].m_always->m_control.Get() == Peer1);

        // P1 -> C1 (Forbidden)
        AZ_TEST_ASSERT(nodes[Peer1].m_never->m_requests == 0);
        AZ_TEST_ASSERT(nodes[Peer1].m_never->m_accepted == 0);
        AZ_TEST_ASSERT(nodes[Peer1].m_never->GetReplica()->IsPrimary());
        AZ_TEST_ASSERT(nodes[Peer1].m_never->m_owner.Get() == Peer1);
        AZ_TEST_ASSERT(nodes[Client1].m_session.GetReplicaMgr().FindReplica(nodes[Peer1].m_never->GetReplicaId())->IsProxy());
        AZ_TEST_ASSERT(nodes[Peer1].m_never->m_control.Get() == Peer1);

        // C2 -> P2 (Forbidden)
        AZ_TEST_ASSERT(nodes[Client2].m_never->m_requests == 0);
        AZ_TEST_ASSERT(nodes[Client2].m_never->m_accepted == 0);
        AZ_TEST_ASSERT(nodes[Client2].m_never->GetReplica()->IsPrimary());
        AZ_TEST_ASSERT(nodes[Client2].m_never->m_owner.Get() == Client2);
        AZ_TEST_ASSERT(nodes[Peer2].m_session.GetReplicaMgr().FindReplica(nodes[Client2].m_never->GetReplicaId())->IsProxy());
        AZ_TEST_ASSERT(nodes[Client2].m_never->m_control.Get() == Client2);

        // P1 -> Host
        AZ_TEST_ASSERT(nodes[Peer1].m_sometimes->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Peer1].m_sometimes->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Peer1].m_sometimes->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Peer1].m_sometimes->m_owner.Get() == Host);
        AZ_TEST_ASSERT(nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Peer1].m_sometimes->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Peer1].m_sometimes->m_control.Get() == Host);

        // C1 -> P1
        AZ_TEST_ASSERT(nodes[Client1].m_sometimes->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Client1].m_sometimes->m_accepted == 1);
        AZ_TEST_ASSERT(nodes[Client1].m_sometimes->GetReplica()->IsProxy());
        AZ_TEST_ASSERT(nodes[Client1].m_sometimes->m_owner.Get() == Peer1);
        AZ_TEST_ASSERT(nodes[Peer1].m_session.GetReplicaMgr().FindReplica(nodes[Client1].m_sometimes->GetReplicaId())->IsPrimary());
        AZ_TEST_ASSERT(nodes[Client1].m_sometimes->m_control.Get() == Peer1);

        // P2 -> C2 (Forbidden)
        AZ_TEST_ASSERT(nodes[Peer2].m_sometimes->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Peer2].m_sometimes->m_accepted == 0);
        AZ_TEST_ASSERT(nodes[Peer2].m_sometimes->GetReplica()->IsPrimary());
        AZ_TEST_ASSERT(nodes[Peer2].m_sometimes->m_owner.Get() == Peer2);
        AZ_TEST_ASSERT(nodes[Client2].m_session.GetReplicaMgr().FindReplica(nodes[Peer2].m_never->GetReplicaId())->IsProxy());
        AZ_TEST_ASSERT(nodes[Peer2].m_sometimes->m_control.Get() == Peer2);

        // C2 -> Host (Forbidden)
        AZ_TEST_ASSERT(nodes[Client2].m_sometimes->m_requests == 1);
        AZ_TEST_ASSERT(nodes[Client2].m_sometimes->m_accepted == 0);
        AZ_TEST_ASSERT(nodes[Client2].m_sometimes->GetReplica()->IsPrimary());
        AZ_TEST_ASSERT(nodes[Client2].m_sometimes->m_owner.Get() == Client2);
        AZ_TEST_ASSERT(nodes[Host].m_session.GetReplicaMgr().FindReplica(nodes[Client2].m_never->GetReplicaId())->IsProxy());
        AZ_TEST_ASSERT(nodes[Client2].m_sometimes->m_control.Get() == Client2);

        // clean up
        for (int i = 0; i < TotalNodes; ++i)
        {
            nodes[i].m_always = nullptr;
            nodes[i].m_never = nullptr;
            nodes[i].m_sometimes = nullptr;
            nodes[i].m_session.GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(nodes[i].m_session.GetTransport());
        }
    }

const int ReplicaMigrationRequestTest::k_frameTimePerNodeMs;
const int ReplicaMigrationRequestTest::k_hostSendTimeMs;


class PeerRejoinTest
    : public UnitTest::GridMateMPTestFixture
    , public ReplicaMgrCallbackBus::Handler
    , public ::testing::Test
{
    void OnNewHost(bool isHost, ReplicaManager* pMgr) override
    {
        (void)pMgr;
        if (isHost)
        {
            AZ_TracePrintf("GridMate", "Peer %d has completed host migration and is now the host.\n", (int)pMgr->GetLocalPeerId());
        }
        else
        {
            AZ_TracePrintf("GridMate", "Peer %d has has received notification that host migration is complete.\n", (int)pMgr->GetLocalPeerId());
        }
    }

public:
    PeerRejoinTest()    { ReplicaMgrCallbackBus::Handler::BusConnect(m_gridMate); }
    ~PeerRejoinTest()   { ReplicaMgrCallbackBus::Handler::BusDisconnect(); }
};

TEST_F(PeerRejoinTest, DISABLED_PeerRejoinTest)
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<MigratableReplica, MigratableReplica::Descriptor>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<NonMigratableReplica>();

        int frameTime = 10;
        int framesToRun = 300;
        enum
        {
            p1, p2, nPeers
        };

        MPSession                   peers[nPeers];
        MigratableReplica::Ptr      migrRep[nPeers];
        NonMigratableReplica::Ptr   nonMigrRep[nPeers];

        // initialize full-mesh P2P session
        int basePort = 4427;
        for (int i = 0; i < nPeers; ++i)
        {
        TestCarrierDesc desc;
            desc.m_port = basePort + i;
            desc.m_enableDisconnectDetection = true;
            desc.m_threadUpdateTimeMS = frameTime / 2;

            // initialize replica managers
            peers[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            peers[i].AcceptConn(true);
            peers[i].SetClient(false);
            peers[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1
                    , peers[i].GetTransport()
                    , 0
                    , i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0
                    , frameTime / 2));
        }

        AZ_TracePrintf("GridMate", "\n");
        int frameCount = 0;
        while (frameCount < framesToRun)
        {
            static bool allReady = false;
            // establish all connections
            if (frameCount < nPeers)
            {
                for (int i = 0; i < frameCount; ++i)
                {
                    peers[frameCount].GetTransport()->Connect("127.0.0.1", basePort + i);
                }
            }

            if (!allReady)
            {
                allReady = true;
                for (int i = 0; i < nPeers; ++i)
                {
                    if (!peers[i].GetReplicaMgr().IsReady())
                    {
                        allReady = false;
                    }
                }
                if (allReady)
                {
                    AZ_TracePrintf("GridMate", "All peers ready at frame %d\n", frameCount);
                }
            }

            // perform tests
            if (allReady)
            {
                // add replicas
                static bool addReplicas = true;
                if (addReplicas)
                {
                    for (int i = 0; i < nPeers; ++i)
                    {
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            migrRep[i] = CreateAndAttachReplicaChunk<MigratableReplica>(rep, aznew MyObj());
                            peers[i].GetReplicaMgr().AddPrimary(rep);
                        }
                        {
                            auto rep = Replica::CreateReplica(nullptr);
                            nonMigrRep[i] = CreateAndAttachReplicaChunk<NonMigratableReplica>(rep, aznew MyObj());
                            peers[i].GetReplicaMgr().AddPrimary(rep);
                        }
                    }
                    addReplicas = false;
                    AZ_TracePrintf("GridMate", "Replicas added at frame %d\n", frameCount);
                }

                // disconnect p2 and trigger peer migration
                static bool dropP2 = true;
                if (frameCount > 50 && dropP2)
                {
                    peers[p2].GetTransport()->Disconnect(AllConnections);
                    peers[p2].GetReplicaMgr().Shutdown();
                    dropP2 = false;
                    AZ_TracePrintf("GridMate", "Dropped P2 at frame %d\n", frameCount);
                }

                // reconnect p2
                static bool reconP2 = true;
                if (frameCount > 100 && reconP2)
                {
                    peers[p2].GetReplicaMgr().Init(ReplicaMgrDesc(p2 + 1
                            , peers[p2].GetTransport()
                            , 0
                            , 0
                            , frameTime / 2));
                    peers[p1].GetTransport()->Connect("127.0.0.1", basePort + p2);
                    peers[p2].GetTransport()->Connect("127.0.0.1", basePort + p1);
                    reconP2 = false;
                    AZ_TracePrintf("GridMate", "Reconnected P2 at frame %d\n", frameCount);
                }

                // disconnect p2 again
                static bool redropP2 = true;
                if (frameCount > 150 && redropP2)
                {
                    peers[p2].GetTransport()->Disconnect(AllConnections);
                    redropP2 = false;
                    AZ_TracePrintf("GridMate", "Re-Dropped P2 at frame %d\n", frameCount);
                }
            }

            // tick
            int tickPeer = frameCount++ % nPeers;
            peers[tickPeer].Update();
            if (peers[tickPeer].GetReplicaMgr().IsInitialized())
            {
                peers[tickPeer].GetReplicaMgr().Unmarshal();
                peers[tickPeer].GetReplicaMgr().UpdateReplicas();
                peers[tickPeer].GetReplicaMgr().UpdateFromReplicas();
                peers[tickPeer].GetReplicaMgr().Marshal();
            }
            peers[tickPeer].GetTransport()->Update();
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(frameTime));
        }

        // clean up
        for (int i = 0; i < nPeers; ++i)
        {
            peers[i].GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(peers[i].GetTransport());
        }
    }

class ReplicationSecurityOptionsTest
    : public UnitTest::GridMateMPTestFixture
    , public ::testing::Test
{
public:
    enum
    {
        s1,
        s2,
        s3,
        nSessions
    };

    class TestChunk : public ReplicaChunk
    {
    public:
        struct ForwardSourcePeerTrait : public RpcDefaultTraits
        {
            static const bool s_alwaysForwardSourcePeer = true;
            static const bool s_allowNonAuthoritativeRequestRelay = false;
        };

        struct DisableNonAuthoritativeRequestTrait : public RpcAuthoritativeTraits
        {
            static const bool s_alwaysForwardSourcePeer = true;
        };

        GM_CLASS_ALLOCATOR(TestChunk);

        static const char* GetChunkName() { return "ReplicationSecurityOptionsTest::TestChunk"; }

        TestChunk()
            : m_nForwardSourcePeerRpcCallsFromS1("m_nForwardSourcePeerRpcCallsFromS1", 0)
            , m_nForwardSourcePeerRpcCallsFromS2("m_nForwardSourcePeerRpcCallsFromS2", 0)
            , m_nForwardSourcePeerRpcCallsFromS3("m_nForwardSourcePeerRpcCallsFromS3", 0)
            , ForwardSourcePeerRpcFromS1("ForwardSourcePeerRpcFromS1")
            , ForwardSourcePeerRpcFromS2("ForwardSourcePeerRpcFromS2")
            , ForwardSourcePeerRpcFromS3("ForwardSourcePeerRpcFromS3")
            , m_nAuthoritativeOnlyRpcCallsFromS1("m_nAuthoritativeOnlyRpcCallsFromS1", 0)
            , m_nAuthoritativeOnlyRpcCallsFromS2("m_nAuthoritativeOnlyRpcCallsFromS2", 0)
            , m_nAuthoritativeOnlyRpcCallsFromS3("m_nAuthoritativeOnlyRpcCallsFromS3", 0)
            , m_nAuthoritativeOnlyProxyRpcCallsFromS1(0)
            , m_nAuthoritativeOnlyProxyRpcCallsFromS2(0)
            , m_nAuthoritativeOnlyProxyRpcCallsFromS3(0)
            , AuthoritativeOnlyRpcFromS1("AuthoritativeOnlyRpcFromS1")
            , AuthoritativeOnlyRpcFromS2("AuthoritativeOnlyRpcFromS2")
            , AuthoritativeOnlyRpcFromS3("AuthoritativeOnlyRpcFromS3")
        {
        }

        bool IsReplicaMigratable() override { return false; }

        bool OnForwardSourcePeerRpcFromS1(const RpcContext& rpcContext)
        {
            // make sure the requestor is set to s1
            AZ_TEST_ASSERT(rpcContext.m_sourcePeer == s1 + 1);
            m_nForwardSourcePeerRpcCallsFromS1.Modify([](int& value) { ++value; return true; });
            return false;
        }

        bool OnForwardSourcePeerRpcFromS2(const RpcContext& rpcContext)
        {
            // make sure the requestor is set to s2
            AZ_TEST_ASSERT(rpcContext.m_sourcePeer == s2 + 1);
            // requests to s3 should be blocked in this test
            AZ_TEST_ASSERT(GetReplicaManager()->GetLocalPeerId() != s3 + 1);
            m_nForwardSourcePeerRpcCallsFromS2.Modify([](int& value) { ++value; return true; });
            return false;
        }

        bool OnForwardSourcePeerRpcFromS3(const RpcContext& rpcContext)
        {
            // make sure the requestor is set to s3
            AZ_TEST_ASSERT(rpcContext.m_sourcePeer == s3 + 1);
            // requests to s2 should be blocked in this test
            AZ_TEST_ASSERT(GetReplicaManager()->GetLocalPeerId() != s2 + 1);
            m_nForwardSourcePeerRpcCallsFromS3.Modify([](int& value) { ++value; return true; });
            return false;
        }

        bool OnAuthoritativeOnlyRpcFromS1(const RpcContext& rpcContext)
        {
            // make sure the requestor is set to s1
            AZ_TEST_ASSERT(rpcContext.m_sourcePeer == s1 + 1);
            if (IsPrimary())
            {
                m_nAuthoritativeOnlyRpcCallsFromS1.Modify([](int& value) { ++value; return true; });
            }
            else
            {
                ++m_nAuthoritativeOnlyProxyRpcCallsFromS1;
            }
            return true;
        }

        bool OnAuthoritativeOnlyRpcFromS2(const RpcContext& rpcContext)
        {
            // make sure the requestor is set to s2
            AZ_TEST_ASSERT(rpcContext.m_sourcePeer == s2 + 1);
            if (IsPrimary())
            {
                m_nAuthoritativeOnlyRpcCallsFromS2.Modify([](int& value) { ++value; return true; });
            }
            else
            {
                ++m_nAuthoritativeOnlyProxyRpcCallsFromS2;
            }
            return true;
        }

        bool OnAuthoritativeOnlyRpcFromS3(const RpcContext& rpcContext)
        {
            // make sure the requestor is set to s3
            AZ_TEST_ASSERT(rpcContext.m_sourcePeer == s3 + 1);
            if (IsPrimary())
            {
                m_nAuthoritativeOnlyRpcCallsFromS3.Modify([](int& value) { ++value; return true; });
            }
            else
            {
                ++m_nAuthoritativeOnlyProxyRpcCallsFromS3;
            }
            return true;
        }

        DataSet<int> m_nForwardSourcePeerRpcCallsFromS1;
        DataSet<int> m_nForwardSourcePeerRpcCallsFromS2;
        DataSet<int> m_nForwardSourcePeerRpcCallsFromS3;
        Rpc<>::BindInterface<TestChunk, &TestChunk::OnForwardSourcePeerRpcFromS1, ForwardSourcePeerTrait> ForwardSourcePeerRpcFromS1;
        Rpc<>::BindInterface<TestChunk, &TestChunk::OnForwardSourcePeerRpcFromS2, ForwardSourcePeerTrait> ForwardSourcePeerRpcFromS2;
        Rpc<>::BindInterface<TestChunk, &TestChunk::OnForwardSourcePeerRpcFromS3, ForwardSourcePeerTrait> ForwardSourcePeerRpcFromS3;

        DataSet<int> m_nAuthoritativeOnlyRpcCallsFromS1;
        DataSet<int> m_nAuthoritativeOnlyRpcCallsFromS2;
        DataSet<int> m_nAuthoritativeOnlyRpcCallsFromS3;
        int m_nAuthoritativeOnlyProxyRpcCallsFromS1;
        int m_nAuthoritativeOnlyProxyRpcCallsFromS2;
        int m_nAuthoritativeOnlyProxyRpcCallsFromS3;
        Rpc<>::BindInterface<TestChunk, &TestChunk::OnAuthoritativeOnlyRpcFromS1, DisableNonAuthoritativeRequestTrait> AuthoritativeOnlyRpcFromS1;
        Rpc<>::BindInterface<TestChunk, &TestChunk::OnAuthoritativeOnlyRpcFromS2, DisableNonAuthoritativeRequestTrait> AuthoritativeOnlyRpcFromS2;
        Rpc<>::BindInterface<TestChunk, &TestChunk::OnAuthoritativeOnlyRpcFromS3, DisableNonAuthoritativeRequestTrait> AuthoritativeOnlyRpcFromS3;
    };
    using TestChunkPtr = AZStd::intrusive_ptr<TestChunk> ;
};

TEST_F(ReplicationSecurityOptionsTest, DISABLED_ReplicationSecurityOptionsTest)
    {
        AZ_TracePrintf("GridMate", "\n");

        // Register test chunks
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<TestChunk>();

        MPSession sessions[nSessions];
        ReplicaPtr primarys[nSessions];

        // initialize transport
        int basePort = 4427;
        for (int i = 0; i < nSessions; ++i)
        {
        TestCarrierDesc desc;
            desc.m_port = basePort + i;
            // initialize replica managers
            // s2(c)<-->(p)s1(p)<-->(c)s3
            sessions[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            sessions[i].AcceptConn(true);
            sessions[i].SetClient(i != s1);
            sessions[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1, sessions[i].GetTransport(), 0, i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0));

            ReplicationSecurityOptions options;
            options.m_enableStrictSourceValidation = true;
            sessions[i].GetReplicaMgr().SetSecurityOptions(options);
        }

        // connect s2 to s1
        sessions[s2].GetTransport()->Connect("127.0.0.1", basePort);
        
        // connect s3 to s1
        sessions[s3].GetTransport()->Connect("127.0.0.1", basePort);

        // main test loop
        for (int tick = 0; tick < 1000; ++tick)
        {
            if (tick == 100)
            {
                for (int i = 0; i < nSessions; ++i)
                {
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().IsReady());
                    primarys[i] = Replica::CreateReplica("ReplicationSecurityOptionsTest::TestReplica");
                    TestChunkPtr chunk = CreateReplicaChunk<TestChunk>();
                    primarys[i]->AttachReplicaChunk(chunk);
                    sessions[i].GetReplicaMgr().AddPrimary(primarys[i]);
                }
            }

            if (tick == 200)
            {
                AZ_TEST_START_TRACE_SUPPRESSION;
                for (int i = 0; i < nSessions; ++i)
                {
                    sessions[s1].GetReplicaMgr().FindReplica(primarys[i]->GetRepId())->FindReplicaChunk<TestChunk>()->ForwardSourcePeerRpcFromS1();
                    sessions[s2].GetReplicaMgr().FindReplica(primarys[i]->GetRepId())->FindReplicaChunk<TestChunk>()->ForwardSourcePeerRpcFromS2();
                    sessions[s3].GetReplicaMgr().FindReplica(primarys[i]->GetRepId())->FindReplicaChunk<TestChunk>()->ForwardSourcePeerRpcFromS3();
                }
            }

            if (tick == 300)
            {
                // The previous test should have triggered the following assert twice:
                // ReplicaChunk.cpp(449): AZ_Assert(false, "Discarding non-authoritative RPC <%s> because s_allowNonAuthoritativeRequestRelay trait is disabled!", GetDescriptor()->GetRpcName(this, rpc));
                AZ_TEST_STOP_TRACE_SUPPRESSION(2);

                // All chunks should have received the call from the host
                AZ_TEST_ASSERT(primarys[s1]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1 == 1);
                AZ_TEST_ASSERT(primarys[s2]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1 == 1);
                AZ_TEST_ASSERT(primarys[s3]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1 == 1);

                // the host chunk should have received calls from both clients
                AZ_TEST_ASSERT(primarys[s1]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2 == 1);
                AZ_TEST_ASSERT(primarys[s1]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3 == 1);

                // the chunk on s2 should receive its own call but not from s3
                AZ_TEST_ASSERT(primarys[s2]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2 == 1);
                AZ_TEST_ASSERT(primarys[s2]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3 == 0);

                // the chunk on s3 should receive its own call but not from s2
                AZ_TEST_ASSERT(primarys[s3]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2 == 0);
                AZ_TEST_ASSERT(primarys[s3]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3 == 1);

                // all datasets should have propagated properly
                for (int i = 0; i < nSessions; ++i)
                {
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1.Get() == primarys[s1]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2.Get() == primarys[s1]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3.Get() == primarys[s1]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3.Get());

                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1.Get() == primarys[s2]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2.Get() == primarys[s2]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3.Get() == primarys[s2]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3.Get());

                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1.Get() == primarys[s3]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS1.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2.Get() == primarys[s3]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS2.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3.Get() == primarys[s3]->FindReplicaChunk<TestChunk>()->m_nForwardSourcePeerRpcCallsFromS3.Get());
                }
            }

            if (tick == 400)
            {
                AZ_TEST_START_TRACE_SUPPRESSION;
                for (int i = 0; i < nSessions; ++i)
                {
                    sessions[s1].GetReplicaMgr().FindReplica(primarys[i]->GetRepId())->FindReplicaChunk<TestChunk>()->AuthoritativeOnlyRpcFromS1();
                    sessions[s2].GetReplicaMgr().FindReplica(primarys[i]->GetRepId())->FindReplicaChunk<TestChunk>()->AuthoritativeOnlyRpcFromS2();
                    sessions[s3].GetReplicaMgr().FindReplica(primarys[i]->GetRepId())->FindReplicaChunk<TestChunk>()->AuthoritativeOnlyRpcFromS3();
                }
            }

            if (tick == 500)
            {
                // The previous test should have triggered the following assert six times:
                // ReplicaChunk.cpp(444): AZ_Assert(false, "Discarding non-authoritative RPC <%s> because s_allowNonAuthoritativeRequests trait is disabled!", GetDescriptor()->GetRpcName(this, rpc));
                AZ_TEST_STOP_TRACE_SUPPRESSION(6);

                // Each chunk should have received their own AuthoritativeOnlyRpc once.
                AZ_TEST_ASSERT(primarys[s1]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1 == 1);
                AZ_TEST_ASSERT(primarys[s2]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2 == 1);
                AZ_TEST_ASSERT(primarys[s3]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3 == 1);

                // Calls from other nodes should have been discarded.
                AZ_TEST_ASSERT(primarys[s1]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2 == 0);
                AZ_TEST_ASSERT(primarys[s1]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3 == 0);
                AZ_TEST_ASSERT(primarys[s2]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1 == 0);
                AZ_TEST_ASSERT(primarys[s2]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3 == 0);
                AZ_TEST_ASSERT(primarys[s3]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1 == 0);
                AZ_TEST_ASSERT(primarys[s3]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2 == 0);

                // Calls should have successfully propagated to the other 2 proxies
                AZ_TEST_ASSERT(sessions[s1].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyProxyRpcCallsFromS2 == 1);
                AZ_TEST_ASSERT(sessions[s1].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyProxyRpcCallsFromS3 == 1);
                AZ_TEST_ASSERT(sessions[s2].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyProxyRpcCallsFromS1 == 1);
                AZ_TEST_ASSERT(sessions[s2].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyProxyRpcCallsFromS3 == 1);
                AZ_TEST_ASSERT(sessions[s3].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyProxyRpcCallsFromS1 == 1);
                AZ_TEST_ASSERT(sessions[s3].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyProxyRpcCallsFromS2 == 1);

                // all datasets should have propagated properly
                for (int i = 0; i < nSessions; ++i)
                {
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1.Get() == primarys[s1]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2.Get() == primarys[s1]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s1]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3.Get() == primarys[s1]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3.Get());

                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1.Get() == primarys[s2]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2.Get() == primarys[s2]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s2]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3.Get() == primarys[s2]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3.Get());

                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1.Get() == primarys[s3]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS1.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2.Get() == primarys[s3]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS2.Get());
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().FindReplica(primarys[s3]->GetRepId())->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3.Get() == primarys[s3]->FindReplicaChunk<TestChunk>()->m_nAuthoritativeOnlyRpcCallsFromS3.Get());
                }
            }

            // tick everything
            for (int i = 0; i < nSessions; ++i)
            {
                sessions[i].Update();
                sessions[i].GetReplicaMgr().Unmarshal();
                sessions[i].GetReplicaMgr().UpdateReplicas();
                sessions[i].GetReplicaMgr().UpdateFromReplicas();
                sessions[i].GetReplicaMgr().Marshal();
                sessions[i].GetTransport()->Update();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        }


        for (int i = 0; i < nSessions; ++i)
        {
            sessions[i].GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(sessions[i].GetTransport());
        }
    }


/*
    *03.25.2015* Typical test results on Core i7 3.4 GHz desktop (with polling):
    Release:
        Replica update time (msec): avg=2.02, min=2, max=3 (peers=40, replicas=16000, freq=0%, samples=4000)
        Replica update time (msec): avg=4.83, min=2, max=12 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=4.73, min=3, max=12 (peers=40, replicas=16000, freq=100%, samples=4000)
    DebugOpt:
        Replica update time (msec): avg=2.53, min=2, max=5 (peers=40, replicas=16000, freq=0%, samples=4000)
        Replica update time (msec): avg=4.21, min=2, max=8 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=5.59, min=3, max=14 (peers=40, replicas=16000, freq=100%, samples=4000)


    Test results (task based marshaling):
    Release:
        Replica update time (msec): avg=0.03, min=0, max=1 (peers=40, replicas=16000, freq=0%, samples=4000)
        Replica update time (msec): avg=3.94, min=1, max=11 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=5.21, min=4, max=15 (peers=40, replicas=16000, freq=100%, samples=4000)
    DebugOpt:
        Replica update time (msec): avg=1.00, min=1, max=2 (peers=40, replicas=16000, freq=0%, samples=4000)
        Replica update time (msec): avg=4.94, min=1, max=9 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=8.05, min=6, max=15 (peers=40, replicas=16000, freq=100%, samples=4000)
*/
class DISABLED_ReplicaStressTest
    : public UnitTest::GridMateMPTestFixture
{
public:
    class StressTestReplica
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(StressTestReplica);
        typedef AZStd::intrusive_ptr<StressTestReplica> Ptr;
        static const char* GetChunkName() { return "StressTestReplica"; }

        StressTestReplica()
            : m_data("Data")
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        bool m_changing;
        DataSet<int> m_data;
    };

    static const size_t NUM_PEERS = 40;
    static const size_t NUM_REPLICAS_PER_PEER = 400;
    static const int FRAME_TIME = 5;
    static const int BASE_PORT = 44270;

    // TODO: Reduce the size or disable the test for platforms which can't allocate 2 GiB
    DISABLED_ReplicaStressTest()
        : UnitTest::GridMateMPTestFixture(2000u * 1024u * 1024u)
    {}

    void UpdateReplica(MPSession& session)
    {
        session.GetReplicaMgr().Unmarshal();
        session.GetReplicaMgr().UpdateReplicas();

        session.GetReplicaMgr().UpdateFromReplicas();
        session.GetReplicaMgr().Marshal();
    }

    void Wait(MPSession* sessions, vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> >& replicas, int numFrames, int frameTime)
    {
        (void)replicas;
        while (numFrames--)
        {
            for (size_t i = 0; i < NUM_PEERS; ++i)
            {
                sessions[i].Update();
                UpdateReplica(sessions[i]);
            }

            for (size_t i = 0; i < NUM_PEERS; ++i)
            {
                sessions[i].GetTransport()->Update();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(frameTime));
        }
    }

    // freq is 0..1, 0.0 - no dirty replicas per tick, 1.0 - all replicas are dirty every tick, 0.5 - 50% of replicas per tick
    void TestReplicas(MPSession* sessions, vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> >& replicas, int numFrames, int frameTime, double freq)
    {
        AZStd::chrono::system_clock::duration minUpdateTime(AZStd::chrono::system_clock::duration::max());
        AZStd::chrono::system_clock::duration maxUpdateTime(AZStd::chrono::system_clock::duration::min());
        AZStd::chrono::system_clock::duration sumSamples(AZStd::chrono::system_clock::duration::zero());
        unsigned long long numSamples = 0;

        int count = 0;
        while (numFrames--)
        {
            MarkChanging(replicas, freq);

            for (auto& r : replicas)
            {
                if (r.second->m_changing)
                {
                    r.second->m_data.Set(count++);
                }
            }

            for (size_t i = 0; i < NUM_PEERS; ++i)
            {
                sessions[i].Update();
                AZStd::chrono::system_clock::time_point beforeUpdateTime = AZStd::chrono::system_clock::now();
                UpdateReplica(sessions[i]);
                auto updateTime = AZStd::chrono::system_clock::now() - beforeUpdateTime;
                minUpdateTime = AZStd::min(updateTime, minUpdateTime);
                maxUpdateTime = AZStd::max(updateTime, maxUpdateTime);
                sumSamples += updateTime;
                ++numSamples;
            }

            for (size_t i = 0; i < NUM_PEERS; ++i)
            {
                sessions[i].GetTransport()->Update();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(frameTime));
        }

        AZ_TEST_ASSERT(numSamples > 0);

        AZ_Printf("GridMate", "\n\n----------------\nReplica update time (msec): avg=%.2f, min=%.2f, max=%.2f (peers=%d, replicas=%d, freq=%d%%, samples=%llu)\n",
            static_cast<double>(AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(sumSamples).count()) / static_cast<double>(numSamples),
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(minUpdateTime).count() / 1000.0,
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(maxUpdateTime).count() / 1000.0,
            NUM_PEERS,
            replicas.size(),
            static_cast<int>(freq * 100.0),
            numSamples);
    }

    bool ConnectPeers(MPSession* sessions, int frameTime)
    {
        size_t frameCount = 0;
        bool allReady = false;
        size_t maxFramesToReady = NUM_PEERS + 100; // this is to avoid infinite loop waiting for all peers to become ready in case some peer cannot connect
        vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> > replicas;

        while (!allReady && frameCount < maxFramesToReady)
        {
            // establish all connections
            if (frameCount < NUM_PEERS)
            {
                for (size_t i = 0; i < frameCount; ++i)
                {
                    sessions[frameCount].GetTransport()->Connect("127.0.0.1", BASE_PORT + static_cast<unsigned int>(i));
                }
            }

            allReady = true;
            for (size_t i = 0; i < NUM_PEERS; ++i)
            {
                if (!sessions[i].GetReplicaMgr().IsReady())
                {
                    allReady = false;
                    break;
                }
            }
            if (allReady)
            {
                AZ_Printf("GridMate", "All peers ready at frame %d\n", frameCount);
            }

            Wait(sessions, replicas, 1, frameTime);
            ++frameCount;
        }

        return allReady;
    }

    virtual void RunStressTests(MPSession* sessions, vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> >& replicas)
    {
        // testing 3 cases & waiting for system to settle in between
        //TestProfiler::StartProfiling();
        Wait(sessions, replicas, 50, FRAME_TIME);
        //TestProfiler::PrintProfilingTotal("GridMate");

        Wait(sessions, replicas, 20, FRAME_TIME);
        //TestProfiler::StartProfiling();
        TestReplicas(sessions, replicas, 100, FRAME_TIME, 0.0); // no replicas are dirty
        //TestProfiler::PrintProfilingTotal("GridMate");

        Wait(sessions, replicas, 20, FRAME_TIME);
        //TestProfiler::StartProfiling();
        TestReplicas(sessions, replicas, 1, FRAME_TIME, 1.0); // single burst dirty replicas
        Wait(sessions, replicas, 2, FRAME_TIME);
        //TestProfiler::PrintProfilingTotal("GridMate");

        Wait(sessions, replicas, 20, FRAME_TIME);
        //TestProfiler::StartProfiling();
        TestReplicas(sessions, replicas, 100, FRAME_TIME, 0.1); // 10% of replicas are marked dirty every frame
        //TestProfiler::PrintProfilingTotal("GridMate");

        Wait(sessions, replicas, 20, FRAME_TIME);
        //TestProfiler::StartProfiling();
        TestReplicas(sessions, replicas, 100, FRAME_TIME, 1.0); // every replica is marked dirty every frame
        //TestProfiler::PrintProfilingTotal("GridMate");
        //TestProfiler::PrintProfilingSelf("GridMate");

        //TestProfiler::StopProfiling();
    }

    virtual void MarkChanging(vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> >& replicas, double freq)
    {
        AZ::Sfmt& sfmt = AZ::Sfmt::GetInstance();
        for (auto& r : replicas)
        {
            r.second->m_changing = sfmt.RandR32_2() <= freq;
        }
    }

    void run()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<StressTestReplica>();

        MPSession sessions[NUM_PEERS];
        vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> > replicas;

        replicas.reserve(NUM_PEERS * NUM_REPLICAS_PER_PEER);

        for (unsigned int i = 0; i < NUM_PEERS; ++i)
        {
            TestCarrierDesc desc;
            desc.m_port = BASE_PORT + i;
            desc.m_enableDisconnectDetection = false;

            // initialize replica managers
            sessions[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            sessions[i].AcceptConn(true);
            sessions[i].SetClient(false);
            sessions[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1
                    , sessions[i].GetTransport()
                    , 0
                    , i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0));
        }

        bool allReady = ConnectPeers(sessions, FRAME_TIME);

        AZ_TEST_ASSERT(allReady);

        for (auto& session : sessions)
        {
            for (size_t j = 0; j < NUM_REPLICAS_PER_PEER; ++j)
            {
                auto rep = Replica::CreateReplica(nullptr);
                auto chunk = CreateAndAttachReplicaChunk<StressTestReplica>(rep);
                replicas.push_back(AZStd::make_pair(rep, chunk));
                session.GetReplicaMgr().AddPrimary(rep);
            }
        }

        RunStressTests(sessions, replicas);

        // clean up
        for (auto& s : sessions)
        {
            s.GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(s.GetTransport());
        }
    }
};

/*
    This test performs updates to the same replicas every frame unlike stress test that picks random replicas every frame
    *03.25.2015* Typical test results on Core i7 3.4 GHz desktop (with polling):
    Release:
        Replica update time (msec): avg=2.54, min=2, max=9 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=3.15, min=2, max=7 (peers=40, replicas=16000, freq=50%, samples=4000)
    DebugOpt:
        Replica update time (msec): avg=3.35, min=3, max=8 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=4.45, min=3, max=10 (peers=40, replicas=16000, freq=50%, samples=4000)

    Test results (task based marshaling):
    Release:
        Replica update time (msec): avg=1.62, min=1, max=10 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=4.38, min=2, max=15 (peers=40, replicas=16000, freq=50%, samples=4000)
    DebugOpt:
        Replica update time (msec): avg=2.01, min=1, max=5 (peers=40, replicas=16000, freq=10%, samples=4000)
        Replica update time (msec): avg=4.61, min=3, max=10 (peers=40, replicas=16000, freq=50%, samples=4000)
*/
class DISABLED_ReplicaStableStressTest
    : public DISABLED_ReplicaStressTest
{
public:

    void MarkChanging(vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> >& replicas, double freq) override
    {
        (void)replicas;
        (void)freq;
    }

    void RunStressTests(MPSession* sessions, vector<AZStd::pair<ReplicaPtr, StressTestReplica::Ptr> >& replicas) override
    {
        DISABLED_ReplicaStressTest::MarkChanging(replicas, 0.1); // picks 10% of replicas
        Wait(sessions, replicas, 20, FRAME_TIME);
        //TestProfiler::StartProfiling();
        TestReplicas(sessions, replicas, 100, FRAME_TIME, 0.1);
        /*TestProfiler::PrintProfilingTotal("GridMate");
        TestProfiler::PrintProfilingSelf("GridMate");*/

        DISABLED_ReplicaStressTest::MarkChanging(replicas, 0.5); // picks 50% of replicas
        Wait(sessions, replicas, 20, FRAME_TIME);
        //TestProfiler::StartProfiling();
        TestReplicas(sessions, replicas, 100, FRAME_TIME, 0.5);
        /*TestProfiler::PrintProfilingTotal("GridMate");
        TestProfiler::PrintProfilingSelf("GridMate");

        TestProfiler::StopProfiling();*/
    }
};


/*
* This test verifies bandwidth limiter. The test takes ~2 minutes. It sends ~7k/s of data with a limit of 4k with the following pattern:
*
*
*       time | 10s | 10s |   20s    |    20s   | 10s |   20s    |
*            +-----+-----+----------+----------+-----+----------+
*   sendrate | 0k  | 7k  |    7k    |   1.5k   | 7k  |    7k    |
*            |     |     |          |          |     |          |
*   expected |none |brst |  capped  |under cap |brst |  capped  |
*
*/
class DISABLED_ReplicaBandiwdthTest
    : public UnitTest::GridMateMPTestFixture
{
public:

    class BandwidthTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(BandwidthTestChunk);
        typedef AZStd::intrusive_ptr<BandwidthTestChunk> Ptr;
        static const char* GetChunkName() { return "BandwidthTestChunk"; }

        BandwidthTestChunk()
            : m_value("Value")
        {
            Touch();
        }

        void Touch()
        {
            AZStd::string randomStr;
            for (unsigned i = 0; i < k_strSize; ++i)
            {
                randomStr += 'a' + (rand() % 26);
            }
            m_value.Set(randomStr);
        }

        bool IsReplicaMigratable() override { return false; }

        static const unsigned k_strSize = 64;
        DataSet<AZStd::string> m_value;
    };

    void run()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<BandwidthTestChunk>();

        MPSession sessions[nSessions];
        // initialize transport
        int basePort = 4427;
        for (int i = 0; i < nSessions; ++i)
        {
            TestCarrierDesc desc;
            desc.m_port = basePort + i;
            desc.m_enableDisconnectDetection = false;
            sessions[i].SetClient(i != sHost);
            sessions[i].SetTransport(DefaultCarrier::Create(desc, m_gridMate));
            sessions[i].AcceptConn(true);
            sessions[i].GetReplicaMgr().Init(ReplicaMgrDesc(i + 1, sessions[i].GetTransport(), 0, i == 0 ? ReplicaMgrDesc::Role_SyncHost : 0));
        }

        // adding replicas for the host
        static const size_t k_numReplicas = 10;
        BandwidthTestChunk::Ptr chunks[k_numReplicas];
        for (size_t i = 0; i < k_numReplicas; ++i)
        {
            auto rep = Replica::CreateReplica(nullptr);
            chunks[i] = CreateAndAttachReplicaChunk<BandwidthTestChunk>(rep);
            sessions[sHost].GetReplicaMgr().AddPrimary(rep);
        }

        // connect to host
        for (size_t i = 0; i < nSessions; ++i)
        {
            if (i == sHost)
            {
                continue;
            }

            sessions[i].GetTransport()->Connect("127.0.0.1", basePort);
        }

        static const int k_delayMS = 100; // tick time

        bool isDone = false;
        size_t numReplicasToChange = 0;

        /*
           10 replicas x ~70 bytes per replica x 10 frames per second =~ 7000 bytes per second
           Will try to cutoff it at 4k per sec
        */
        static const unsigned k_sendRateLimit = 4000;

        unsigned tickNo = 0;
        AZ_Printf("GridMate", "Created %d sessions.\n", nSessions);
        while (!isDone)
        {
            for (size_t i = 0; i < numReplicasToChange; ++i)
            {
                chunks[i]->Touch();
            }

            for (auto connId : sessions[sHost].m_connections)
            {
                if (connId == InvalidConnectionID)
                {
                    continue;
                }

                TrafficControl::Statistics lastSecEffective;
                sessions[sHost].GetTransport()->QueryStatistics(connId, nullptr, nullptr, &lastSecEffective);
                m_measurements[connId].m_sendData.push_back(lastSecEffective.m_dataSend);

                if (!(tickNo % 30)) // printout every 3 seconds
                {
                    AZ_Printf("GridMate", "  - effective sendRate=%.2f KB\n", lastSecEffective.m_dataSend / 1000.f);
                }
                //AZ_TracePrintf("GridMate", "%d\n", lastSecEffective.m_dataSend);
            }


            // =======================================================
            // Actual tests
            // =======================================================
            if (tickNo == 100) // things should've settle at this point, session is established, initial replicas are sent
            {
                for (size_t i = 0; i < AZ_ARRAY_SIZE(sessions); ++i)
                {
                    AZ_TEST_ASSERT(sessions[i].GetReplicaMgr().IsReady());
                }

                numReplicasToChange = k_numReplicas;
                ResetMeasurements();
                AZ_Printf("GridMate", "Starting replica data send. Unlimited.\n");
            }
            else if (tickNo == 200)
            {
                // test if initial unlimited burst was allowed
                for (const auto& m : m_measurements)
                {
                    AZ_TEST_ASSERT(m.second.Max() > k_sendRateLimit * 1.5f);
                }
                ResetMeasurements();
                sessions[sHost].GetReplicaMgr().SetSendLimit(k_sendRateLimit);
                AZ_Printf("GridMate", "Limited by SendLimit=%d Bps.\n", sessions[sHost].GetReplicaMgr().GetSendLimit());
            }
            else if (tickNo == 400)
            {
                // checking if bandwidth was rate limited
                for (const auto& m : m_measurements)
                {
                    if (!(m.second.Mean() < k_sendRateLimit * 1.1f))
                    {
                        AZ_Printf("GridMate", "rate mean: %f limit %d\n", m.second.Mean(), sessions[sHost].GetReplicaMgr().GetSendLimit())
                    }
                    AZ_TEST_ASSERT(m.second.Mean() < k_sendRateLimit * 1.1f); // allowing 10% margin of error
                }
                ResetMeasurements();
                numReplicasToChange = k_numReplicas / 4;                    // reducing outgoing traffic 1/4
                AZ_Printf("GridMate", "Reduced send rate...\n");
            }
            else if (tickNo == 600)
            {
                // checking if traffic below the limit
                for (const auto& m : m_measurements)
                {
                    AZ_TEST_ASSERT(m.second.Mean() < k_sendRateLimit * 0.7f);
                }
                ResetMeasurements();
                sessions[sHost].GetReplicaMgr().SetSendLimit(0);            // returning to unlimited send rate
                numReplicasToChange = k_numReplicas;
                AZ_Printf("GridMate", "Full send rate...\n");
            }
            else if (tickNo == 700)
            {
                // test if burst allowed
                for (const auto& m : m_measurements)
                {
                    AZ_TEST_ASSERT(m.second.Max() > k_sendRateLimit * 1.5f);
                }
                ResetMeasurements();
                sessions[sHost].GetReplicaMgr().SetSendLimit(k_sendRateLimit);
                AZ_Printf("GridMate", "Limited by SendLimit=%d Bps.\n", sessions[sHost].GetReplicaMgr().GetSendLimit());
            }
            else if (tickNo == 900)
            {
                // checking if bandwidth was limited
                for (const auto& m : m_measurements)
                {
                    AZ_TEST_ASSERT(m.second.Mean() < k_sendRateLimit * 1.1f); // allowing 10% jerking around limit
                }

                isDone = true;
            }

            // =======================================================
            // Tick everything
            // =======================================================
            for (size_t i = 0; i < nSessions; ++i)
            {
                sessions[i].Update();
                sessions[i].GetReplicaMgr().Unmarshal();
            }

            for (size_t i = 0; i < nSessions; ++i)
            {
                sessions[i].GetReplicaMgr().UpdateReplicas();
            }

            for (size_t i = 0; i < nSessions; ++i)
            {
                sessions[i].GetReplicaMgr().UpdateFromReplicas();
                sessions[i].GetReplicaMgr().Marshal();
            }

            for (size_t i = 0; i < nSessions; ++i)
            {
                sessions[i].GetTransport()->Update();
            }

            ++tickNo;
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(k_delayMS));
        }


        for (int i = 0; i < nSessions; ++i)
        {
            sessions[i].GetReplicaMgr().Shutdown();
            DefaultCarrier::Destroy(sessions[i].GetTransport());
        }
    }

    void ResetMeasurements()
    {
        for (auto& m : m_measurements)
        {
            m.second.Reset();
        }
    }

    struct Measurement
    {
        unsigned int Mean() const
        {
            AZ_Assert(!m_sendData.empty(), "No data!");
            unsigned int sum = 0;
            for (auto val : m_sendData)
            {
                sum += val;
            }

            return sum / static_cast<unsigned int>(m_sendData.size());
        }

        unsigned int Max() const
        {
            unsigned int curMax = 0;
            for (auto val : m_sendData)
            {
                curMax = AZStd::GetMax(curMax, val);
            }

            return curMax;
        }

        void Reset()
        {
            m_sendData.clear();
        }

        vector<unsigned int> m_sendData; // data sent per second
    };

    enum
    {
        sHost, sClient1, nSessions
    };
    unordered_map<ConnectionID, Measurement> m_measurements;
};

} // namespace UnitTest

GM_TEST_SUITE(ReplicaSuite)
GM_TEST(InterpolatorTest)

#if !defined(AZ_DEBUG_BUILD) // these tests are a little slow for debug
GM_TEST(DISABLED_ReplicaBandiwdthTest)
GM_TEST(DISABLED_ReplicaStressTest)
GM_TEST(DISABLED_ReplicaStableStressTest)
#endif

GM_TEST_SUITE_END()
