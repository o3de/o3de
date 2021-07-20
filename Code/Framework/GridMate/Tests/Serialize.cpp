/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <GridMate/Serialize/CompressionMarshal.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/ContainerMarshal.h>
#include <GridMate/Serialize/UtilityMarshal.h>
#include <GridMate/Serialize/UuidMarshal.h>

#include <GridMate/Containers/vector.h>
#include <GridMate/Containers/unordered_map.h>
#include <GridMate/Containers/unordered_set.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/bitset.h>

using namespace GridMate;

namespace UnitTest
{
    class WriteBufferTest
        : public GridMateMPTestFixture
    {
    public:
        template<typename T>
        T EndianInBuffer(const T& data)
        {
            T res = data;
            AZStd::endian_swap(res);
            return res;
        }


        void run()
        {
            char ch = 127;
            unsigned char uch = 201;
            short sshort = 32002;
            unsigned short ushort = 32001;
            int i = 123456;
            unsigned int ui = 0x7000ffff;
            float f = -5.0f;
            double d = 10.0;

            WriteBufferDynamic wb(EndianType::BigEndian);
            AZ_TEST_ASSERT(wb.Size() == 0);

            wb.Write(ch);
            AZ_TEST_ASSERT(wb.Size() == 1);
            AZ_TEST_ASSERT((*wb.Get()) == ch);

            wb.Clear();
            AZ_TEST_ASSERT(wb.Size() == 0);

            wb.Write(uch);
            AZ_TEST_ASSERT(wb.Size() == 1);
            AZ_TEST_ASSERT((*(unsigned char*)(wb.Get())) == EndianInBuffer(uch));

            wb.Write(ch);
            AZ_TEST_ASSERT(wb.Size() == 2);
            AZ_TEST_ASSERT((*(unsigned char*)(wb.Get())) == EndianInBuffer(uch));

            wb.Clear();

            wb.Write(sshort);
            AZ_TEST_ASSERT(wb.Size() == 2);
            AZ_TEST_ASSERT((*(short*)(wb.Get())) == EndianInBuffer(sshort));

            wb.Write(ushort);
            AZ_TEST_ASSERT(wb.Size() == 4);
            AZ_TEST_ASSERT((*((unsigned short*)(wb.Get()) + 1)) == EndianInBuffer(ushort));
            wb.Clear();

            wb.Write(i);
            AZ_TEST_ASSERT(wb.Size() == 4);
            AZ_TEST_ASSERT((*(int*)(wb.Get())) == EndianInBuffer(i));

            wb.Write(ui);
            AZ_TEST_ASSERT(wb.Size() == 8);
            AZ_TEST_ASSERT(*((unsigned int*)(wb.Get()) + 1) == EndianInBuffer(ui));

            wb.Clear();

            wb.Write(f);
            AZ_TEST_ASSERT(wb.Size() == 4);
            AZ_TEST_ASSERT((*(float*)(wb.Get())) == EndianInBuffer(f));

            wb.Write(d);
            AZ_TEST_ASSERT(wb.Size() == 12);
            AZ_TEST_ASSERT(*(double*)(wb.Get() + 4) == EndianInBuffer(d));

            WriteBufferDynamic wb2(EndianType::BigEndian);

            wb2.Write(ch);

            wb += wb2;

            AZ_TEST_ASSERT(wb.Size() == 13);
            AZ_TEST_ASSERT((*(float*)(wb.Get())) == EndianInBuffer(f));
            AZ_TEST_ASSERT(*(double*)(wb.Get() + 4) == EndianInBuffer(d));
            AZ_TEST_ASSERT(*(char*)(wb.Get() + 12) == EndianInBuffer(ch));

            WriteBufferDynamic wb3 = wb + wb2;
            AZ_TEST_ASSERT(wb3.Size() == 14);
            AZ_TEST_ASSERT((*(float*)(wb3.Get())) == EndianInBuffer(f));
            AZ_TEST_ASSERT(*(double*)(wb3.Get() + 4) == EndianInBuffer(d));
            AZ_TEST_ASSERT(*(char*)(wb3.Get() + 12) == EndianInBuffer(ch));
            AZ_TEST_ASSERT(*(char*)(wb3.Get() + 13) == EndianInBuffer(ch));
        }
    };

    class ReadBufferTest
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            char ch = 127;
            unsigned char uch = 201;
            short sshort = 32002;
            unsigned short ushort = 32001;
            int i = 123456;
            unsigned int ui = 0x7000ffff;
            float f = -5.0f;
            double d = 10.0;

            WriteBufferStatic<> wb(EndianType::BigEndian);
            wb.Write(ch);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());
                char rch;
                rb.Read(rch);
                AZ_TEST_ASSERT(rch == ch);
            }

            wb.Write(uch);
            wb.Write(sshort);
            wb.Write(ushort);
            wb.Write(i);
            wb.Write(ui);
            wb.Write(f);
            wb.Write(d);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());
                char rch;
                unsigned char ruch = 0;
                short rsshort = 0;
                unsigned short rushort = 0;
                int ri = 0;
                unsigned int rui = 0;
                float rf = 0;
                double rd = 0;
                rb.Read(rch);
                rb.Read(ruch);
                AZ_TEST_ASSERT(ruch == uch);
                rb.Read(rsshort);
                AZ_TEST_ASSERT(rsshort == sshort);
                rb.Read(rushort);
                AZ_TEST_ASSERT(rushort == ushort);
                rb.Read(ri);
                AZ_TEST_ASSERT(ri == i);
                rb.Read(rui);
                AZ_TEST_ASSERT(rui == ui);
                rb.Read(rf);
                AZ_TEST_ASSERT(rf == f);
                rb.Read(rd);
                AZ_TEST_ASSERT(rd == d);
            }
        }
    };

    class DataMarshalTest
        : public GridMateMPTestFixture
    {
        typedef vector<float> FloatVectorType;
        typedef unordered_map<int, float> IntFloatMapType;
        typedef unordered_set<int> IntSetType;

    public:
        void run()
        {
            char ch = 127;
            WriteBufferStatic<> wb(EndianType::BigEndian);
            ReadBuffer rb(wb.GetEndianType());

            // ------------------------------------
            // Marshaler
            {
                Marshaler<char>().Marshal(wb, ch);
                AZ_TEST_ASSERT(wb.Size() == 1);
                AZ_TEST_ASSERT((*wb.Get()) == ch);

                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());

                char sch;
                Marshaler<char>().Unmarshal(sch, rb);
                AZ_TEST_ASSERT(sch == ch);

                wb.Clear();
            }

            // ------------------------------------
            // Test the other syntax
            {
                wb.Write(ch, Marshaler<char>());

                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                char rch;
                rb.Read(rch, Marshaler<char>());
                AZ_TEST_ASSERT(rch == ch);

                wb.Clear();
            }

            // ------------------------------------
            // Implicit syntax
            {
                wb.Write(ch);

                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                char rch;
                rb.Read(rch);
                AZ_TEST_ASSERT(rch == ch);

                wb.Clear();
            }

            // ------------------------------------
            // Markers
            {
                wb.Write<AZ::u32>(0xDEADDEAD);

                auto marker = wb.InsertMarker<AZ::u32>(10);
                marker.SetData(1111);

                wb.Write<AZ::u32>(0xF00DF00D);

                {
                    rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                    AZ::u32 v1, v2, v3;
                    rb.Read(v1);
                    rb.Read(v2);
                    rb.Read(v3);

                    AZ_TEST_ASSERT(v1 == 0xDEADDEAD);
                    AZ_TEST_ASSERT(v2 == 1111);
                    AZ_TEST_ASSERT(v3 == 0xF00DF00D);
                }

                marker.SetData(2222);

                {
                    rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                    AZ::u32 v1, v2, v3;
                    rb.Read(v1);
                    rb.Read(v2);
                    rb.Read(v3);

                    AZ_TEST_ASSERT(v1 == 0xDEADDEAD);
                    AZ_TEST_ASSERT(v2 == 2222);
                    AZ_TEST_ASSERT(v3 == 0xF00DF00D);
                }

                wb.Clear();
            }

            // ------------------------------------
            // Explicit Marker
            {
                wb.Write<AZ::u32>(0xDEADDEAD);

                auto marker = wb.InsertMarker<AZ::u32, ConversionMarshaler<AZ::u8, AZ::u32> >(10);
                marker.SetData(0x50505050);

                wb.Write<AZ::u32>(0xF00DF00D);

                {
                    rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                    AZ::u32 v1, v3;
                    AZ::u8 v2;
                    rb.Read(v1);
                    rb.Read(v2);
                    rb.Read(v3);

                    AZ_TEST_ASSERT(v1 == 0xDEADDEAD);
                    AZ_TEST_ASSERT(v2 == 0x50);
                    AZ_TEST_ASSERT(v3 == 0xF00DF00D);
                }

                marker.SetData(0x0A0A0A0A);

                {
                    rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                    AZ::u32 v1, v3;
                    AZ::u8 v2;
                    rb.Read(v1);
                    rb.Read(v2);
                    rb.Read(v3);

                    AZ_TEST_ASSERT(v1 == 0xDEADDEAD);
                    AZ_TEST_ASSERT(v2 == 0x0A);
                    AZ_TEST_ASSERT(v3 == 0xF00DF00D);
                }

                wb.Clear();
            }

            // ------------------------------------
            // Compound Markers
            {
                wb.Write<AZ::u32>(0xDEADDEAD);

                AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(
                    AZ::Vector3(-11.0f, -22.0f, -33.0f),
                    AZ::Vector3(11.0f, 22.0f, 33.0f));

                auto marker = wb.InsertMarker<AZ::Aabb>();
                marker.SetData(aabb);

                wb.Write<AZ::u32>(0xF00DF00D);

                {
                    rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                    AZ::u32 v1, v3;
                    AZ::Aabb readAabb;
                    rb.Read(v1);
                    rb.Read(readAabb);
                    rb.Read(v3);

                    AZ_TEST_ASSERT(v1 == 0xDEADDEAD);
                    AZ_TEST_ASSERT(readAabb.GetMin().GetX() == -11.0f);
                    AZ_TEST_ASSERT(readAabb.GetMin().GetY() == -22.0f);
                    AZ_TEST_ASSERT(readAabb.GetMin().GetZ() == -33.0f);
                    AZ_TEST_ASSERT(readAabb.GetMax().GetX() == 11.0f);
                    AZ_TEST_ASSERT(readAabb.GetMax().GetY() == 22.0f);
                    AZ_TEST_ASSERT(readAabb.GetMax().GetZ() == 33.0f);
                    AZ_TEST_ASSERT(v3 == 0xF00DF00D);
                }

                aabb.SetMin(AZ::Vector3(-1111.0f, -2222.0f, -3333.0f));
                aabb.SetMax(AZ::Vector3(1111.0f, 2222.0f, 3333.0f));
                marker.SetData(aabb);

                wb.Write<AZ::u32>(0xFACEFACE);

                {
                    rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                    AZ::u32 v1, v3, v4;
                    AZ::Aabb readAabb;
                    rb.Read(v1);
                    rb.Read(readAabb);
                    rb.Read(v3);
                    rb.Read(v4);

                    AZ_TEST_ASSERT(v1 == 0xDEADDEAD);
                    AZ_TEST_ASSERT(readAabb.GetMin().GetX() == -1111.0f);
                    AZ_TEST_ASSERT(readAabb.GetMin().GetY() == -2222.0f);
                    AZ_TEST_ASSERT(readAabb.GetMin().GetZ() == -3333.0f);
                    AZ_TEST_ASSERT(readAabb.GetMax().GetX() == 1111.0f);
                    AZ_TEST_ASSERT(readAabb.GetMax().GetY() == 2222.0f);
                    AZ_TEST_ASSERT(readAabb.GetMax().GetZ() == 3333.0f);
                    AZ_TEST_ASSERT(v3 == 0xF00DF00D);
                    AZ_TEST_ASSERT(v4 == 0xFACEFACE);
                }

                wb.Clear();
            }

            // ------------------------------------
            // Float16Marshaler
            wb.Write(1.0f, Float16Marshaler(0.0f, 2.0f));
            AZ_TEST_ASSERT(wb.Size() == 2);

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            float rf;
            Float16Marshaler(0.0f, 2.0f).Unmarshal(rf, rb);
            AZ_TEST_ASSERT_FLOAT_CLOSE(rf, 1.0f);

            wb.Clear();

            // ------------------------------------
            // HalfMarshaler
            wb.Write(3.0f, HalfMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 2);

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rf, HalfMarshaler());
            AZ_TEST_ASSERT_FLOAT_CLOSE(rf, 3.0f);

            wb.Clear();

            // ------------------------------------
            // Enum (1 byte)
            {
                enum class TestEnum8 : AZ::u8
                {
                    Value = 254,
                };

                wb.Write(TestEnum8::Value);
                AZ_TEST_ASSERT(wb.Size() == 1);

                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                TestEnum8 ri;
                rb.Read(ri);
                AZ_TEST_ASSERT(ri == TestEnum8::Value);
                wb.Clear();
            }

            // ------------------------------------
            // Enum (2 byte)
            {
                enum class TestEnum16 : AZ::u16
                {
                    Value = 1234,
                };

                wb.Write(TestEnum16::Value);
                AZ_TEST_ASSERT(wb.Size() == 2);

                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                TestEnum16 ri;
                rb.Read(ri);
                AZ_TEST_ASSERT(ri == TestEnum16::Value);
                wb.Clear();
            }

            // ------------------------------------
            // Enum (manual)
            {
                enum class TestEnum
                {
                    Value = 127,
                };

                wb.Write(TestEnum::Value, ConversionMarshaler<AZ::u8, TestEnum>());
                AZ_TEST_ASSERT(wb.Size() == 1);

                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                TestEnum ri;
                rb.Read(ri, ConversionMarshaler<AZ::u8, TestEnum>());
                AZ_TEST_ASSERT(ri == TestEnum::Value);
                wb.Clear();
            }

            // ------------------------------------
            // CRC
            {
                AZ::Crc32 crc(0x12345678);

                wb.Write(crc);
                AZ_TEST_ASSERT(wb.Size() == sizeof(AZ::u32));

                AZ::Crc32 icrc;
                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                rb.Read(icrc);
                AZ_TEST_ASSERT(icrc == crc);
                wb.Clear();
            }

            // ------------------------------------
            // String
            {
                string s = "hello";

                wb.Write(s);
                AZ_TEST_ASSERT(wb.Size() == s.length() + sizeof(AZ::u16));

                string rs;
                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                rb.Read(rs);
                AZ_TEST_ASSERT(rs == s);
                wb.Clear();
            }

            // ------------------------------------
            // Uuid
            {
                AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();

                wb.Write(uuid1);
                AZ_TEST_ASSERT(wb.Size() == sizeof(uuid1));

                AZ::Uuid uuid2 = AZ::Uuid::CreateNull();
                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                rb.Read(uuid2);
                AZ_TEST_ASSERT(uuid1 == uuid2);
                wb.Clear();
            }

            //////////////////////////////////////////////////////////////////////////
            // Time Marshaler
            //////////////////////////////////////////////////////////////////////////
            AZStd::chrono::microseconds timeMicro(10000);
            wb.Write(timeMicro, Marshaler<AZStd::chrono::microseconds>());
            AZ_TEST_ASSERT(wb.Size() == 4);

            AZStd::chrono::microseconds readMicro;
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readMicro, Marshaler<AZStd::chrono::microseconds>());
            AZ_TEST_ASSERT(readMicro.count() == timeMicro.count());

            wb.Clear();

            AZStd::chrono::milliseconds timeMilli(1000);
            wb.Write(timeMilli);
            AZ_TEST_ASSERT(wb.Size() == 4);

            AZStd::chrono::milliseconds readMilli;
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readMilli);
            AZ_TEST_ASSERT(readMilli.count() == timeMilli.count());

            wb.Clear();

            AZStd::chrono::seconds timeSeconds(100);
            wb.Write(timeSeconds, Marshaler<AZStd::chrono::seconds>());
            AZ_TEST_ASSERT(wb.Size() == 4);

            AZStd::chrono::seconds readSeconds;
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readSeconds, Marshaler<AZStd::chrono::seconds>());
            AZ_TEST_ASSERT(readSeconds.count() == timeSeconds.count());

            wb.Clear();

            AZStd::chrono::minutes timeMin(10);
            wb.Write(timeMin);
            AZ_TEST_ASSERT(wb.Size() == 4);

            AZStd::chrono::minutes readMin;
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readMin);
            AZ_TEST_ASSERT(readMin.count() == timeMin.count());

            wb.Clear();

            AZStd::chrono::hours timeHour(10);
            wb.Write(timeHour, Marshaler<AZStd::chrono::hours>());
            AZ_TEST_ASSERT(wb.Size() == 4);

            AZStd::chrono::hours readHour;
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readHour, Marshaler<AZStd::chrono::hours>());
            AZ_TEST_ASSERT(readHour.count() == timeHour.count());

            wb.Clear();

            wb.Write(timeHour);
            AZ_TEST_ASSERT(wb.Size() == 4);

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readMilli);
            AZ_TEST_ASSERT(readMilli.count() == (timeHour.count() * 3600000));

            wb.Clear();

            //////////////////////////////////////////////////////////////////////////
            // ContainerMarshaler (Vector)
            //////////////////////////////////////////////////////////////////////////
            FloatVectorType fArray;
            fArray.push_back(1.0f);
            fArray.push_back(2.0f);
            fArray.push_back(3.0f);
            fArray.push_back(5.0f);

            // ContainerMarshaler - default
            wb.Clear();
            wb.Write(fArray);
            AZ_TEST_ASSERT(wb.Size() == (fArray.size() * sizeof(FloatVectorType::value_type) + sizeof(AZ::u16)));

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            FloatVectorType rfArray;
            rb.Read(rfArray);
            AZ_TEST_ASSERT(rfArray.size() == fArray.size());
            AZ_TEST_ASSERT(rfArray[0] == fArray[0]);
            AZ_TEST_ASSERT(rfArray[1] == fArray[1]);
            AZ_TEST_ASSERT(rfArray[2] == fArray[2]);
            AZ_TEST_ASSERT(rfArray[3] == fArray[3]);

            // ContainerMarshaler with element compression by half
            wb.Clear();
            wb.Write(fArray, ContainerMarshaler<FloatVectorType, HalfMarshaler>());
            AZ_TEST_ASSERT(wb.Size() == (fArray.size() * (sizeof(FloatVectorType::value_type) / 2) + sizeof(AZ::u16)));

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rfArray, ContainerMarshaler<FloatVectorType, HalfMarshaler>());
            AZ_TEST_ASSERT(rfArray.size() == fArray.size());
            AZ_TEST_ASSERT_FLOAT_CLOSE(rfArray[0], fArray[0]);
            AZ_TEST_ASSERT_FLOAT_CLOSE(rfArray[1], fArray[1]);
            AZ_TEST_ASSERT_FLOAT_CLOSE(rfArray[2], fArray[2]);
            AZ_TEST_ASSERT_FLOAT_CLOSE(rfArray[3], fArray[3]);

            //////////////////////////////////////////////////////////////////////////
            // ContainerMarshaler (Set)
            //////////////////////////////////////////////////////////////////////////
            IntSetType iSet;
            iSet.insert(1);
            iSet.insert(2);
            iSet.insert(3);

            // ContainerMarshaler default
            wb.Clear();
            wb.Write(iSet);
            AZ_TEST_ASSERT(wb.Size() == (iSet.size() * (sizeof(IntSetType::key_type)) + sizeof(AZ::u16)));

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            IntSetType riSet;
            rb.Read(riSet);
            AZ_TEST_ASSERT(riSet.size() == iSet.size());
            for (auto it1 = iSet.begin(); it1 != iSet.end(); ++it1)
            {
                auto it2 = riSet.find(*it1);
                AZ_TEST_ASSERT(it2 != riSet.end());
            }

            // ContainerMarshaler with compressed key
            wb.Clear();
            wb.Write(iSet, ContainerMarshaler<IntSetType, ConversionMarshaler<AZ::s8, int> >());
            AZ_TEST_ASSERT(wb.Size() == (iSet.size() * (sizeof(IntSetType::key_type) / 4) + sizeof(AZ::u16)));

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(riSet, ContainerMarshaler<IntSetType, ConversionMarshaler<AZ::s8, int> >());
            AZ_TEST_ASSERT(riSet.size() == iSet.size());
            for (auto it1 = iSet.begin(); it1 != iSet.end(); ++it1)
            {
                auto it2 = riSet.find(*it1);
                AZ_TEST_ASSERT(it2 != riSet.end());
            }

            //////////////////////////////////////////////////////////////////////////
            // MapContainerMarshaler
            //////////////////////////////////////////////////////////////////////////
            IntFloatMapType ifMap;
            ifMap.insert(AZStd::make_pair(1, 5.0f));
            ifMap.insert(AZStd::make_pair(10, 3.0f));
            ifMap.insert(AZStd::make_pair(6, 2.0f));
            ifMap.insert(AZStd::make_pair(3, 0.5f));

            // MapContainerMarshaler default
            wb.Clear();
            wb.Write(ifMap);
            AZ_TEST_ASSERT(wb.Size() == (ifMap.size() * (sizeof(IntFloatMapType::key_type) + sizeof(IntFloatMapType::mapped_type)) + sizeof(AZ::u16)));

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            IntFloatMapType rifMap;
            rb.Read(rifMap);
            AZ_TEST_ASSERT(rifMap.size() == ifMap.size());
            for (IntFloatMapType::iterator it1 = ifMap.begin(); it1 != ifMap.end(); ++it1)
            {
                IntFloatMapType::iterator it2 = rifMap.find(it1->first);
                AZ_TEST_ASSERT(it2 != rifMap.end());
                AZ_TEST_ASSERT(it1->second == it2->second);
            }

            // MapContainerMarshaler with compressed key and mapped value
            wb.Clear();
            wb.Write(ifMap, MapContainerMarshaler<IntFloatMapType, ConversionMarshaler<AZ::s8, int>, HalfMarshaler>());
            AZ_TEST_ASSERT(wb.Size() == (ifMap.size() * (sizeof(IntFloatMapType::key_type) / 4 + sizeof(IntFloatMapType::mapped_type) / 2) + sizeof(AZ::u16)));

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rifMap, MapContainerMarshaler<IntFloatMapType, ConversionMarshaler<AZ::s8, int>, HalfMarshaler>());
            AZ_TEST_ASSERT(rifMap.size() == ifMap.size());
            for (IntFloatMapType::iterator it1 = ifMap.begin(); it1 != ifMap.end(); ++it1)
            {
                IntFloatMapType::iterator it2 = rifMap.find(it1->first);
                AZ_TEST_ASSERT(it2 != rifMap.end());
                AZ_TEST_ASSERT_FLOAT_CLOSE(it1->second, it2->second);
            }
        }
    };

    class MathMarshalTest
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            //////////////////////////////////////////////////////////////////////////
            // Vector3
            //////////////////////////////////////////////////////////////////////////
            AZ::Vector3 v, rv;

            WriteBufferStatic<> wb(EndianType::BigEndian);

            // Vec3Marshaler
            v.Set(100.1f, 0.004f, 2000.45f);
            wb.Write(v);
            AZ_TEST_ASSERT(wb.Size() == 12);

            ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rv);
            AZ_TEST_ASSERT(v.IsClose(rv));

            // Vec3CompMarshaler
            wb.Clear();
            wb.Write(v, Vec3CompMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 6);

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rv, Vec3CompMarshaler());
            AZ_TEST_ASSERT(v.IsClose(rv, 0.5f));

            // Vec3CompNormMarshaler
            v.Set(0.0f, 1.0f, 0.0f);
            wb.Clear();
            wb.Write(v, Vec3CompNormMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 1); // 1 flags

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rv, Vec3CompNormMarshaler());
            AZ_TEST_ASSERT(v.IsClose(rv));

            v.Set(0.0f, 1.0f, 1.0f);
            v.Normalize();
            wb.Clear();
            wb.Write(v, Vec3CompNormMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 5); // 1 flags + 2 for Y + 2 for Z

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rv, Vec3CompNormMarshaler());
            AZ_TEST_ASSERT(v.IsClose(rv, 0.03f));

            //////////////////////////////////////////////////////////////////////////
            // Color
            //////////////////////////////////////////////////////////////////////////
            AZ::Color color, readColor;
            color.Set(1, 0.2f, 0.6f, 0.8f);
            // Start the read color at a totally different value.
            readColor.Set(0, 1, 1, 0);
            wb.Clear();
            wb.Write(color);
            AZ_TEST_ASSERT(wb.Size() == 16);
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readColor);
            AZ_TEST_ASSERT(color.IsClose(readColor));

            //////////////////////////////////////////////////////////////////////////
            // Quaternion
            //////////////////////////////////////////////////////////////////////////
            AZ::Quaternion q, rq;

            // QuatMarshaler
            q = AZ::Quaternion::CreateRotationX(1.0f);
            wb.Clear();
            wb.Write(q);
            AZ_TEST_ASSERT(wb.Size() == 16);

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rq);
            AZ_TEST_ASSERT(q.IsClose(rq));

            // QuatCompMarshaler
            wb.Clear();
            wb.Write(q, QuatCompMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 8);

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rq, QuatCompMarshaler());
            AZ_TEST_ASSERT(q.IsClose(rq, 0.03f));

            // QuatCompNormMarshaler
            q.Set(0.0f, 1.0f, 0.0f, 0.0f);
            wb.Clear();
            wb.Write(q, QuatCompNormMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 1); // 1 flags

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rq, QuatCompNormMarshaler());
            AZ_TEST_ASSERT(q.IsClose(rq));

            q.Set(1.0f, 1.0f, 1.0f, 0.5f);
            q.Normalize();
            wb.Clear();
            wb.Write(q, QuatCompNormMarshaler());
            AZ_TEST_ASSERT(wb.Size() == 7); // 1 flags + 2 for X + 2 for Y + 2 for Z

            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(rq, QuatCompNormMarshaler());
            AZ_TEST_ASSERT(q.IsClose(rq, 0.03f));
        }
    };

    class TransformMarshalTest
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            AZ::Transform originalTransform, readTransform;
            WriteBufferStatic<> wb(EndianType::BigEndian);

            originalTransform.SetFromEulerDegrees(AZ::Vector3(35.0f, 27.0f, 49.0f));
            originalTransform.SetTranslation(AZ::Vector3(134.8f, -2017.3f, 519.2f));

            wb.Write(originalTransform);

            ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readTransform);
            EXPECT_TRUE(readTransform.IsClose(originalTransform));

            wb.Clear();
            wb.Write(originalTransform, TransformCompressor());
            rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
            rb.Read(readTransform, TransformCompressor());
            EXPECT_TRUE(readTransform.IsClose(originalTransform));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Integer Quantization Marshaler
    //////////////////////////////////////////////////////////////////////////
    class CompressionMarshalTest
        : public GridMateMPTestFixture
    {
    public:
        template<class T, int Min, int Max, AZ::u64 Bytes, int Epsilon>
        void PerformTest()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);
            ReadBuffer rb(wb.GetEndianType());

            for (int i = Min; i <= Max; i = i + 15259) // Arbitrary large prime number.
            {
                wb.Write(static_cast<T>(i), IntegerQuantizationMarshaler<Min, Max, Bytes>());
                AZ_TEST_ASSERT(wb.Size() == Bytes);

                T readValue;
                rb = ReadBuffer(wb.GetEndianType(), wb.Get(), wb.Size());
                rb.Read(readValue, IntegerQuantizationMarshaler<Min, Max, Bytes>());
                AZ_TEST_ASSERT_CLOSE(readValue, i, Epsilon);

                wb.Clear();
            }
        }

        void run()
        {
            PerformTest<AZ::u16, 0, USHRT_MAX, 1, 257>();
            PerformTest<AZ::u16, 0, USHRT_MAX, 2, 3>();
            PerformTest<AZ::u16, 0, USHRT_MAX, 4, 2>();
            PerformTest<AZ::s16, SHRT_MIN, SHRT_MAX, 1, 257>();
            PerformTest<AZ::s16, SHRT_MIN, SHRT_MAX, 2, 3>();
            PerformTest<AZ::s16, SHRT_MIN, SHRT_MAX, 4, 2>();
            PerformTest<AZ::u32, 0, USHRT_MAX, 1, 257>();
            PerformTest<AZ::u32, 0, USHRT_MAX, 2, 3>();
            PerformTest<AZ::u32, 0, (INT_MAX / 4), 4, 17>();
            PerformTest<AZ::s32, SHRT_MIN, SHRT_MAX, 1, 257>();
            PerformTest<AZ::s32, SHRT_MIN, SHRT_MAX, 2, 3>();
            PerformTest<AZ::s32, (INT_MIN / 4), (INT_MAX / 4), 4, 33>();

            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x0;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 1);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x1;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 1);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0xf;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 1);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x7f;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 1);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x80;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 2);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0xff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 2);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x3fff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 2);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x4000;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 3);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0xffff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 3);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x1fffff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 3);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x200000;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 4);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0xffffff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 4);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0xfffffff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 4);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0x10000000;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 5);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = 0xffffffff;
                VlqU32Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 5);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Integer Quantization Marshaler
    //////////////////////////////////////////////////////////////////////////
    class Compression64MarshalTest
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            AZStd::bitset<64> test;
            test.set(40);
            AZ::u64 test_value = test.to_ullong();
            AZ_TEST_ASSERT(test_value == (1ull << 40));

            // Testing ranges for each byte count
            RunTest<1, 0, 0x80, 10>();
            RunTest<2, 0x80, 0x4000, 10>();
            RunTest<3, 0x4000, 0x200000, 10>();
            RunTest<4, 0x200000, 0x10000000, 10>();
            RunTest<5, 0x10000000, 0x0800000000, 10>();
            RunTest<6, 0x0800000000, 0x040000000000, 10>();
            RunTest<7, 0x040000000000, 0x02000000000000, 10>();
            RunTest<8, 0x02000000000000, 0x0100000000000000, 10>();
            RunTest<9, 0x0100000000000000, 0xEFFFFFFFFFFFFFFF, 10>();

            // corner case - MAX value
            {
                AZ::u64 src = 0xFFFFFFFFFFFFFFFFull;
                WriteBufferStatic<> wb(EndianType::BigEndian);
                VlqU64Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 9);

                AZ::u64 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU64Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }

            // corner case - Zero value
            {
                AZ::u64 src = 0;
                WriteBufferStatic<> wb(EndianType::BigEndian);
                VlqU64Marshaler().Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == 1);

                AZ::u64 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU64Marshaler().Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
        }

        template <AZ::u8 Bytes, AZ::u64 StartRange, AZ::u64 EndRange, AZ::u64 Steps>
        void RunTest()
        {
            static_assert(EndRange > 0, "Catch overflow when specifying large numbers");
            static_assert(Steps > 0, "Steps should be a sane value");

            VlqU64Marshaler marshaler;

            for (AZ::u64 src = StartRange; src < EndRange; src += (EndRange - StartRange) / Steps)
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                marshaler.Marshal(wb, src);
                AZ_TEST_ASSERT(wb.Size() == Bytes);

                AZ::u64 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                marshaler.Unmarshal(dest, rb);
                AZ_TEST_ASSERT(src == dest);
            }
        }
    };

    class VariableLengthMarshalerPerformanceTest
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            AZ::SimpleLcgRandom random;
            AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
            for (int i = 0; i < 100000; ++i)
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = random.GetRandom();
                VlqU32Marshaler().Marshal(wb, src);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                VlqU32Marshaler().Unmarshal(dest, rb);
            }
            AZStd::chrono::system_clock::time_point end = AZStd::chrono::system_clock::now();
            AZ_Printf("GridMate", "Test with VLQ took %.2f milliseconds.\n", static_cast<float>(AZStd::chrono::microseconds(end - start).count()) / 1000.f);

            start = AZStd::chrono::system_clock::now();
            for (int i = 0; i < 100000; ++i)
            {
                WriteBufferStatic<> wb(EndianType::BigEndian);
                AZ::u32 src = random.GetRandom();
                wb.Write(src);
                AZ::u32 dest = 0;
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                rb.Read(dest);
            }
            end = AZStd::chrono::system_clock::now();
            AZ_Printf("GridMate", "Test reference took %.2f milliseconds.\n", static_cast<float>(AZStd::chrono::microseconds(end - start).count()) / 1000.f);
        }
    };

    /*
     * Testing bit packing feature of the read/write buffers.
     */
    class BitPackingTest
        : public GridMateMPTestFixture
    {
    public:
        template<typename T>
        T EndianInBuffer(const T& data)
        {
            T res = data;
            AZStd::endian_swap(res);
            return res;
        }

        void run()
        {
            test_WriteBitsFromVoid();
            test_Char();
            test_ReadInner();
            test_ReadInnerInner();
            test_VlqU32();
            test_Uint();
            test_JustOneBool();
            test_UShort();
            test_PartialByte();
            test_FullByte();
            test_SpanningBytes();
            test_16AfterBits();
        }

        void test_16AfterBits()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);

            int prefix = 0;
            AZ::u16 value = 6;

            wb.WriteRaw(&prefix, {0, 9});      // 1
            wb.Write(value);     // 2

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.GetExactSize());
                AZ_TEST_ASSERT(rb.Size() == wb.GetExactSize());

                int rprefix = 0;
                rb.ReadRaw(&rprefix, {0, 9}); // 1
                AZ_TEST_ASSERT(rprefix == prefix);

                short rvalue = 0;
                rb.Read(rvalue); // 2
                AZ_TEST_ASSERT(rvalue == value);
            }
        }

        void test_WriteBitsFromVoid()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);

            bool btrue = true;
            bool bfalse = false;

            wb.WriteRaw(&btrue, {0, 1});      // 1
            AZ_TEST_ASSERT(wb.GetExactSize() == PackedSize(0, 1));

            wb.WriteRaw(&bfalse, {0, 1});     // 2
            AZ_TEST_ASSERT(wb.GetExactSize() == PackedSize(0, 2));

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.GetExactSize());
                AZ_TEST_ASSERT(rb.Size() == wb.GetExactSize());

                bool rbool;

                rb.Read(rbool); // 1
                AZ_TEST_ASSERT(rbool == btrue);

                rb.Read(rbool); // 2
                AZ_TEST_ASSERT(rbool == bfalse);
            }
        }

        void test_ReadInnerInner()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);

            unsigned int ui1 = 0x7000ffff;
            unsigned int ui2 = 0x7000aaaa;
            wb.Write(true);     // 1
            wb.Write(ui1);      // 2
            wb.Write(false);    // 3
            wb.Write(true);     // 4
            wb.Write(ui2);      // 5
            wb.Write(true);     // 6
            wb.Write(false);    // 7

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.GetExactSize());
                AZ_TEST_ASSERT(rb.Size() == wb.GetExactSize());

                bool rbool;
                unsigned int rui1, rui2;

                rb.Read(rbool); // 1
                AZ_TEST_ASSERT(rbool);

                {
                    auto innerBuffer = rb.ReadInnerBuffer(PackedSize(4+4, 1+1+1));

                    innerBuffer.Read(rui1); // 2
                    AZ_TEST_ASSERT(rui1 == ui1);

                    innerBuffer.Read(rbool); // 3
                    AZ_TEST_ASSERT(rbool == false);

                    {
                        auto innerInnerBuffer = innerBuffer.ReadInnerBuffer(PackedSize(4, 1));

                        innerInnerBuffer.Read(rbool); // 4
                        AZ_TEST_ASSERT(rbool == true);

                        innerInnerBuffer.Read(rui2); // 5
                        AZ_TEST_ASSERT(rui2 == ui2);
                    }

                    // inner buffer should have skipped to the remaining part
                    innerBuffer.Read(rbool); // 6
                    AZ_TEST_ASSERT(rbool);
                }

                // main buffer should have skipped to the last part
                rb.Read(rbool); // 7
                AZ_TEST_ASSERT(!rbool);
            }
        }

        void test_ReadInner()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);

            unsigned int ui1 = 0x7000ffff;
            unsigned int ui2 = 0x7000aaaa;
            wb.Write(true);
            wb.Write(ui1);
            wb.Write(false);
            wb.Write(ui2);
            wb.Write(true);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;
                unsigned int rui1, rui2;

                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(rui1);
                AZ_TEST_ASSERT(rui1 == ui1);
                {
                    auto innerBuffer = rb.ReadInnerBuffer(PackedSize(4, 1));

                    innerBuffer.Read(test);
                    AZ_TEST_ASSERT(test == false);
                    innerBuffer.Read(rui2);
                    AZ_TEST_ASSERT(rui2 == ui2);
                }

                // main buffer should have skipped to the last part
                rb.Read(test);
                AZ_TEST_ASSERT(test);
            }
        }

        void test_JustOneBool()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);
            wb.Write(false);

            AZ_TEST_ASSERT(wb.Size() == 1);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;

                rb.Read(test);
                AZ_TEST_ASSERT(!test);
            }
        }

        void test_PartialByte()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);
            wb.Write(true);
            wb.Write(false);
            wb.Write(true);
            wb.Write(true);
            wb.Write(false);

            AZ_TEST_ASSERT(wb.Size() == 1);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;

                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
            }
        }

        void test_FullByte()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);
            wb.Write(true);
            wb.Write(true);
            wb.Write(true);
            wb.Write(true);

            wb.Write(false);
            wb.Write(false);
            wb.Write(false);
            wb.Write(true);

            AZ_TEST_ASSERT(wb.Size() == 1);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;

                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
            }
        }

        void test_VlqU32()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);
            VlqU32Marshaler marshaler;

            AZ::u32 v = 0xf;
            wb.Write(v, marshaler);
            wb.Write(false);
            wb.Write(v, marshaler);
            wb.Write(true);

            auto currentSize = wb.GetExactSize();
            AZ_TEST_ASSERT(currentSize.GetBytes() == 2);
            AZ_TEST_ASSERT(currentSize.GetAdditionalBits() == 2);

            auto size = wb.Size();
            AZ_TEST_ASSERT(size == 3);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;
                AZ::u32 rv;

                rb.Read(rv, marshaler);
                AZ_TEST_ASSERT(rv == v);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
                rb.Read(rv, marshaler);
                AZ_TEST_ASSERT(rv == v);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
            }
        }

        void test_Uint()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);

            unsigned int ui = 0xff;
            wb.Write(true);
            wb.Write(ui);
            wb.Write(false);
            wb.Write(ui);
            wb.Write(true);

            auto currentSize = wb.Size();
            AZ_TEST_ASSERT(currentSize == 9);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;
                unsigned int rui;

                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(rui);
                AZ_TEST_ASSERT(rui == ui);
                rb.Read(test);
                AZ_TEST_ASSERT(test == false);
                rb.Read(rui);
                AZ_TEST_ASSERT(rui == ui);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
            }
        }

        void test_Char()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);
            char ch = 127;

            wb.Write(true);
            wb.Write(ch);
            wb.Write(false);
            wb.Write(ch);
            wb.Write(true);

            auto currentSize = wb.Size();
            AZ_TEST_ASSERT(currentSize == 3);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;
                unsigned char rch;

                rb.Read(test);
                AZ_TEST_ASSERT(test);
                rb.Read(rch);
                AZ_TEST_ASSERT(rch == ch);
                rb.Read(test);
                AZ_TEST_ASSERT(!test);
                rb.Read(rch);
                AZ_TEST_ASSERT(rch == ch);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
            }
        }

        void test_UShort()
        {
            WriteBufferStatic<> wb(EndianType::BigEndian);

            unsigned short ushort = 32001;
            wb.Write(ushort);
            wb.Write(false);
            wb.Write(ushort);
            wb.Write(true);

            auto currentSize = wb.Size();
            AZ_TEST_ASSERT(currentSize == 5);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());

                bool test;
                unsigned short rushort;

                rb.Read(rushort);
                AZ_TEST_ASSERT(rushort == ushort);
                rb.Read(test);
                AZ_TEST_ASSERT(!test);
                rb.Read(rushort);
                AZ_TEST_ASSERT(rushort == ushort);
                rb.Read(test);
                AZ_TEST_ASSERT(test);
            }
        }

        void test_SpanningBytes()
        {
            char ch = 127;
            unsigned char uch = 201;
            short sshort = 32002;
            unsigned short ushort = 32001;
            int i = 123456;
            unsigned int ui = 0x7000ffff;
            float f = -5.0f;
            double d = 10.0;

            WriteBufferStatic<> wb(EndianType::BigEndian);
            wb.Write(ch);

            {
                // insert some bools inbetween
                wb.Write(true);
                wb.Write(false);
                wb.Write(true);
            }

            wb.Write(uch);
            wb.Write(sshort);
            wb.Write(ushort);
            wb.Write(i);
            wb.Write(ui);
            wb.Write(f);
            wb.Write(d);

            {
                ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                AZ_TEST_ASSERT(rb.Size() == wb.Size());
                char rch;
                unsigned char ruch = 0;
                short rsshort = 0;
                unsigned short rushort = 0;
                int ri = 0;
                unsigned int rui = 0;
                float rf = 0;
                double rd = 0;
                rb.Read(rch);

                {
                    // read the bools inbetween
                    bool tmp;
                    rb.Read(tmp);
                    AZ_TEST_ASSERT(tmp);
                    rb.Read(tmp);
                    AZ_TEST_ASSERT(!tmp);
                    rb.Read(tmp);
                    AZ_TEST_ASSERT(tmp);
                }

                rb.Read(ruch);
                AZ_TEST_ASSERT(ruch == uch);
                rb.Read(rsshort);
                AZ_TEST_ASSERT(rsshort == sshort);
                rb.Read(rushort);
                AZ_TEST_ASSERT(rushort == ushort);
                rb.Read(ri);
                AZ_TEST_ASSERT(ri == i);
                rb.Read(rui);
                AZ_TEST_ASSERT(rui == ui);
                rb.Read(rf);
                AZ_TEST_ASSERT(rf == f);
                rb.Read(rd);
                AZ_TEST_ASSERT(rd == d);
            }
        }
    };

    /*
    * Testing bit packing feature of the read/write buffers.
    */
    class PackedSizeTest
        : public GridMateMPTestFixture
    {
    public:

        void run()
        {
            AZ_TEST_ASSERT(PackedSize(0, 0).GetSizeInBytesRoundUp() == 0);
            AZ_TEST_ASSERT(PackedSize(0, 1).GetSizeInBytesRoundUp() == 1);
            AZ_TEST_ASSERT(PackedSize(1, 1).GetSizeInBytesRoundUp() == 2);
            AZ_TEST_ASSERT(PackedSize(2, 0).GetSizeInBytesRoundUp() == 2);

            AZ_TEST_ASSERT(PackedSize(0, 1) + PackedSize(0, 7) == PackedSize(1, 0));
            AZ_TEST_ASSERT(PackedSize(10, 4) + PackedSize(10, 7) == PackedSize(21, 3));

            AZ_TEST_ASSERT(PackedSize(10, 7) - PackedSize(10, 4) == PackedSize(0, 3));
            AZ_TEST_ASSERT(PackedSize(10, 4) - PackedSize(0, 7) == PackedSize(9, 5));

            AZ_TEST_ASSERT(PackedSize(10, 4) > PackedSize(10, 3));
            AZ_TEST_ASSERT(PackedSize(10, 4) >= PackedSize(10, 4));

            AZ_TEST_ASSERT(PackedSize(0, 4) <= PackedSize(0, 4));
            AZ_TEST_ASSERT(PackedSize(0, 4) < PackedSize(0, 7));

            {
                PackedSize tmp(10);
                tmp -= PackedSize(1, 4);
                AZ_TEST_ASSERT(tmp == PackedSize(8, 4));
            }
            {
                PackedSize tmp(10);
                tmp += PackedSize(1, 3);
                AZ_TEST_ASSERT(tmp == PackedSize(11, 3));
            }

            AZ_TEST_ASSERT(PackedSize(0, 4) > 0);

            AZ_TEST_ASSERT(PackedSize(0, 0) == 0);

            {
                PackedSize tmp(10);
                tmp.IncrementBit();
                AZ_TEST_ASSERT(tmp == PackedSize(10, 1));
            }
            {
                PackedSize tmp(10);
                tmp.IncrementBits(9);
                AZ_TEST_ASSERT(tmp == PackedSize(11, 1));
            }
            {
                PackedSize tmp(10);
                tmp.IncrementBytes(3);
                AZ_TEST_ASSERT(tmp == PackedSize(13));
            }
            {
                PackedSize tmp(10);
                tmp.DecrementBits(3);
                AZ_TEST_ASSERT(tmp == PackedSize(9, 5));
            }
            {
                PackedSize tmp(10);
                tmp.DecrementBytes(5);
                AZ_TEST_ASSERT(tmp == PackedSize(5));
            }
        }
    };
}

GM_TEST_SUITE(SerializeSuite)
GM_TEST(PackedSizeTest)
GM_TEST(BitPackingTest)
GM_TEST(WriteBufferTest)
GM_TEST(ReadBufferTest)
GM_TEST(DataMarshalTest)
#if !AZ_TRAIT_DISABLE_FAILED_GRIDMATE_TESTS
GM_TEST(MathMarshalTest)
GM_TEST(TransformMarshalTest)
#endif // !AZ_TRAIT_DISABLE_FAILED_GRIDMATE_TESTS
GM_TEST(CompressionMarshalTest)
GM_TEST(Compression64MarshalTest)
GM_TEST(VariableLengthMarshalerPerformanceTest)
GM_TEST_SUITE_END()
