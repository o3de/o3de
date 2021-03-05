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

// Description : Interface to manage SoftCode module loading and patching


#ifndef CRYINCLUDE_CRYCOMMON_ISOFTCODEMGR_H
#define CRYINCLUDE_CRYCOMMON_ISOFTCODEMGR_H
#pragma once


// Provides the generic interface for exchanging member values between SoftCode modules,
struct IExchangeValue
{
    // <interfuscator:shuffle>
    virtual ~IExchangeValue() {}

    // Allocates a new IExchangeValue with the underlying type
    virtual IExchangeValue* Clone() const = 0;
    // Returns the size of the underlying type (to check compatibility)
    virtual size_t GetSizeOf() const = 0;
    // </interfuscator:shuffle>
};

template <typename T>
struct ExchangeValue
    : public IExchangeValue
{
    ExchangeValue(T& value)
        : m_value(value)
    {}

    virtual IExchangeValue* Clone() const { return new ExchangeValue(*this); }
    virtual size_t GetSizeOf() const { return sizeof(m_value); }

    T m_value;
};

template <typename T, size_t S>
struct ExchangeArray
    : public IExchangeValue
{
    ExchangeArray(T* pArr)
    {
        for (size_t i = 0; i < S; ++i)
        {
            m_array[i] = pArr[i];
        }
    }

    virtual IExchangeValue* Clone() const { return new ExchangeArray(*this); }
    virtual size_t GetSizeOf() const { return sizeof(m_array); }

    T m_array[S];
};

/*
    This is a non-intrusive support function for types where default construction does no initialization.
    SoftCoding relies on default construction to initialize object state correctly.
    For most types this works as expected but for some types (typically things like vectors or matrices)
    default initialization would be too costly and is therefore not implemented.
    This function allows a specialized implementation to be used for such types that will perform
    initialization on the newly constructed instance. For example:

    inline void DefaultInitialize(Matrix34& matrix)
    {
        matrix.SetIdentity();
    }
*/
template <typename T>
void DefaultInitialize(T& t)
{
    t = T();
}

// Vector support
template<class F>
struct Vec2_tpl;
template<typename T>
struct Vec3_tpl;
template <class F>
void DefaultInitialize(Vec2_tpl<F>& vec) { vec.zero(); }
template <typename T>
void DefaultInitialize(Vec3_tpl<T>& vec) { vec.zero(); }

// Matrix support
template<typename F>
struct Matrix33_tpl;
template<typename F>
struct Matrix34_tpl;
template<typename F>
struct Matrix44_tpl;

template <typename F>
void DefaultInitialize(Matrix33_tpl<F>& matrix) { matrix.SetIdentity(); }
template <typename F>
void DefaultInitialize(Matrix34_tpl<F>& matrix) { matrix.SetIdentity(); }
template <typename F>
void DefaultInitialize(Matrix44_tpl<F>& matrix) { matrix.SetIdentity(); }

// Quat support
template <typename F>
struct Quat_tpl;
template <typename F>
void DefaultInitialize(Quat_tpl<F>& quat) { quat.SetIdentity(); }

// Interface for performing an exchange of instance data
struct IExchanger
{
    // <interfuscator:shuffle>
    virtual ~IExchanger() {}

    // True if data is being read from instance members
    virtual bool IsLoading() const = 0;

    virtual size_t InstanceCount() const = 0;

    virtual bool BeginInstance(void* pInstance) = 0;
    virtual bool SetValue(const char* name, IExchangeValue& value) = 0;
    virtual IExchangeValue* GetValue(const char* name, void* pTarget, size_t targetSize) = 0;
    // </interfuscator:shuffle>

    template <typename T>
    void Visit(const char* name, T& instance);

    template <typename T, size_t S>
    void Visit(const char* name, T (&arr)[S]);
};

template <typename T>
void IExchanger::Visit(const char* name, T& value)
{
    if (IsLoading())
    {
        IExchangeValue* pValue = GetValue(name, &value, sizeof(value));
        if (pValue)
        {
            ExchangeValue<T>* pTypedValue = static_cast<ExchangeValue<T>*>(pValue);
            value = pTypedValue->m_value;
        }
    }
    else    // Saving
    {
        // If this member is stored
        if (SetValue(name, ExchangeValue<T>(value)))
        {
            // Set the original value to the default state (to allow safe destruction)
            DefaultInitialize(value);
        }
    }
}

template <typename T, size_t S>
void IExchanger::Visit(const char* name, T (&arr)[S])
{
    if (IsLoading())
    {
        IExchangeValue* pValue = GetValue(name, &arr, sizeof(arr));
        if (pValue)
        {
            ExchangeArray<T, S>* pTypedArray = static_cast<ExchangeArray<T, S>*>(pValue);
            // TODO: Accommodate array resizing? Complex however...
            for (size_t i = 0; i < S; ++i)
            {
                arr[i] = pTypedArray->m_array[i];
            }
        }
    }
    else    // Saving
    {
        // If this member is stored
        if (SetValue(name, ExchangeArray<T, S>(arr)))
        {
            T defaultValue;
            DefaultInitialize(defaultValue);

            // Set the original value to the default value (to allow safe destruction)
            for (size_t i = 0; i < S; ++i)
            {
                arr[i] = defaultValue;
            }
        }
    }
}

struct InstanceTracker;

struct ITypeRegistrar
{
    // <interfuscator:shuffle>
    virtual ~ITypeRegistrar() {}

    virtual const char* GetName() const = 0;

    // Creates an instance of the type
    virtual void* CreateInstance() = 0;
    // </interfuscator:shuffle>

#ifdef SOFTCODE_ENABLED
    // How many active instances exist of this type?
    virtual size_t InstanceCount() const = 0;
    // Used to remove a tracked instance from the Registrar
    virtual void RemoveInstance(InstanceTracker* pTracker) = 0;
    // Exchanges the instance state with the given exchanger data set
    virtual bool ExchangeInstances(IExchanger& exchanger) = 0;
    // Destroys all tracked instances of this type
    virtual bool DestroyInstances() = 0;
    // Returns true if pInstance is of this type (linear search)
    virtual bool HasInstance(void* pInstance) const = 0;
#endif
};

struct ITypeLibrary
{
    // <interfuscator:shuffle>
    virtual ~ITypeLibrary() {}

    virtual const char* GetName() = 0;
    virtual void* CreateInstanceVoid(const char* typeName) = 0;
    // </interfuscator:shuffle>

#ifdef SOFTCODE_ENABLED
    virtual void SetOverride(ITypeLibrary* pOverrideLib) = 0;

    // Fills in the supplied type list if large enough, and sets count to number of types
    virtual size_t GetTypes(ITypeRegistrar** ppRegistrar, size_t& count) const = 0;
#endif
};

struct ISoftCodeListener
{
    // <interfuscator:shuffle>
    virtual ~ISoftCodeListener() {}

    // Called when an instance is replaced to allow managing systems to fixup pointers
    virtual void InstanceReplaced(void* pOldInstance, void* pNewInstance) = 0;
    // </interfuscator:shuffle>
};

/// Interface for ...
struct ISoftCodeMgr
{
    // <interfuscator:shuffle>
    virtual ~ISoftCodeMgr() {}

    // Used to register built-in libraries on first use
    virtual void RegisterLibrary(ITypeLibrary* pLib) = 0;

    // Loads any new SoftCode modules
    virtual void LoadNewModules() = 0;

    virtual void AddListener(const char* libraryName, ISoftCodeListener* pListener, const char* listenerName) = 0;
    virtual void RemoveListener(const char* libraryName, ISoftCodeListener* pListener) = 0;

    // To be called regularly to poll for library updates
    virtual void PollForNewModules() = 0;

    // Stops thread execution until a new SoftCode instance is available
    virtual void* WaitForUpdate(void* pInstance) = 0;

    /// Frees this instance from memory
    //virtual void Release() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ISOFTCODEMGR_H
