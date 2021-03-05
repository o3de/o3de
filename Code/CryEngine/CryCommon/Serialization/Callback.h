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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CALLBACK_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CALLBACK_H
#pragma once

#include <AzCore/std/functional.h>

namespace Serialization
{
    struct ICallback
    {
        virtual bool SerializeValue(IArchive& ar, const char* name, const char* value) = 0;
        virtual ICallback* Clone() = 0;
        virtual void Release() = 0;
        virtual TypeID Type() const = 0;

        typedef AZStd::function<void(void*, const TypeID&)> ApplyFunction;
        virtual void Call(const ApplyFunction&) = 0;
    };

    template<class T, class Decorator = T>
    struct CallbackSimple
        : ICallback
    {
        typedef AZStd::function<void(const T&)> CallbackFunction;
        T* value;
        T oldValue;
        CallbackFunction callback;

        CallbackSimple(T* value, const T& oldValue, const AZStd::function<void(const T&)>& callback)
            : value(value)
            , oldValue(oldValue)
            , callback(callback)
        {
        }

        ICallback* Clone() { return new CallbackSimple<T>(0, oldValue, callback);   }
        void Release() { delete this; }
        bool SerializeValue(IArchive& ar, const char* name, const char* label) { return ar(*value, name, label); }
        TypeID Type() const{ return TypeID::get<T>(); }

        void Call(const ApplyFunction& applyFunction)
        {
            T newValue;
            applyFunction((void*)&newValue, TypeID::get<T>());
            if (oldValue != newValue)
            {
                callback(newValue);
                oldValue = newValue;
            }
        }
    };

    template<class T, class Decorator = T>
    struct CallbackWithDecorator
        : ICallback
    {
        typedef AZStd::function<void(const T&)> CallbackFunction;
        typedef AZStd::function<Decorator (T&)> DecoratorFunction;

        T oldValue;
        T* value;
        CallbackFunction callback;
        DecoratorFunction decorator;

        CallbackWithDecorator(T* value,
            const T& oldValue,
            const CallbackFunction& callback,
            const DecoratorFunction& decorator)
            : value(value)
            , oldValue(oldValue)
            , callback(callback)
            , decorator(decorator)
        {
        }

        ICallback* Clone() { return new CallbackWithDecorator<T, Decorator>(0, oldValue, callback, decorator); }
        void Release() { delete this;   }
        bool SerializeValue(IArchive& ar, const char* name, const char* label) { return ar(decorator(*value), name, label); }
        TypeID Type() const{ return TypeID::get<Decorator>(); }

        void Call(const ApplyFunction& applyFunction)
        {
            T newValue;
            Decorator dec = decorator(newValue);
            applyFunction((void*)&dec, TypeID::get<Decorator>());
            if (oldValue != newValue)
            {
                callback(newValue);
                oldValue = newValue;
            }
        }
    };



    namespace Detail
    {
        template <typename T>
        struct MethodReturnType
        {
            typedef void type;
        };

        template <typename ClassType, typename ReturnType, typename Arg0>
        struct MethodReturnType<ReturnType(ClassType::*)(Arg0) const>
        {
            typedef ReturnType type;
        };

        template<class T>
        struct OperatorBracketsReturnType
        {
            typedef typename MethodReturnType<decltype(& T::operator())>::type Type;
        };
    }

    template<class T, class CallbackFunc>
    CallbackSimple<T>
    Callback(T& value, const CallbackFunc& callback)
    {
        return CallbackSimple<T>(&value, value, AZStd::function<void(const T&)>(callback));
    }


    template<class T, class CallbackFunc, class DecoratorFunc>
    CallbackWithDecorator<T, typename Detail::OperatorBracketsReturnType<DecoratorFunc>::Type>
    Callback(T& value, const CallbackFunc& callback, const DecoratorFunc& decorator)
    {
        typedef typename Detail::OperatorBracketsReturnType<DecoratorFunc>::Type Decorator;
        return CallbackWithDecorator<T, Decorator>(&value, value,
            AZStd::function<void(const T&)>(callback),
            AZStd::function<Decorator(T&)>(decorator));
    }


    template<class T>
    bool Serialize(IArchive& ar, CallbackSimple<T>& callback, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(static_cast<ICallback&>(callback), name, label);
        }
        else
        {
            if (!ar(*callback.value, name, label))
            {
                return false;
            }
            return true;
        }
    }

    template<class T, class Decorator>
    bool Serialize(IArchive& ar, CallbackWithDecorator<T, Decorator>& callback, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(static_cast<ICallback&>(callback), name, label);
        }
        else
        {
            if (!ar(*callback.value, name, label))
            {
                return false;
            }
            return true;
        }
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CALLBACK_H
