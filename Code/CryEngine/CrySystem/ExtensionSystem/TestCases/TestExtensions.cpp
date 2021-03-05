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

// Description : Part of CryEngine's extension framework.


#include "CrySystem_precompiled.h"
#include "TestExtensions.h"

#ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/Impl/ICryFactoryRegistryImpl.h>
#include <CryExtension/CryCreateClassInstance.h>


//////////////////////////////////////////////////////////////////////////


namespace TestComposition
{
    struct ITestExt1
        : public ICryUnknown
    {
        CRYINTERFACE_DECLARE(ITestExt1, 0x9d9e0dcfa5764cb0, 0xa73701595f75bd32)

        virtual void Call1() const = 0;
    };

    DECLARE_SMART_POINTERS(ITestExt1);


    class CTestExt1
        : public ITestExt1
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(ITestExt1)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CTestExt1, "TestExt1", 0x43b04e7cc1be45ca, 0x9df6ccb1c0dc1ad8)

    public:
        virtual void Call1() const;

    private:
        int i;
    };

    CRYREGISTER_CLASS(CTestExt1)

    CTestExt1::CTestExt1()
    {
        i = 1;
    }

    CTestExt1::~CTestExt1()
    {
        printf("Inside CTestExt1 dtor\n");
    }

    void CTestExt1::Call1() const
    {
        printf("Inside CTestExt1::Call1()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    struct ITestExt2
        : public ICryUnknown
    {
        CRYINTERFACE_DECLARE(ITestExt2, 0x8eb7a4b399874b9c, 0xb96bd6da7a8c72f9)

        virtual void Call2() = 0;
    };

    DECLARE_SMART_POINTERS(ITestExt2);


    class CTestExt2
        : public ITestExt2
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(ITestExt2)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CTestExt2, "TestExt2", 0x25b3ebf8f1754b9a, 0xb5494e3da7cdd80f)

    public:
        virtual void Call2();

    private:
        int i;
    };

    CRYREGISTER_CLASS(CTestExt2)

    CTestExt2::CTestExt2()
    {
        i = 2;
    }

    CTestExt2::~CTestExt2()
    {
        printf("Inside CTestExt2 dtor\n");
    }

    void CTestExt2::Call2()
    {
        printf("Inside CTestExt2::Call2()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    class CComposed
        : public ICryUnknown
    {
        CRYGENERATE_CLASS(CComposed, "Composed", 0x0439d74b8dcd4b7f, 0x9287dcdf7e26a3a5)

        CRYCOMPOSITE_BEGIN()
        CRYCOMPOSITE_ADD(m_pTestExt1, "Ext1")
        CRYCOMPOSITE_ADD(m_pTestExt2, "Ext2")
        CRYCOMPOSITE_END(CComposed)

        CRYINTERFACE_BEGIN()
        CRYINTERFACE_END()

    private:
        ITestExt1Ptr m_pTestExt1;
        ITestExt2Ptr m_pTestExt2;
    };

    CRYREGISTER_CLASS(CComposed)

    CComposed::CComposed()
        : m_pTestExt1()
        , m_pTestExt2()
    {
        CryCreateClassInstance("TestExt1", m_pTestExt1);
        CryCreateClassInstance("TestExt2", m_pTestExt2);
    }

    CComposed::~CComposed()
    {
    }

    //////////////////////////////////////////////////////////////////////////

    struct ITestExt3
        : public ICryUnknown
    {
        CRYINTERFACE_DECLARE(ITestExt3, 0xdd017935a2134898, 0xbd2fffa145551876)

        virtual void Call3() = 0;
    };

    DECLARE_SMART_POINTERS(ITestExt3);

    class CTestExt3
        : public ITestExt3
    {
        CRYGENERATE_CLASS(CTestExt3, "TestExt3", 0xeceab40bc4bb4988, 0xa9f63c1db85a69b1)

        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(ITestExt3)
        CRYINTERFACE_END()

    public:
        virtual void Call3();

    private:
        int i;
    };

    CRYREGISTER_CLASS(CTestExt3)

    CTestExt3::CTestExt3()
    {
        i = 3;
    }

    CTestExt3::~CTestExt3()
    {
        printf("Inside CTestExt3 dtor\n");
    }

    void CTestExt3::Call3()
    {
        printf("Inside CTestExt3::Call3()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    class CComposed2
        : public ICryUnknown
    {
        CRYGENERATE_CLASS(CComposed2, "Composed2", 0x0439d74b8dcd4b7e, 0x9287dcdf7e26a3a6)

        CRYCOMPOSITE_BEGIN()
        CRYCOMPOSITE_ADD(m_pTestExt3, "Ext3")
        CRYCOMPOSITE_END(CComposed2)

        CRYINTERFACE_BEGIN()
        CRYINTERFACE_END()

    private:
        ITestExt3Ptr m_pTestExt3;
    };

    CRYREGISTER_CLASS(CComposed2)

    CComposed2::CComposed2()
        : m_pTestExt3()
    {
        CryCreateClassInstance("TestExt3", m_pTestExt3);
    }

    CComposed2::~CComposed2()
    {
    }

    //////////////////////////////////////////////////////////////////////////

    class CTestExt4
        : public ITestExt1
        , public ITestExt2
        , public ITestExt3
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(ITestExt1)
        CRYINTERFACE_ADD(ITestExt2)
        CRYINTERFACE_ADD(ITestExt3)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CTestExt4, "TestExt4", 0x43204e7cc1be45ca, 0x9df4ccb1c0dc1ad8)

    public:
        virtual void Call1() const;
        virtual void Call2();
        virtual void Call3();

    private:
        int i;
    };

    CRYREGISTER_CLASS(CTestExt4)

    CTestExt4::CTestExt4()
    {
        i = 4;
    }

    CTestExt4::~CTestExt4()
    {
        printf("Inside CTestExt4 dtor\n");
    }

    void CTestExt4::Call1() const
    {
        printf("Inside CTestExt4::Call1()\n");
    }

    void CTestExt4::Call2()
    {
        printf("Inside CTestExt4::Call2()\n");
    }

    void CTestExt4::Call3()
    {
        printf("Inside CTestExt4::Call3()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    class CMegaComposed
        : public CComposed
        , public CComposed2
    {
        CRYGENERATE_CLASS(CMegaComposed, "MegaComposed", 0x512787559f84503, 0x421ac1af66f2fb6f)

        CRYCOMPOSITE_BEGIN()
        CRYCOMPOSITE_ADD(m_pTestExt4, "Ext4")
        CRYCOMPOSITE_ENDWITHBASE2(CMegaComposed, CComposed, CComposed2)

        CRYINTERFACE_BEGIN()
        CRYINTERFACE_END()

    private:
        AZStd::shared_ptr<CTestExt4> m_pTestExt4;
    };

    CRYREGISTER_CLASS(CMegaComposed)

    CMegaComposed::CMegaComposed()
        : m_pTestExt4()
    {
        printf("Inside CMegaComposed ctor\n");
        m_pTestExt4 = CTestExt4::CreateClassInstance();
    }

    CMegaComposed::~CMegaComposed()
    {
        printf("Inside CMegaComposed dtor\n");
    }

    //////////////////////////////////////////////////////////////////////////

    static void TestComposition()
    {
        printf("\nTest composition:\n");

        ICryUnknownPtr p;
        if (CryCreateClassInstance("MegaComposed", p))
        {
            ITestExt1Ptr p1 = cryinterface_cast<ITestExt1>(crycomposite_query(p, "Ext1"));
            if (p1)
            {
                p1->Call1(); // calls CTestExt1::Call1()
            }
            ITestExt2Ptr p2 = cryinterface_cast<ITestExt2>(crycomposite_query(p, "Ext2"));
            if (p2)
            {
                p2->Call2(); // calls CTestExt2::Call2()
            }
            ITestExt3Ptr p3 = cryinterface_cast<ITestExt3>(crycomposite_query(p, "Ext3"));
            if (p3)
            {
                p3->Call3(); // calls CTestExt3::Call3()
            }
            p3 = cryinterface_cast<ITestExt3>(crycomposite_query(p, "Ext4"));
            if (p3)
            {
                p3->Call3(); // calls CTestExt4::Call3()
            }
            p1 = cryinterface_cast<ITestExt1>(crycomposite_query(p.get(), "Ext4"));
            p2 = cryinterface_cast<ITestExt2>(crycomposite_query(p.get(), "Ext4"));

            bool b = CryIsSameClassInstance(p1, p2); // true
        }

        {
            ICryUnknownConstPtr pCUnk = p;
            ICryUnknownConstPtr pComp1 = crycomposite_query(pCUnk.get(), "Ext1");
            //ICryUnknownPtr pComp1 = crycomposite_query(pCUnk, "Ext1"); // must fail to compile due to const rules

            ITestExt1ConstPtr p1 = cryinterface_cast<const ITestExt1>(pComp1);
            if (p1)
            {
                p1->Call1();
            }
        }
    }
} // namespace TestComposition


//////////////////////////////////////////////////////////////////////////


namespace TestExtension
{
    class CFoobar
        : public IFoobar
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IFoobar)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CFoobar, "Foobar", 0x76c8dd6d16634531, 0x95d3b1cfabcf7ef4)

    public:
        virtual void Foo();
    };

    CRYREGISTER_CLASS(CFoobar)

    CFoobar::CFoobar()
    {
    }

    CFoobar::~CFoobar()
    {
    }

    void CFoobar::Foo()
    {
        printf("Inside CFoobar::Foo()\n");
    }

    static void TestFoobar()
    {
        AZStd::shared_ptr<CFoobar> p = CFoobar::CreateClassInstance();
        {
            CryInterfaceID iid = cryiidof<IFoobar>();
            CryClassID clsid = p->GetFactory()->GetClassID();
            int t = 0;
        }

        {
            IAPtr sp_ = cryinterface_cast<IA>(p); // sp_ == NULL

            ICryUnknownPtr sp1 = cryinterface_cast<ICryUnknown>(p);
            IFoobarPtr sp = cryinterface_cast<IFoobar>(sp1);
            sp->Foo();
        }

        {
            CFoobar* pF = p.get();
            pF->Foo();
            ICryUnknown* p1 = cryinterface_cast<ICryUnknown>(pF);
        }

        IFoobar* pFoo = cryinterface_cast<IFoobar>(p.get());
        ICryFactory* pF1 = pFoo->GetFactory();
        pFoo->Foo();

        int t = 0;
    }

    //////////////////////////////////////////////////////////////////////////

    class CRaboof
        : public IRaboof
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IRaboof)
        CRYINTERFACE_END()

        CRYGENERATE_SINGLETONCLASS(CRaboof, "Raabof", 0xba482ce12b2e4309, 0x8238ed8b52cb1f1e)

    public:
        virtual void Rab();
    };

    CRYREGISTER_SINGLETON_CLASS(CRaboof)

    CRaboof::CRaboof()
    {
    }

    CRaboof::~CRaboof()
    {
    }

    void CRaboof::Rab()
    {
        printf("Inside CRaboof::Rab()\n");
    }

    static void TestRaboof()
    {
        AZStd::shared_ptr<CRaboof> pFoo0_ = CRaboof::CreateClassInstance();
        IRaboofPtr pFoo0 = cryinterface_cast<IRaboof>(pFoo0_);
        ICryUnknownPtr p0 = cryinterface_cast<ICryUnknown>(pFoo0);

        CryInterfaceID iid = cryiidof<IRaboof>();
        CryClassID clsid = p0->GetFactory()->GetClassID();

        AZStd::shared_ptr<CRaboof> pFoo1 = CRaboof::CreateClassInstance();

        pFoo0->Rab();
        pFoo1->Rab();
    }

    //////////////////////////////////////////////////////////////////////////

    class CAB
        : public IA
        , public IB
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IA)
        CRYINTERFACE_ADD(IB)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CAB, "AB", 0xb9e54711a64448c0, 0xa4819b4ed3024d04)

    public:
        virtual void A();
        virtual void B();

    private:
        int i;
    };

    CRYREGISTER_CLASS(CAB)

    CAB::CAB()
    {
        i = 0x12345678;
    }

    CAB::~CAB()
    {
    }

    void CAB::A()
    {
        printf("Inside CAB::A()\n");
    }

    void CAB::B()
    {
        printf("Inside CAB::B()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    class CABC
        : public CAB
        , public IC
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IC)
        CRYINTERFACE_ENDWITHBASE(CAB)

        CRYGENERATE_CLASS(CABC, "ABC", 0x4e61feae11854be7, 0xa16157c5f8baadd9)

    public:
        virtual void C();

    private:
        int a;
    };

    CRYREGISTER_CLASS(CABC)

    CABC::CABC()
    //: CAB()
    {
        a = 0x87654321;
    }

    CABC::~CABC()
    {
    }

    void CABC::C()
    {
        printf("Inside CABC::C()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    class CCustomC
        : public ICustomC
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IC)
        CRYINTERFACE_ADD(ICustomC)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CCustomC, "CustomC", 0xee61760b98a44b71, 0xa05e7372b44bd0fd)

    public:
        virtual void C();
        virtual void C1();

    private:
        int a;
    };

    CRYREGISTER_CLASS(CCustomC)

    CCustomC::CCustomC()
    {
        a = 0x87654321;
    }

    CCustomC::~CCustomC()
    {
    }

    void CCustomC::C()
    {
        printf("Inside CCustomC::C()\n");
    }

    void CCustomC::C1()
    {
        printf("Inside CCustomC::C1()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    class CMultiBase
        : public CAB
        , public CCustomC
    {
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ENDWITHBASE2(CAB, CCustomC)

        CRYGENERATE_CLASS(CMultiBase, "MultiBase", 0x75966b8f98644d42, 0x8fbdd489e94cc29e)

    public:
        virtual void A();
        virtual void C1();

        int i;
    };

    CRYREGISTER_CLASS(CMultiBase)

    CMultiBase::CMultiBase()
    {
        i = 0x87654321;
    }

    CMultiBase::~CMultiBase()
    {
    }

    void CMultiBase::C1()
    {
        printf("Inside CMultiBase::C1()\n");
    }

    void CMultiBase::A()
    {
        printf("Inside CMultiBase::A()\n");
    }

    //////////////////////////////////////////////////////////////////////////

    static void TestComplex()
    {
        {
            ICPtr p;
            if (CryCreateClassInstance(MAKE_CRYGUID(0x75966b8f98644d42, 0x8fbdd489e94cc29e), p))
            {
                p->C();
            }
        }

        {
            ICustomCPtr p;
            if (CryCreateClassInstance("MultiBase", p))
            {
                p->C();
            }
        }

        {
            IFoobarPtr p;
            if (CryCreateClassInstance(MAKE_CRYGUID(0x75966b8f98644d42, 0x8fbdd489e94cc29e), p))
            {
                p->Foo();
            }
        }

        {
            AZStd::shared_ptr<CMultiBase> p = CMultiBase::CreateClassInstance();
            AZStd::shared_ptr<const CMultiBase> pc = p;

            {
                ICryUnknownPtr pUnk = cryinterface_cast<ICryUnknown>(p);
                ICryUnknownConstPtr pCUnk0 = cryinterface_cast<const ICryUnknown>(p);
                ICryUnknownConstPtr pCUnk1 = cryinterface_cast<const ICryUnknown>(pc);
                //ICryUnknownPtr pUnkF = cryinterface_cast<ICryUnknown>(pc); // must fail to compile due to const rules

                ICryFactory* pF = pUnk->GetFactory();

                int t = 0;
            }

            ICPtr pC = cryinterface_cast<IC>(p);
            ICustomCPtr pCC = cryinterface_cast<ICustomC>(pC);

            p->C();
            p->C1();

            pC->C();
            pCC->C1();

            IAPtr pA = cryinterface_cast<IA>(p);
            pA->A();
            p->A();
        }

        {
            AZStd::shared_ptr<CCustomC> p = CCustomC::CreateClassInstance();

            ICPtr pC = cryinterface_cast<IC>(p);
            ICustomCPtr pCC = cryinterface_cast<ICustomC>(pC);

            p->C();
            p->C1();

            pC->C();
            pCC->C1();
        }
        {
            CryInterfaceID ia = cryiidof<IA>();
            CryInterfaceID ib = cryiidof<IB>();
            CryInterfaceID ic = cryiidof<IC>();
            CryInterfaceID ico = cryiidof<ICryUnknown>();
        }

        {
            AZStd::shared_ptr<CAB> p = CAB::CreateClassInstance();
            CryClassID clsid = p->GetFactory()->GetClassID();

            IAPtr pA = cryinterface_cast<IA>(p);
            IBPtr pB = cryinterface_cast<IB>(p);

            IBPtr pB1 = cryinterface_cast<IB>(pA);
            IAPtr pA1 = cryinterface_cast<IA>(pB);

            pA->A();
            pB->B();

            ICryUnknownPtr p1 = cryinterface_cast<ICryUnknown>(pA);
            ICryUnknownPtr p2 = cryinterface_cast<ICryUnknown>(pB);
            const ICryUnknown* p3 = cryinterface_cast<const ICryUnknown>(pB.get());

            int t = 0;
        }

        {
            AZStd::shared_ptr<CABC> pABC = CABC::CreateClassInstance();
            CryClassID clsid = pABC->GetFactory()->GetClassID();

            ICryFactory* pFac = pABC->GetFactory();
            pFac->ClassSupports(cryiidof<IA>());
            pFac->ClassSupports(cryiidof<IRaboof>());

            IAPtr pABC0 = cryinterface_cast<IA>(pABC);
            IBPtr pABC1 = cryinterface_cast<IB>(pABC0);
            ICPtr pABC2 = cryinterface_cast<IC>(pABC1);

            pABC2->C();
            pABC1->B();

            pABC2->GetFactory();

            const IC* pCconst = pABC2.get();
            const ICryUnknown* pOconst = cryinterface_cast<const ICryUnknown>(pCconst);
            const IA* pAconst = cryinterface_cast<const IA>(pOconst);
            const IB* pBconst = cryinterface_cast<const IB>(pAconst);

            //const IA* pA11 = cryinterface_cast<IA>(pOconst);

            pCconst = cryinterface_cast<const IC>(pBconst);

            IC* pC = static_cast<IC*>(static_cast<void*>(pABC1.get()));
            pC->C(); // calls IB::B()

            int t = 0;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // use of extension system without any of the helper macros/templates

    class CDontLikeMacrosFactory
        : public ICryFactory
    {
        // ICryFactory
    public:
        virtual const char* GetClassName() const
        {
            return "DontLikeMacros";
        }
        virtual const CryClassID& GetClassID() const
        {
            static const CryClassID cid = {0x73c3ab0042e6488aull, 0x89ca1a3763365565ull};
            return cid;
        }
        virtual bool ClassSupports(const CryInterfaceID& iid) const
        {
            return iid == cryiidof<ICryUnknown>() || iid == cryiidof<IDontLikeMacros>();
        }
        virtual void ClassSupports(const CryInterfaceID*& pIIDs, size_t& numIIDs) const
        {
            static const CryInterfaceID iids[2] = {cryiidof<ICryUnknown>(), cryiidof<IDontLikeMacros>()};
            pIIDs = iids;
            numIIDs = 2;
        }
        virtual ICryUnknownPtr CreateClassInstance() const;

    public:
        static CDontLikeMacrosFactory& Access()
        {
            return s_factory;
        }

    private:
        CDontLikeMacrosFactory() {}
        ~CDontLikeMacrosFactory() {}

    private:
        static CDontLikeMacrosFactory s_factory;
    };

    CDontLikeMacrosFactory CDontLikeMacrosFactory::s_factory;

    class CDontLikeMacros
        : public IDontLikeMacros
    {
        // ICryUnknown
    public:
        virtual ICryFactory* GetFactory() const
        {
            return &CDontLikeMacrosFactory::Access();
        };

        // only needed to be able to create initial shared_ptr<CDontLikeMacros> so we don't lose type info for debugging (i.e. inspecting shared_ptr<>)
        template <class T>
        friend void AZStd::Internal::sp_ms_deleter<T>::destroy();
        template <class T>
        friend AZStd::shared_ptr<T> AZStd::make_shared<T>();

    protected:
        virtual void* QueryInterface(const CryInterfaceID& iid) const
        {
            if (iid == cryiidof<ICryUnknown>())
            {
                return (void*) (ICryUnknown*) this;
            }
            else if (iid == cryiidof<IDontLikeMacros>())
            {
                return (void*) (IDontLikeMacros*) this;
            }
            else
            {
                return 0;
            }
        }

        virtual void* QueryComposite(const char*) const
        {
            return 0;
        }

        // IDontLikeMacros
    public:
        virtual void CallMe()
        {
            printf("Yey, no macros...\n");
        }

        CDontLikeMacros() {}

    protected:
        virtual ~CDontLikeMacros() {}
    };

    ICryUnknownPtr CDontLikeMacrosFactory::CreateClassInstance() const
    {
        AZStd::shared_ptr<CDontLikeMacros> p = AZStd::make_shared<CDontLikeMacros>();
        return ICryUnknownPtr(*static_cast<AZStd::shared_ptr<ICryUnknown>*>(static_cast<void*>(&p)));
    }

    static SRegFactoryNode g_dontLikeMacrosFactory(&CDontLikeMacrosFactory::Access());

    //////////////////////////////////////////////////////////////////////////

    static void TestDontLikeMacros()
    {
        ICryFactory* f = &CDontLikeMacrosFactory::Access();

        f->ClassSupports(cryiidof<ICryUnknown>());
        f->ClassSupports(cryiidof<IDontLikeMacros>());

        const CryInterfaceID* pIIDs = 0;
        size_t numIIDs = 0;
        f->ClassSupports(pIIDs, numIIDs);

        ICryUnknownPtr p = f->CreateClassInstance();
        IDontLikeMacrosPtr pp = cryinterface_cast<IDontLikeMacros>(p);

        ICryUnknownPtr pq = crycomposite_query(p, "blah");

        pp->CallMe();
    }
} // namespace TestExtension


//////////////////////////////////////////////////////////////////////////


void TestExtensions(ICryFactoryRegistryImpl* pReg)
{
    printf("Test extensions:\n");

    struct MyCallback
        : public ICryFactoryRegistryCallback
    {
        virtual void OnNotifyFactoryRegistered(ICryFactory* pFactory)
        {
            int test = 0;
        }
        virtual void OnNotifyFactoryUnregistered(ICryFactory* pFactory)
        {
            int test = 0;
        }
    };

    //pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x4);
    //pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x1);
    //pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x3);
    //pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x3);
    //pReg->RegisterCallback((ICryFactoryRegistryCallback*) 0x2);

    //pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x2);
    //pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x2);
    //pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x4);
    //pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x3);
    //pReg->UnregisterCallback((ICryFactoryRegistryCallback*) 0x1);

    //MyCallback callback0;
    //pReg->RegisterCallback(&callback0);
    //pReg->RegisterFactories(g_pHeadToRegFactories);

    //pReg->RegisterFactories(g_pHeadToRegFactories);
    //pReg->UnregisterFactories(g_pHeadToRegFactories);

    ICryFactory* pF[4];
    size_t numFactories = 4;
    pReg->IterateFactories(cryiidof<IA>(), pF, numFactories);
    pReg->IterateFactories(MAKE_CRYGUID(-1, -1), pF, numFactories);

    numFactories = (size_t) -1;
    pReg->IterateFactories(cryiidof<ICryUnknown>(), 0, numFactories);

    MyCallback callback1;
    pReg->RegisterCallback(&callback1);
    pReg->UnregisterCallback(&callback1);

    ICryFactory* p;
    p = pReg->GetFactory(MAKE_CRYGUID(0xee61760b98a44b71, 0xa05e7372b44bd0fd));
    p = pReg->GetFactory("CustomC");
    p = pReg->GetFactory("ABC");
    p = pReg->GetFactory((const char*)0);

    p = pReg->GetFactory("DontLikeMacros");
    p = pReg->GetFactory(MAKE_CRYGUID(0x73c3ab0042e6488a, 0x89ca1a3763365565));

    TestExtension::TestFoobar();
    TestExtension::TestRaboof();
    TestExtension::TestComplex();
    TestExtension::TestDontLikeMacros();

    TestComposition::TestComposition();
}

#endif // #ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES
