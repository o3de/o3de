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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORY_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORY_H
#pragma once

#include <map>
#include <vector>

#include "Serialization/Assert.h"
#include "Serialization/IClassFactory.h"
#include "Serialization/TypeID.h"

namespace Serialization {
    class IArchive;

    class ClassFactoryManager
    {
    public:
        static ClassFactoryManager& the()
        {
            static ClassFactoryManager factoryManager;
            return factoryManager;
        }

        const IClassFactory* find(TypeID baseType) const
        {
            lazyRegisterFactories();
            Factories::const_iterator it = factories_.find(baseType);
            if (it == factories_.end())
            {
                return 0;
            }
            else
            {
                return it->second;
            }
        }

        void registerFactory([[maybe_unused]] TypeID type, IClassFactory* factory)
        {
            factory->m_next = m_head;
            m_head = factory;
        }
    protected:
        void lazyRegisterFactories() const
        {
            if (m_head)
            {
                IClassFactory* factory = m_head;
                while (factory)
                {
                    const_cast<ClassFactoryManager*>(this)->factories_[factory->baseType_] = factory;
                    factory = factory->m_next;
                }
                const_cast<ClassFactoryManager*>(this)->m_head = nullptr;
            }
        }

        typedef AZStd::unordered_map<TypeID, const IClassFactory*, AZStd::hash<TypeID>, AZStd::equal_to<TypeID>, AZ::StdLegacyAllocator> Factories;
        Factories factories_;
        IClassFactory* m_head = nullptr;
    };

    template<class BaseType>
    class ClassFactory
        : public IClassFactory
    {
    public:
        static ClassFactory& the()
        {
            static AZStd::aligned_storage_for_t<ClassFactory> storage;
            if (s_instance != (decltype(s_instance))&storage)
            {
                s_instance = new(&storage) ClassFactory();
            }
            return *s_instance;
        }

        static void destroy()
        {
            if (s_instance)
            {
                s_instance->~ClassFactory();
                s_instance = nullptr;
            }
        }

        class CreatorBase
        {
        public:
            virtual ~CreatorBase() {}
            virtual BaseType* create() const = 0;
            virtual const TypeDescription& description() const{ return *description_; }
            virtual void* vptr() const { return vptr_; }
            virtual TypeID typeID() const = 0;
        protected:
            const TypeDescription* description_ = nullptr;
            void* vptr_ = nullptr;
        public:
            CreatorBase* next;
        };

        static void* extractVPtr(BaseType* ptr)
        {
            return *((void**)ptr);
        }

        template<class Derived>
        struct Annotation
        {
            Annotation(IClassFactory* factory, const char* name, const char* value) { static_cast<ClassFactory<BaseType>*>(factory)->addAnnotation<Derived>(name, value); }
        };

        template<class Derived>
        class Creator
            : public CreatorBase
        {
        public:
            Creator(const TypeDescription* description, ClassFactory* factory = nullptr)
            {
                this->description_ = description;

                if (!factory)
                {
                    factory = &ClassFactory::the();
                }

                factory->registerCreator(this);
            }

            void* vptr() const override
            {
                if (!this->vptr_)
                {
                    Derived vptrProbe;
                    const_cast<Creator*>(this)->vptr_ = extractVPtr(&vptrProbe);
                }
                return this->vptr_;
            }

            BaseType* create() const override { return new Derived(); }
            TypeID typeID() const override { return Serialization::TypeID::get<Derived>(); }
        };

        ClassFactory()
            : IClassFactory(TypeID::get<BaseType>())
        {
            ClassFactoryManager::the().registerFactory(baseType_, this);
        }

        ~ClassFactory()
        {
            m_data->~Data();
            m_data = nullptr;
        }

        typedef AZStd::unordered_map<string, const CreatorBase*, AZStd::hash<string>, AZStd::equal_to<string>, AZ::StdLegacyAllocator> TypeToCreatorMap;
        typedef AZStd::unordered_map<void*, CreatorBase*, AZStd::hash<void*>, AZStd::equal_to<void*>, AZ::StdLegacyAllocator> VPtrToCreatorMap;
        typedef AZStd::unordered_map<string, TypeID, AZStd::hash<string>, AZStd::equal_to<string>, AZ::StdLegacyAllocator> RegisteredNameToTypeID;
        typedef AZStd::unordered_map<TypeID, std::vector<std::pair<const char*, const char*> >, AZStd::hash<TypeID>, AZStd::equal_to<TypeID>, AZ::StdLegacyAllocator> AnnotationMap;

        virtual BaseType* create(const char* registeredName) const
        {
            lazyRegisterCreators();
            if (!registeredName)
            {
                return 0;
            }
            if (registeredName[0] == '\0')
            {
                return 0;
            }
            typename TypeToCreatorMap::const_iterator it = m_data->typeToCreatorMap_.find(registeredName);
            if (it != m_data->typeToCreatorMap_.end())
            {
                return it->second->create();
            }
            else
            {
                return 0;
            }
        }

        virtual const char* getRegisteredTypeName(BaseType* ptr) const
        {
            lazyRegisterCreators();
            if (ptr == 0)
            {
                return "";
            }
            void* vptr = extractVPtr(ptr);
            typename VPtrToCreatorMap::const_iterator it = m_data->vptrToCreatorMap_.find(vptr);
            if (it == m_data->vptrToCreatorMap_.end())
            {
                return "";
            }
            return it->second->description().name();
        }

        BaseType* createByIndex(int index) const
        {
            lazyRegisterCreators();
            YASLI_ASSERT(size_t(index) < m_data->creators_.size());
            return m_data->creators_[index]->create();
        }

        void serializeNewByIndex(IArchive& ar, int index, const char* name, const char* label)
        {
            lazyRegisterCreators();
            YASLI_ESCAPE(size_t(index) < m_data->creators_.size(), return );
            BaseType* ptr = m_data->creators_[index]->create();
            ar(*ptr, name, label);
            delete ptr;
        }
        // from ClassFactoryInterface:
        size_t size() const{ return m_data->creators_.size(); }
        const TypeDescription* descriptionByIndex(int index) const override
        {
            lazyRegisterCreators();
            if (size_t(index) >= int(m_data->creators_.size()))
            {
                return 0;
            }
            return &m_data->creators_[index]->description();
        }

        const TypeDescription* descriptionByRegisteredName(const char* name) const override
        {
            lazyRegisterCreators();
            const size_t numCreators = m_data->creators_.size();
            for (size_t i = 0; i < numCreators; ++i)
            {
                if (strcmp(m_data->creators_[i]->description().name(), name) == 0)
                {
                    return &m_data->creators_[i]->description();
                }
            }
            return 0;
        }
        // ^^^

        TypeID typeIDByRegisteredName(const char* registeredTypeName) const
        {
            lazyRegisterCreators();
            RegisteredNameToTypeID::const_iterator it = m_data->registeredNameToTypeID_.find(registeredTypeName);
            if (it == m_data->registeredNameToTypeID_.end())
            {
                return TypeID();
            }
            return it->second;
        }

        const char* findAnnotation(const char* registeredTypeName, const char* name) const
        {
            lazyRegisterCreators();
            TypeID typeID = typeIDByRegisteredName(registeredTypeName);
            AnnotationMap::const_iterator it = m_data->annotations_.find(typeID);
            if (it == m_data->annotations_.end())
            {
                return "";
            }
            for (size_t i = 0; i < it->second.size(); ++i)
            {
                if (strcmp(it->second[i].first, name) == 0)
                {
                    return it->second[i].second;
                }
            }
            return "";
        }
        void unregisterCreator(const TypeDescription& typeDescription)
        {
            auto creator = m_data->typeToCreatorMap_.find(typeDescription.name());
            if (creator != m_data->typeToCreatorMap_.end())
            {
                m_data->creators_.erase(std::find(m_data->creators_.begin(), m_data->creators_.end(), m_data->creator->second));
                m_data->vptrToCreatorMap_.erase(m_data->vptrToCreatorMap_.find(creator->second->vptr()));
                m_data->typeToCreatorMap_.erase(creator);
            }
        }

    protected:
        virtual void registerCreator(CreatorBase* creator)
        {
            creator->next = creatorsList;
            creatorsList = creator;
        }

        void lazyRegisterCreators() const
        {
            if (!m_data)
            {
                const_cast<ClassFactory*>(this)->m_data = ::new((void*)&m_dataStorage) Data();
                for (CreatorBase* creator = creatorsList; creator; creator = creator->next)
                {
                    if (!const_cast<ClassFactory*>(this)->m_data->typeToCreatorMap_.insert(AZStd::make_pair(creator->description().name(), creator)).second)
                    {
                        YASLI_ASSERT(0 && "Type registered twice in the same factory. Was SERIALIZATION_CLASS_NAME put into header file by mistake?");
                    }
                    const_cast<ClassFactory*>(this)->m_data->creators_.push_back(creator);
                    const_cast<ClassFactory*>(this)->m_data->registeredNameToTypeID_[creator->description().name()] = creator->typeID();
                    const_cast<ClassFactory*>(this)->m_data->vptrToCreatorMap_[creator->vptr()] = creator;
                }
            }
        }

        template<class T>
        void addAnnotation(const char* name, const char* value)
        {
            addAnnotation(Serialization::TypeID::get<T>(), name, value);
        }

        virtual void addAnnotation(const Serialization::TypeID& id, const char* name, const char* value)
        {
            lazyRegisterCreators();
            m_data->annotations_[id].push_back(std::make_pair(name, value));
        }

        CreatorBase* creatorsList = nullptr;
        static ClassFactory* s_instance;

        struct Data
        {
            TypeToCreatorMap typeToCreatorMap_;
            AZStd::vector<CreatorBase*, AZ::StdLegacyAllocator> creators_;
            VPtrToCreatorMap vptrToCreatorMap_;
            RegisteredNameToTypeID registeredNameToTypeID_;
            AnnotationMap annotations_;
        };
        Data* m_data = nullptr;
        AZStd::aligned_storage_for_t<Data> m_dataStorage;
    };

    template <class T>
    ClassFactory<T>* ClassFactory<T>::s_instance = nullptr;
}

#define SERIALIZATION_CLASS_NULL(BaseType, name)                                                          \
    namespace {                                                                                           \
        bool BaseType##_NullRegistered = Serialization::ClassFactory<BaseType>::the().setNullLabel(name); \
    }

#define SERIALIZATION_CLASS_NAME(BaseType, Type, name, label)                                                                   \
    static const Serialization::TypeDescription Type##BaseType##_DerivedDescription(name, label);                               \
    static Serialization::ClassFactory<BaseType>::Creator<Type> Type##BaseType##_Creator(&Type##BaseType##_DerivedDescription); \
    int dummyForType_##Type##BaseType;

#define SERIALIZATION_CLASS_NAME_FOR_FACTORY(Factory, BaseType, Type, name, label)                \
    static const Serialization::TypeDescription Type##BaseType##_DerivedDescription(name, label); \
    static Serialization::ClassFactory<BaseType>::Creator<Type> Type##BaseType##_Creator(&Type##BaseType##_DerivedDescription, &(Factory));

#define SERIALIZATION_CLASS_ANNOTATION(BaseType, Type, attributeName, attributeValue) \
    static Serialization::ClassFactory<BaseType>::Annotation<Type> Type##BaseType##_Annotation(&Serialization::ClassFactory<BaseType>::the(), attributeName, attributeValue);

#define SERIALIZATION_CLASS_ANNOTATION_FOR_FACTORY(factory, BaseType, Type, attributeName, attributeValue) \
    static Serialization::ClassFactory<BaseType>::Annotation<Type> Type##BaseType##_Annotation(&factory, attributeName, attributeValue);

#define SERIALIZATION_FORCE_CLASS(BaseType, Type) \
    extern int dummyForType_##Type##BaseType;     \
    int* dummyForTypePtr_##Type##BaseType = &dummyForType_##Type##BaseType + 1;

#include "ClassFactoryImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORY_H
