/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWNUMBER_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWNUMBER_H
#pragma once


#include "QPropertyTree.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/Decorators/Range.h"
#include "PropertyRowNumberField.h"

#include <limits.h>
#include <float.h>
#include <math.h>

template<class T>
string numberAsString(T value)
{
    Serialization::MemoryWriter buf;
    buf << value;
    return buf.c_str();
}

inline long long stringToSignedInteger(const char* str)
{
    long long value;
#ifdef _MSC_VER
    value = _atoi64(str);
#else
    char* endptr = (char*)str;
    value = strtoll(str, &endptr, 10);
#endif
    return value;
}

inline unsigned long long stringToUnsignedInteger(const char* str)
{
    unsigned long long value;
    if (*str == '-') {
        value = 0;
    }
    else {
#ifdef _MSC_VER
        char* endptr = (char*)str;
        value = _strtoui64(str, &endptr, 10);
#else
        char* endptr = (char*)str;
        value = strtoull(str, &endptr, 10);
#endif
    }
    return value;
}

template<class Output, class Input>
Output clamp(Input value, Output min, Output max)
{
    if (value < Input(min))
        return min;
    if (value > Input(max))
        return max;
    return Output(value);
}

template<class Out, class In> void clampToType(Out* out, In value) { *out = clamp(value, std::numeric_limits<Out>::lowest(), std::numeric_limits<Out>::max()); }

inline void clampedNumberFromString(char* value, const char* str)        { clampToType(value, stringToSignedInteger(str)); }
inline void clampedNumberFromString(signed char* value, const char* str) { clampToType(value, stringToSignedInteger(str)); }
inline void clampedNumberFromString(short* value, const char* str)         { clampToType(value, stringToSignedInteger(str)); }
inline void clampedNumberFromString(int* value, const char* str)         { clampToType(value, stringToSignedInteger(str)); }
inline void clampedNumberFromString(long* value, const char* str)         { clampToType(value, stringToSignedInteger(str)); }
inline void clampedNumberFromString(long long* value, const char* str)   { clampToType(value, stringToSignedInteger(str)); }
inline void clampedNumberFromString(unsigned char* value, const char* str)        { clampToType(value, stringToUnsignedInteger(str)); }
inline void clampedNumberFromString(unsigned short* value, const char* str)        { clampToType(value, stringToUnsignedInteger(str)); }
inline void clampedNumberFromString(unsigned int* value, const char* str)        { clampToType(value, stringToUnsignedInteger(str)); }
inline void clampedNumberFromString(unsigned long* value, const char* str)        { clampToType(value, stringToUnsignedInteger(str)); }
inline void clampedNumberFromString(unsigned long long* value, const char* str) { clampToType(value, stringToUnsignedInteger(str)); }
inline void clampedNumberFromString(float* value, const char* str)
{
    double v = atof(str);
    if (v > FLT_MAX)
        v = FLT_MAX;
    if (v < -FLT_MAX)
        v = -FLT_MAX;
    *value = float(v);
}

inline void clampedNumberFromString(double* value, const char* str)
{
    *value = atof(str);
}


template<class Type>
class PropertyRowNumber : public PropertyRowNumberField{
public:
    PropertyRowNumber()
    {
        softMin_ = std::numeric_limits<Type>::lowest();
        softMax_ = std::numeric_limits<Type>::max();
        hardMin_ = std::numeric_limits<Type>::lowest();
        hardMax_ = std::numeric_limits<Type>::max();
    }

    void setValue(Type value, const void* handle, const Serialization::TypeID& type)
    {
        value_ = value;
        serializer_.setPointer((void*)handle);
        serializer_.setType(type);
    }
    bool setValueFromString(const char* str) override{
        Type value = value_;
        clampedNumberFromString(&value_, str);
        return value_ != value;
    }
    string valueAsString() const override
    {
        return numberAsString(Type(value_));
    }

    bool assignToPrimitive(void* object, [[maybe_unused]] size_t size) const override
    {
        *reinterpret_cast<Type*>(object) = value_;
        return true;
    }

    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override
    {
        Serialization::RangeDecorator<Type>* range = (Serialization::RangeDecorator<Type>*)ser.pointer();
        serializer_.setPointer((void*)range->value);
        serializer_.setType(Serialization::TypeID::get<Type>());
        value_ = *range->value;
        softMin_ = range->softMin;
        softMax_ = range->softMax;
        hardMin_ = range->hardMin;
        hardMax_ = range->hardMax;
    }

  bool assignTo(const Serialization::SStruct& ser) const override 
    {
        if (ser.type() == Serialization::TypeID::get<Serialization::RangeDecorator<Type>>()) {
            Serialization::RangeDecorator<Type>* range = (Serialization::RangeDecorator<Type>*)ser.pointer();
            *range->value = value_;
        }
        else if (ser.type() == Serialization::TypeID::get<Type>()) {
            *(Type*)ser.pointer() = value_;
        }            
    return true;
    }

    void serializeValue(Serialization::IArchive& ar) 
    {
        ar(value_, "value", "Value");
        ar(softMin_, "softMin", "SoftMin");
        ar(softMax_, "softMax", "SoftMax");
        ar(hardMin_, "hardMin", "HardMin");
        ar(hardMax_, "hardMax", "HardMax");
    }

    void startIncrement() override
    {
        incrementStartValue_ = value_;
    }

    void endIncrement(QPropertyTree* tree) override
    {
        if (value_ != incrementStartValue_) {
            Type value = value_;
            value_ = incrementStartValue_;
            value_ = value;
            tree->model()->rowChanged(this, true);
        }
    }

    void incrementLog(float screenFraction, float valueFieldFraction)
    {
        bool bothSoftLimitsSet = (std::numeric_limits<Type>::lowest() == 0 || softMin_ != std::numeric_limits<Type>::lowest()) && softMax_ != std::numeric_limits<Type>::max();

        if (bothSoftLimitsSet)
        {
            Type softRange = softMax_ - softMin_;
            double newValue = incrementStartValue_ + softRange * valueFieldFraction;
            value_ = clamp(newValue, hardMin_, hardMax_);
        }
        else
        {
            double screenFractionMultiplier = 1000.0;
            if (Serialization::TypeID::get<Type>() == Serialization::TypeID::get<float>() || 
                  Serialization::TypeID::get<Type>() == Serialization::TypeID::get<double>()) 
                screenFractionMultiplier = 10.0;

            double startPower = log10(fabs(double(incrementStartValue_)) + 1.0) - 3.0;
            double power = startPower + fabs(screenFraction) * 10.0f;
            double delta = pow(10.0, power) - pow(10.0, startPower) + screenFractionMultiplier * fabs(screenFraction);
            double newValue;
            if (screenFraction > 0.0f)
                newValue = double(incrementStartValue_) + delta;
            else
                newValue = double(incrementStartValue_) - delta;
#ifdef _MSC_VER
            if (_isnan(newValue)) {
#else
            if (isnan(newValue)) {
#endif
                if (screenFraction > 0.0f)
                    newValue = DBL_MAX;
                else
                    newValue = -DBL_MAX;
            }
            value_ = clamp(newValue, hardMin_, hardMax_);
        }
    }

    double sliderPosition() const override
    {
        if ((softMin_ == std::numeric_limits<Type>::lowest() && softMax_ == std::numeric_limits<Type>::max() && softMax_ != Type(255)) || (softMin_ >= softMax_))
            return 0.0;
        return clamp(double(value_ - softMin_) / (softMax_ - softMin_), 0.0, 1.0);
    }
protected:
    Type incrementStartValue_; 
    Type value_; 
    Type softMin_;
    Type softMax_;
    Type hardMin_;
    Type hardMax_;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWNUMBER_H
