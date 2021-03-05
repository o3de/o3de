/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 *
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_FACTORY_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_FACTORY_H
#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/functional.h>
#include "Serialization/Assert.h"

template<class _Key, class _Product, class _KeyPred = std::less<_Key>>
class Factory {
public:
    typedef AZStd::map<_Key, AZStd::function<_Product *()>, _KeyPred, AZ::StdLegacyAllocator> Creators;
    typedef _Product* (*ProductConstructionFunction)(void);

    Factory() {}

    struct Creator
    {
        Creator()
        {
            if (s_creatorsHead)
            {
                m_next = s_creatorsHead;
            }
            s_creatorsHead = this;
        }

        Creator(Factory& factory, _Key key, ProductConstructionFunction construction_function_)
            : Creator()
        {
            // capture key and construction_function_ by value so the lazy load won't try to access possibly deleted data
            Register = [&, key, construction_function_]()
            {
                factory.add(key, construction_function_);
            };
        }

        Creator(_Key key, ProductConstructionFunction construction_function_)
            : Creator()
        {
            // capture key and construction_function_ by value so the lazy load won't try to access possibly deleted data
            Register = [&, key, construction_function_]()
            {
                Factory::the().add(key, construction_function_);
            };
        }

        AZStd::function<void()> Register;
        Creator* m_next = nullptr;
    };

    void add(const _Key& key, AZStd::function<_Product *()> creator) {
        YASLI_ASSERT(creators_.find(key) == creators_.end());
        YASLI_ASSERT(creator);
        creators_[key] = creator;
    }

    void remove(const _Key& key) {
        auto& entry = creators_.find(key);
        if (entry != creators_.end()) {
            creators_.erase(entry);
        }
    }

    _Product* create(const _Key& key) const 
    {
        lazyRegisterCreators();
        typename Creators::const_iterator it = creators_.find(key);
        if (it != creators_.end()) {
            return it->second();
        }
        else
            return 0;
    }

    std::size_t size() const 
    {
        lazyRegisterCreators();
        return creators_.size(); 
    }

    _Product* createByIndex(int index) const 
    {
        lazyRegisterCreators();
        YASLI_ASSERT(index >= 0 && index < creators_.size());
        typename Creators::const_iterator it = creators_.begin();
        std::advance(it, index);
        return it->second();
    }


    const Creators& creators() const 
    {
        lazyRegisterCreators(); 
        return creators_;
    }

    static Factory& the()
    {
        static Factory* genericFactory = nullptr;
        static AZStd::aligned_storage_for_t<Factory> s_storage;
        if (!genericFactory)
        {
            genericFactory = new(&s_storage) Factory();
        }
        return *genericFactory;
    }

private:
    void lazyRegisterCreators() const
    {
        if (s_creatorsHead)
        {
            Creator* creator = s_creatorsHead;
            while (creator)
            {
                creator->Register();
                creator = creator->m_next;
            }
            s_creatorsHead = nullptr;
        }
    }

protected:
    Creators creators_;

    static Creator* s_creatorsHead;
};

template <class _Key, class _Product, class _KeyPred>
typename Factory<_Key, _Product, _KeyPred>::Creator* Factory<_Key, _Product, _KeyPred>::s_creatorsHead = nullptr;

#define REGISTER_IN_FACTORY(factory, key, product, construction_function)                  \
    static factory::Creator factory##product##Creator(key, construction_function);

#define REGISTER_IN_FACTORY_INSTANCE(factory, factoryType, key, product)              \
    static factoryType::Creator<product> factoryType##product##Creator(factory, key);

#define DECLARE_SEGMENT(fileName) int dataSegment##fileName;

#define FORCE_SEGMENT(fileName) \
    extern int dataSegment##fileName; \
    int* dataSegmentPtr##fileName = &dataSegment##fileName;


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_FACTORY_H
