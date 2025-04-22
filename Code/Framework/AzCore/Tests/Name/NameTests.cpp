/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <AzCore/std/utils.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/Internal/NameData.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/time.h>

#include <thread>
#include <stdlib.h>
#include <time.h>
#include <cstring>

namespace UnitTest
{
    static AZ::Name globalName = AZ::Name::FromStringLiteral("global", AZ::Interface<AZ::NameDictionary>::Get());

    class NameDictionaryTester
    {
    public:
        static void Create()
        {
            AZ::NameDictionary::Create();

            ASSERT_EQ(0, GetEntryCount());
        }

        static void Destroy()
        {
            AZ::NameDictionary::Destroy();
        }

        static const auto& GetDictionary()
        {
            return AZ::NameDictionary::Instance().m_dictionary;
        }
        
        static size_t GetEntryCount()
        {
            // Subtract any static scope names hanging around
            AZ::Name* head = &AZ::NameDictionary::Instance().m_deferredHead;
            size_t staticNameCount = 0;
            AZ::Name* current = head;
            AZStd::set<AZ::Name::Hash> recordedNames;
            while (current != nullptr)
            {
                // Add one entry to the count for every unique literal in the dictionary, compare by hash (which the name dictionary has guaranteed deduplicated for us)
                if (current->m_data != nullptr && !recordedNames.contains(current->m_data->GetHash()))
                {
                    recordedNames.insert(current->m_data->GetHash());
                    ++staticNameCount;
                }
                current = current->m_nextName;
                if (current == head)
                {
                    break;
                }
            }
            return GetDictionary().size() - staticNameCount;
        }

        //! Directly calculate the hash value for a string without collision resolution
        static AZ::Name::Hash CalcDirectHashValue(AZStd::string_view name, const uint32_t maxUniqueHashes = std::numeric_limits<uint32_t>::max())
        {
            uint32_t hash = AZ::NameDictionary::Instance().CalcHash(name);

            if (maxUniqueHashes < std::numeric_limits<uint32_t>::max())
            {
                hash %= maxUniqueHashes;
                // Rather than use this modded value as the hash, we run a string hash again to
                // get spread-out hash values that are similar to the real ones, and avoid clustering.
                AZStd::fixed_string<16> buffer;
                AZStd::to_string(buffer, hash);
                hash = AZStd::hash<AZStd::string_view>{}(buffer) & 0xFFFFFFFF;
            }

            return hash;
        }
    };
    
    class ThreadCreatesOneName
    {
    public:
        ThreadCreatesOneName(AZStd::string_view name) : m_originalName(name)
        { }

        void Start()
        {
            m_thread = AZStd::thread([this]() { ThreadJob(); });
        }

        void Join()
        {
            m_thread.join();
        }

        const AZ::Name& GetAzName() const
        {
            return m_azName;
        }

        void ThreadJob()
        {
            m_azName = AZ::Name(m_originalName);
        }
                
    private:
        AZStd::thread m_thread;
        AZ::Name m_azName;
        AZStd::string m_originalName;
    };

    template<size_t N>
    class ThreadRepeatedlyCreatesAndReleasesOneName
    {
    public:
        ThreadRepeatedlyCreatesAndReleasesOneName(AZStd::string_view name) : m_originalName(name)
        { }

        void Start()
        {
            m_thread = AZStd::thread([this]() { ThreadJob(); });
        }

        void Join()
        {
            m_thread.join();
        }

        const AZ::Name& GetAzName() const
        {
            return m_azName;
        }

        void ThreadJob()
        {
            m_azName = AZ::Name(m_originalName);

            for (size_t i = 0; i < N; ++i)
            {
                m_azName = AZ::Name();
                m_azName = AZ::Name(m_originalName);
            }

        }

    private:
        AZStd::thread m_thread;
        AZ::Name m_azName;
        AZStd::string m_originalName;
    };

    class NameTest : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            NameDictionaryTester::Create();
        }

        void TearDown() override
        {
            NameDictionaryTester::Destroy();
            LeakDetectionFixture::TearDown();
        }

        static constexpr int RandomStringBufferSize = 16;

        char* MakeRandomString(char buffer[RandomStringBufferSize])
        {
            azsnprintf(buffer, RandomStringBufferSize, "%d", m_random.GetRandom());
            return buffer;
        }

        AZ::Internal::NameData* GetNameData(AZ::Name& name)
        {
            return name.m_data.get();
        }

        void FreeMemoryFromNameData(AZ::Internal::NameData* nameData)
        {
            delete nameData;
        }

        AZ::SimpleLcgRandom m_random;
    };

    // Implemented in script.cpp
    void AZTestAssert(bool check);

    class NameScriptingTest : public LeakDetectionFixture
    {
    public:
        void run()
        {
            NameDictionaryTester::Create();

            {
                AZ::BehaviorContext* behaviorContext = aznew AZ::BehaviorContext();
                behaviorContext->Method("AZTestAssert", &AZTestAssert);

                AZ::Name::Reflect(behaviorContext);

                AZ::ScriptContext* scriptContext = aznew AZ::ScriptContext();
                scriptContext->BindTo(behaviorContext);

                scriptContext->Execute("azName = Name(\"First\");");
                scriptContext->Execute("AZTestAssert(azName:IsEmpty() == false);");
                scriptContext->Execute("azName2 = Name(\"First\");");
                scriptContext->Execute("AZTestAssert(azName == azName2);");
                scriptContext->Execute("azName3 = Name(\"Second\");");
                scriptContext->Execute("AZTestAssert(azName3 ~= azName2);");

                AZ_TEST_START_ASSERTTEST;
                scriptContext->Execute("azName4 = Name(0);");
                scriptContext->Execute("azName5 = Name(\"a\", \"b\");");
                AZ_TEST_STOP_ASSERTTEST(2);

                delete scriptContext;
                delete behaviorContext;
            }

            NameDictionaryTester::Destroy();
        }
    };

    TEST_F(NameScriptingTest, Test)
    {
        run();
    }

    template<class ConcurrencyTestThreadT>
    void RunConcurrencyTest(uint32_t nameCount, uint32_t threadsPerName)
    {        
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 0);

        // Use a fixed seed.
        srand(123456);

        AZStd::set<AZStd::string> localDictionary;
        for (uint32_t i = 0; i < nameCount; i++)
        {
            const int length = rand() % 5 + 10;
            AZStd::string name(length, '\0');
            AZStd::generate(name.begin(), name.end(), [] {
                const char asciiBase = ((rand() % 2) ? 'a' : 'A');

                // Generate random Ascii values.
                return static_cast<char>(rand() % 26 + asciiBase);
            });

            localDictionary.emplace(AZStd::move(name));
        }

        AZStd::vector<ConcurrencyTestThreadT> threads;
        for (const AZStd::string& nameString : localDictionary)
        {
            for (uint32_t i = 0; i < threadsPerName; ++i)
            {
                threads.push_back(ConcurrencyTestThreadT(nameString));
            }
        }

        for (ConcurrencyTestThreadT& th : threads)
        {
            th.Start();
        }

        for (ConcurrencyTestThreadT& th : threads)
        {
            th.Join();
        }

        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), localDictionary.size());

        // Make sure all entries in the localDictionary got copied into the globalDictionary
        for (const AZStd::string& nameString : localDictionary)
        {
            auto& globalDictionary = NameDictionaryTester::GetDictionary();
            // Workaround VS2022 17.3 issue with incorrect detection of unused lambda captures assigning the nameString reference to a same type
            auto it = AZStd::find_if(globalDictionary.begin(), globalDictionary.end(), [&nameString = nameString](const AZStd::pair<AZ::Name::Hash, AZ::NameDictionary::ScopedNameDataWrapper>& entry)
            {
                return entry.second.m_nameData->GetName() == nameString;
            });
            EXPECT_TRUE(it != globalDictionary.end()) << "Can't find '" << nameString.data() << "' in local dictionary.";
        }

        // Make sure all the threads got an accurate Name object
        for (ConcurrencyTestThreadT& th : threads)
        {
            AZ::Name lookupName = AZ::NameDictionary::Instance().FindName(th.GetAzName().GetHash());
            EXPECT_EQ(th.GetAzName().GetStringView(), lookupName.GetStringView());
        }
    }
    
    TEST_F(NameTest, NameConstructorTest)
    {
        AZ::Name a("a");
        EXPECT_FALSE(a.IsEmpty());
        EXPECT_EQ(a.GetStringView(), "a");
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        AZ::Name a2;
        EXPECT_TRUE(a2.IsEmpty());
        EXPECT_TRUE(a2.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        a2 = a;
        EXPECT_FALSE(a2.IsEmpty());
        EXPECT_EQ(a2.GetStringView().data(), a.GetStringView().data());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        a = AZStd::move(a2);
        EXPECT_TRUE(a2.IsEmpty());
        EXPECT_FALSE(a.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        AZ::Name a3(AZStd::move(a));
        EXPECT_FALSE(a3.IsEmpty());
        EXPECT_TRUE(a.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        AZ::Name a4(a3);
        EXPECT_FALSE(a3.IsEmpty());
        EXPECT_FALSE(a4.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        AZ::Name b;
        EXPECT_TRUE(b.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        b = "b";
        EXPECT_EQ(b.GetStringView().compare("b"), 0);
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 2);

        b = AZStd::string{"b2"};
        EXPECT_EQ(b.GetStringView().compare("b2"), 0);
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 2);
            
        a3 = AZ::Name{};
        EXPECT_TRUE(a3.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 2);

        a4 = AZ::Name{};
        EXPECT_TRUE(a4.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

        b = AZ::Name{};
        EXPECT_TRUE(b.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 0);

        b = "q";
        EXPECT_EQ(b.GetStringView().compare("q"), 0);
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);
            
        b = a;
        EXPECT_TRUE(b.IsEmpty());
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 0);

        // Test specific construction case that was failing. 
        // The constructor calls Name::SetName() which does a move assignment
        // Name& Name::operator=(Name&& rhs) was leaving m_view pointing to the m_data in a temporary Name object.
        AZ::Name emptyName(AZStd::string_view{});
        EXPECT_TRUE(emptyName.IsEmpty());
        EXPECT_EQ(0, emptyName.GetStringView().data()[0]);
    }

    TEST_F(NameTest, EmptyNameIsNotInDictionary)
    {
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 0);
        AZ::Name a;
        AZ::Name b{""};
        AZ::Name c{"temp"};
        c = "";
        EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 0);
    }

    TEST_F(NameTest, NameConstructFromHash)
    {
        // Limit the number of hashes to make sure construction from hash works when there are hash collisions.
        const uint32_t maxUniqueHashes = 10;
        AZ::NameDictionary::Destroy();
        AZ::NameDictionary::Create();

        AZ::Name nameA{"a"};
        AZ::Name nameB{"B"};

        AZ::Name name;
        
        name = AZ::Name{AZ::Name::Hash{0u}};
        EXPECT_TRUE(name.IsEmpty());

        name = AZ::Name{nameA.GetHash()};
        EXPECT_TRUE(name.GetStringView() == nameA.GetStringView());
        EXPECT_TRUE(name == nameA);

        name = AZ::Name{nameB.GetHash()};
        EXPECT_TRUE(name.GetStringView() == nameB.GetStringView());
        EXPECT_TRUE(name == nameB);

        AZStd::array<AZ::Name, maxUniqueHashes * 10> nameList;
        for (size_t i = 0; i < nameList.size(); ++i)
        {
            nameList[i] = AZ::Name{AZStd::string::format("name %zu", i)};

            AZ::Name nameFromHash{nameList[i].GetHash()};
            EXPECT_TRUE(nameFromHash.GetStringView() == nameList[i].GetStringView());
            EXPECT_TRUE(nameFromHash == nameList[i]);
        }

        // Check again after the dictionary contents have stabilized
        for (AZ::Name& originalName : nameList)
        {
            AZ::Name nameFromHash{originalName.GetHash()};

            EXPECT_TRUE(nameFromHash.GetStringView() == originalName.GetStringView());
            EXPECT_TRUE(nameFromHash == originalName);
        }
    }

    TEST_F(NameTest, NameComparisonTest)
    {
        AZ::Name a{"a"};
        AZ::Name b{"b"};

        // == is true

        EXPECT_TRUE(AZ::Name{} == AZ::Name{});
        EXPECT_TRUE(a == a);

        // != is false

        EXPECT_FALSE(AZ::Name{} != AZ::Name{});
        EXPECT_FALSE(a != a);

        // == is false

        EXPECT_FALSE(a == b);
        EXPECT_FALSE(a == AZ::Name{});
        EXPECT_FALSE(b == AZ::Name{});

        // != is true

        EXPECT_TRUE(a != b);
        EXPECT_TRUE(a != AZ::Name{});
        EXPECT_TRUE(b != AZ::Name{});
    }

    TEST_F(NameTest, CollisionResolutionsArePersistent)
    {
        // When hash calculations collide, the resolved hash value is order-dependent and therefore is not guaranteed
        // be the same between runs of the application. However, the resolved hash values ARE guaranteed to be preserved
        // during the dictionary lifetime. This is a precautionary measure to avoid possible edge cases.

        const uint32_t maxUniqueHashes = 50;
        AZ::NameDictionary::Destroy();
        AZ::NameDictionary::Create();

        // We need this number of names to collide for the test to be valid
        const size_t requiredCollisionCount = 3;

        // Find a set of names that collide
        AZStd::map<uint32_t, AZStd::vector<AZStd::string>> hashTable;
        AZStd::vector<AZStd::string>* collidingNameSet = nullptr;
        for (int i = 0; i < 1000; ++i)
        {
            AZStd::string nameText = AZStd::string::format("name%d", i);
            uint32_t hash = NameDictionaryTester::CalcDirectHashValue(nameText, maxUniqueHashes);

            collidingNameSet = &hashTable[hash];

            collidingNameSet->push_back(nameText);

            if (collidingNameSet->size() >= requiredCollisionCount)
            {
                break;
            }
        }

        ASSERT_NE(collidingNameSet, nullptr);
        ASSERT_GE(collidingNameSet->size(), requiredCollisionCount);

        AZStd::unique_ptr<AZ::Name> nameA = AZStd::make_unique<AZ::Name>((*collidingNameSet)[0]);
        AZStd::unique_ptr<AZ::Name> nameB = AZStd::make_unique<AZ::Name>((*collidingNameSet)[1]);
        AZStd::unique_ptr<AZ::Name> nameC = AZStd::make_unique<AZ::Name>((*collidingNameSet)[2]);

        // Even though we destroy nameA and nameB, we should still be able to retrieve nameC.
        nameA.reset();
        nameB.reset();

        AZ::Name newNameC{(*collidingNameSet)[2]};

        EXPECT_EQ(newNameC, *nameC);
        EXPECT_EQ(newNameC.GetStringView(), nameC->GetStringView());
    }

    TEST_F(NameTest, ReportLeakedNames)
    {
        AZ::Internal::NameData* leakedNameData = nullptr;
        {
            constexpr AZStd::string_view leakedNameContents{ "hello" };
            AZ::Name leakedName{ leakedNameContents };
            AZ_TEST_START_TRACE_SUPPRESSION;
            AZ::NameDictionary::Destroy();
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            leakedNameData = GetNameData(leakedName);

            // Create the dictionary again to avoid crash when the intrusive_ptr in Name tries to access NameDictionary to free it
            AZ::NameDictionary::Create();
        } 
        
        FreeMemoryFromNameData(leakedNameData); // free it to avoid memory system reporting the leak
    }

    TEST_F(NameTest, NullTerminatedTest)
    {
        const char* buffer = "There is a name in here";
        
        AZ::Name a{AZStd::string_view(&buffer[9], 1)};

        AZ::Name b;
        b = AZStd::string_view(&buffer[11], 4);

        EXPECT_TRUE(a.GetStringView() == "a");
        EXPECT_TRUE(a.GetStringView()[1] == '\0');
        EXPECT_TRUE(a.GetStringView().data() == a.GetCStr());

        EXPECT_TRUE(b.GetStringView() == "name");
        EXPECT_TRUE(b.GetStringView()[4] == '\0');
        EXPECT_TRUE(b.GetStringView().data() == b.GetCStr());
    }

    TEST_F(NameTest, SerializationTest)
    {
        AZ::SerializeContext* serializeContext = aznew AZ::SerializeContext();
        AZ::Name::Reflect(serializeContext);

        const char* testString = "MyReflectedString";

        auto doSerialization = [&](AZ::ObjectStream::StreamType streamType)
        {
            AZ::Name reflectedName(testString);
            EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);

            AZStd::vector<char, AZ::OSStdAllocator> nameBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator> > outStream(&nameBuffer);

            {
                AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&outStream, *serializeContext, streamType);

                bool writeOK = objStream->WriteClass(&reflectedName);
                ASSERT_TRUE(writeOK);

                bool finalizeOK = objStream->Finalize();
                ASSERT_TRUE(finalizeOK);
            }
            outStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            reflectedName = AZ::Name{};
            EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 0);

            AZ::ObjectStream::FilterDescriptor filterDesc;
            AZ::Name* serializedName = AZ::Utils::LoadObjectFromStream<AZ::Name>(outStream, serializeContext, filterDesc);
            reflectedName = *serializedName;
            delete serializedName;

            EXPECT_EQ(NameDictionaryTester::GetEntryCount(), 1);
            EXPECT_EQ(reflectedName.GetStringView(), testString);
        };

        doSerialization(AZ::ObjectStream::ST_BINARY);
        doSerialization(AZ::ObjectStream::ST_XML);
        doSerialization(AZ::ObjectStream::ST_JSON);

        delete serializeContext;
    }
    
    TEST_F(NameTest, NameContainerTest)
    {
        AZStd::unordered_set<AZ::Name> nameSet;
        nameSet.insert(AZ::Name{ "c" });
        nameSet.insert(AZ::Name{ "a" });
        nameSet.insert(AZ::Name{ "b" });
        nameSet.insert(AZ::Name{ "b" });
        nameSet.insert(AZ::Name{ "a" });
        nameSet.insert(AZ::Name{ "b" });

        EXPECT_EQ(3, nameSet.size());

        EXPECT_EQ(1, nameSet.count(AZ::Name{ "a" }));
        EXPECT_EQ(1, nameSet.count(AZ::Name{ "b" }));
        EXPECT_EQ(1, nameSet.count(AZ::Name{ "c" }));

        EXPECT_EQ(0, nameSet.count(AZ::Name{}));
        EXPECT_EQ(0, nameSet.count(AZ::Name{ "d" }));
    }

    TEST_F(NameTest, ConcurrencyDataTest_EachThreadCreatesOneName_NoCollision)
    {
        AZ::NameDictionary::Destroy();
        AZ::NameDictionary::Create();

        // 3 threads per name effectively makes two readers and one writer (the first to run will write in the dictionary)
        RunConcurrencyTest<ThreadCreatesOneName>(AZStd::thread::hardware_concurrency(), 3);
    }

    TEST_F(NameTest, ConcurrencyDataTest_EachThreadCreatesOneName_HighCollisions)
    {
        AZ::NameDictionary::Destroy();
        AZ::NameDictionary::Create();

        // 3 threads per name effectively makes two readers and one writer (the first to run will write in the dictionary)
        RunConcurrencyTest<ThreadCreatesOneName>(AZStd::thread::hardware_concurrency() / 2, 3);
    }

    TEST_F(NameTest, ConcurrencyDataTest_EachThreadRepeatedlyCreatesAndReleasesOneName_NoCollision)
    {
        AZ::NameDictionary::Destroy();
        AZ::NameDictionary::Create();

        // We use only 2 threads for each name to try and maximize how much names are added and removed,
        // instead of letting shared references cause the name to stay in the dictionary.
        RunConcurrencyTest<ThreadRepeatedlyCreatesAndReleasesOneName<100>>(100, 2);
    }

    TEST_F(NameTest, ConcurrencyDataTest_EachThreadRepeatedlyCreatesAndReleasesOneName_HighCollisions)
    {
        AZ::NameDictionary::Destroy();

        // Create an instance of the NameDictionary with the max hash slots set to 1
        // to produce collisions on every name.
        ASSERT_EQ(nullptr, AZ::Interface<AZ::NameDictionary>::Get());
        constexpr AZ::u64 maxHashSlots = 1;
        AZStd::unique_ptr<AZ::NameDictionary> nameDictionary = AZStd::make_unique<AZ::NameDictionary>(maxHashSlots);
        AZ::Interface<AZ::NameDictionary>::Register(nameDictionary.get());

        // We use only 2 threads for each name to try and maximize how much names are added and removed,
        // instead of letting shared references cause the name to stay in the dictionary.
        RunConcurrencyTest<ThreadRepeatedlyCreatesAndReleasesOneName<100>>(100, 2);

        AZ::Interface<AZ::NameDictionary>::Unregister(nameDictionary.get());
    }

    TEST_F(NameTest, ConcurrencyDataTest_EachThreadRepeatedlyCreatesAndReleasesOneName_AndAccessItOnManyThread)
    {
        AZ::NameDictionary::Destroy();
        AZ::NameDictionary::Create();

        // We use only 1 name, but 200 threads to catch any race condtions in the NameData::release function
        // Once that function decrements the m_useCount to 0, it should never be incremented again.
        RunConcurrencyTest<ThreadRepeatedlyCreatesAndReleasesOneName<100>>(1, 2);
    }

    TEST_F(NameTest, NameRef)
    {
        AZ::NameRef fromRValue = AZ::Name("test");
        EXPECT_EQ("test", fromRValue.GetStringView());

        AZ::Name name("foo");
        AZ::NameRef fromLValue = name;
        EXPECT_EQ("foo", fromLValue.GetStringView());

        AZ::Name fromRefLValue = fromLValue;
        EXPECT_EQ("foo", fromRefLValue.GetStringView());

        AZ::Name fromRefRValue = AZStd::move(fromRValue);
        EXPECT_EQ("test", fromRefRValue.GetStringView());

        EXPECT_TRUE(AZ::Name(fromLValue) == fromRefLValue);
        EXPECT_TRUE(fromLValue == fromRefLValue);
        EXPECT_TRUE(fromRefLValue == AZ::Name(fromLValue));
        EXPECT_TRUE(fromLValue == AZ::Name("foo"));
        EXPECT_TRUE(AZ::Name("foo") == fromLValue);
    }

    TEST_F(NameTest, NameLiteral)
    {
        static AZ::Name staticName = AZ::Name::FromStringLiteral("static", AZ::Interface<AZ::NameDictionary>::Get());
        EXPECT_EQ("literal", AZ_NAME_LITERAL("literal").GetStringView());
        EXPECT_EQ("static", staticName.GetStringView());
        EXPECT_EQ("global", globalName.GetStringView());
    }

    TEST_F(NameTest, DISABLED_NameVsStringPerf_Creation)
    {
        constexpr int CreateCount = 1000;

        char buffer[RandomStringBufferSize];

        AZStd::sys_time_t newNameTime;
        [[maybe_unused]] AZStd::sys_time_t existingNameTime;
        AZStd::sys_time_t stringTime;

        {
            const size_t dictionaryNoiseSize = 1000;

            AZStd::vector<AZ::Name> existingNames;
            existingNames.reserve(dictionaryNoiseSize);
            for (int i = 0; i < dictionaryNoiseSize; ++i)
            {
                existingNames.push_back(AZ::Name{ MakeRandomString(buffer) });
            }

            EXPECT_EQ(dictionaryNoiseSize, NameDictionaryTester::GetEntryCount());
            
            {
                const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

                for (int i = 0; i < CreateCount; ++i)
                {
                    AZ::Name name{ MakeRandomString(buffer) };
                }

                newNameTime = AZStd::GetTimeNowMicroSecond() - startTime;
            }

            {
                const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

                for (int i = 0; i < CreateCount; ++i)
                {
                    const int randomEntry = m_random.GetRandom() % dictionaryNoiseSize;
                    // Make a copy of the string just in case there are optimizations in the Name system when
                    // same string pointer is used to construct a new Name. What we want to test here is looking
                    // up a name in the dictionary from new data.
                    azstrcpy(buffer, RandomStringBufferSize, existingNames[randomEntry].GetCStr());
                    AZ::Name name{ buffer };
                }

                existingNameTime = AZStd::GetTimeNowMicroSecond() - startTime;
            }


            // This isn't relevant to the performance test, but we might as well check this while we're here
            existingNames.clear();
            EXPECT_EQ(0, NameDictionaryTester::GetEntryCount());
        }

        {
            const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

            for (int i = 0; i < CreateCount; ++i)
            {
                AZStd::string s{ MakeRandomString(buffer) };
            }

            stringTime = AZStd::GetTimeNowMicroSecond() - startTime;
        }

        AZ_TracePrintf("NameTest", "Create %d strings:        %d us\n", CreateCount, stringTime);
        AZ_TracePrintf("NameTest", "Create %d new names:      %d us\n", CreateCount, newNameTime);
        AZ_TracePrintf("NameTest", "Create %d existing names: %d us\n", CreateCount, existingNameTime);

        // It is known that NameDictionary is slower at creation time for existing entries.
        EXPECT_GT(newNameTime, stringTime);
    }

    TEST_F(NameTest, DISABLED_NameVsStringPerf_Comparison)
    {
        constexpr int CompareCount = 10000;
        
        AZStd::string stringA = "BlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlah 1";
        AZStd::string stringB = "BlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlahBlah 2";
        AZStd::string stringC = "Blah";

        AZStd::sys_time_t nameTime;
        AZStd::sys_time_t stringTime;

        {
            AZ::Name a1{ stringA };
            AZ::Name a2{ stringA };
            AZ::Name b{ stringB };
            AZ::Name c{ stringC };

            const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

            bool result = true;

            for (int i = 0; i < CompareCount; ++i)
            {
                result &= (a1 == a2);
                result &= (a1 != b);
                result &= (a1 != c);
            }

            nameTime = AZStd::GetTimeNowMicroSecond() - startTime;

            EXPECT_TRUE(result);
        }

        {
            AZStd::string a1{ stringA };
            AZStd::string a2{ stringA };
            AZStd::string b{ stringB };
            AZStd::string c{ stringC };

            const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

            bool result = true;

            for (int i = 0; i < CompareCount; ++i)
            {
                result &= (a1 == a2);
                result &= (a1 != b);
                result &= (a1 != c);
            }

            stringTime = AZStd::GetTimeNowMicroSecond() - startTime;

            EXPECT_TRUE(result);
        }

        AZ_TracePrintf("NameTest", "Compare %d strings:  %d us\n", CompareCount, stringTime);
        AZ_TracePrintf("NameTest", "Compare %d names:    %d us\n", CompareCount, nameTime);

        EXPECT_LE(nameTime, stringTime);
    }

    // LY-115684 This test is unstable to run in parallel since the load during the first part of the loop could be different
    // than the load during the second part
    TEST_F(NameTest, DISABLED_NameVsStringPerf_UnorderedSearch)
    {
        constexpr int SearchCount = 10000;
        constexpr int EntryCount = 10000;

        char buffer[RandomStringBufferSize];

        AZStd::sys_time_t nameTime;
        AZStd::sys_time_t stringTime;

        {
            AZStd::unordered_map<AZ::Name, int> nameMap;
            AZStd::vector<AZ::Name> nameList;
            for (int i = 0; i < EntryCount; ++i)
            {
                AZ::Name name{ MakeRandomString(buffer) };
                nameMap[name] = i;
                nameList.push_back(name);
            }

            bool result = true;

            const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

            for (int i = 0; i < SearchCount; ++i)
            {
                const int randomEntry = m_random.GetRandom() % EntryCount;
                auto& name = nameList[randomEntry];
                auto iter = nameMap.find(name);
                result &= (randomEntry == iter->second);
            }

            nameTime = AZStd::GetTimeNowMicroSecond() - startTime;

            EXPECT_TRUE(result);

            nameMap.clear();
            nameList.clear();
        }

        {
            AZStd::unordered_map<AZStd::string, int> nameMap;
            AZStd::vector<AZStd::string> nameList;
            for (int i = 0; i < EntryCount; ++i)
            {
                AZStd::string name{ MakeRandomString(buffer) };
                nameMap[name] = i;
                nameList.push_back(name);
            }

            bool result = true;

            const AZStd::sys_time_t startTime = AZStd::GetTimeNowMicroSecond();

            for (int i = 0; i < SearchCount; ++i)
            {
                const int randomEntry = m_random.GetRandom() % EntryCount;
                auto& name = nameList[randomEntry];
                auto iter = nameMap.find(name);
                result &= (randomEntry == iter->second);
            }

            stringTime = AZStd::GetTimeNowMicroSecond() - startTime;

            EXPECT_TRUE(result);
        }

        AZ_TracePrintf("NameTest", "Search for %d strings:    %d us\n", SearchCount, stringTime);
        AZ_TracePrintf("NameTest", "Search for %d names:      %d us\n", SearchCount, nameTime);

        EXPECT_LT(nameTime, stringTime);
    }

    // Fixture for validating that multiple NameDictionarys can exist at once.
    class MultiNameDictionaryFixture
        : public LeakDetectionFixture
    {
    public:
        MultiNameDictionaryFixture()
        {
            m_nameDictionary1 = AZStd::make_unique<AZ::NameDictionary>();
            m_nameDictionary2 = AZStd::make_unique<AZ::NameDictionary>();
        }

        ~MultiNameDictionaryFixture()
        {
            m_nameDictionary2.reset();
            m_nameDictionary1.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::NameDictionary> m_nameDictionary1;
        AZStd::unique_ptr<AZ::NameDictionary> m_nameDictionary2;
    };

    TEST_F(MultiNameDictionaryFixture, MultipleDictionaries_Contains_DifferentNameInstances)
    {
        // Dictionary 1 will contain the names of "Name1" and "Dictionary1NameOnly"
        AZ::Name name1FromDict1 = m_nameDictionary1->MakeName("Name1");
        AZ::Name dict1OnlyName = m_nameDictionary1->MakeName("Dictionary1NameOnly");

        // Dictionary 2 will contain the names of "Name1" and "Dictionary2NameOnly"
        AZ::Name name1FromDict2 = m_nameDictionary2->MakeName("Name1");
        AZ::Name dict2OnlyName = m_nameDictionary2->MakeName("Dictionary2NameOnly");

        // First dictionary EXPECTS
        EXPECT_FALSE(m_nameDictionary1->FindName(name1FromDict1.GetHash()).IsEmpty());
        EXPECT_FALSE(m_nameDictionary1->FindName(dict1OnlyName.GetHash()).IsEmpty());
        // The first dictionary should NOT contain Dictionary2NameOnly
        EXPECT_TRUE(m_nameDictionary1->FindName(dict2OnlyName.GetHash()).IsEmpty());

        // Second dictionary EXPECTS
        EXPECT_FALSE(m_nameDictionary2->FindName(name1FromDict1.GetHash()).IsEmpty());
        EXPECT_FALSE(m_nameDictionary2->FindName(dict2OnlyName.GetHash()).IsEmpty());
        // The second dictionary should NOT contain Dictionary1NameOnly
        EXPECT_TRUE(m_nameDictionary2->FindName(dict1OnlyName.GetHash()).IsEmpty());

        // reset the name1FromDict1 variable
        // Only the reference to "Name1" entry in first dictionary should be removed
        const AZ::Name::Hash name1Hash = name1FromDict1.GetHash();
        name1FromDict1 = AZ::Name{};

        EXPECT_TRUE(m_nameDictionary1->FindName(name1Hash).IsEmpty());
        EXPECT_FALSE(m_nameDictionary2->FindName(name1FromDict2.GetHash()).IsEmpty());
    }

    TEST_F(MultiNameDictionaryFixture, NameLiteral_IsOnlyLinkedToSingleNameDictionary)
    {
        // When run with --gtest_repeat=2 the literal is not relinked to the NameDictionary
        // if a static variable is used due to not running the constructor
        const AZ::Name staticName = AZ::Name::FromStringLiteral("firstStatic", m_nameDictionary1.get());
        const AZ::Name literalRef = AZ::Name::FromStringLiteral("secondLiteral", m_nameDictionary2.get());

        // "firstStatic" should be in dictionary1, but not dictionary2
        EXPECT_FALSE(m_nameDictionary1->FindName(staticName.GetHash()).IsEmpty());
        EXPECT_TRUE(m_nameDictionary2->FindName(staticName.GetHash()).IsEmpty());

        // "secondLiteral" should be in dictionary2, but not dictionary1
        EXPECT_FALSE(m_nameDictionary2->FindName(literalRef.GetHash()).IsEmpty());
        EXPECT_TRUE(m_nameDictionary1->FindName(literalRef.GetHash()).IsEmpty());

        // Add a "firstStatic" to second dictionary
        const AZ::Name staticNameDict2{ AZ::Name::FromStringLiteral("firstStatic", m_nameDictionary2.get()) };
        EXPECT_FALSE(m_nameDictionary1->FindName(staticName.GetHash()).IsEmpty());
        EXPECT_FALSE(m_nameDictionary2->FindName(staticNameDict2.GetHash()).IsEmpty());

        // Now create a second name literal for the string literal of "secondLiteral"
        // There should be two different "secondLiteral" instance each associated with a different name dictionary
        const AZ::Name literalRefForDict1 = AZ::Name::FromStringLiteral("secondLiteral", m_nameDictionary1.get());

        EXPECT_EQ(literalRef, literalRefForDict1) << "The values of the names should be equal";
        EXPECT_NE(&literalRef, &literalRefForDict1) << "The memory address of the names should not be equal";

        EXPECT_FALSE(m_nameDictionary1->FindName(literalRef.GetHash()).IsEmpty());
        EXPECT_FALSE(m_nameDictionary2->FindName(literalRefForDict1.GetHash()).IsEmpty());
    }
}

