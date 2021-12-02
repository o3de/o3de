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

} // namespace UnitTest

GM_TEST_SUITE(ReplicaSuite)
GM_TEST(InterpolatorTest)

#if !defined(AZ_DEBUG_BUILD) // these tests are a little slow for debug
GM_TEST(DISABLED_ReplicaBandiwdthTest)
GM_TEST(DISABLED_ReplicaStressTest)
GM_TEST(DISABLED_ReplicaStableStressTest)
#endif

GM_TEST_SUITE_END()
