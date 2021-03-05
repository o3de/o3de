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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_ENUMIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_ENUMIMPL_H
#pragma once

#pragma once
#include "IArchive.h"
#include "STL.h"
#include "Enum.h"
#include "StringList.h"
#ifndef SERIALIZATION_STANDALONE
#include <ISystem.h>
#endif

namespace Serialization {
    inline void CEnumDescription::add(int value, const char* name, const char* label)
    {
        YASLI_ESCAPE(name && label, return );
        // Filter for dupes in case enum description included in a shared header
        NameToValue::iterator nameIt = nameToValue_.find(name);
        if (nameIt != nameToValue_.end() && nameIt->second == value)
        {            LabelToValue::iterator labelIt = labelToValue_.find(label);
            if (labelIt != labelToValue_.end() && labelIt->second == value)
            {
                return;
            }
        }
        nameToValue_[name] = value;
        labelToValue_[label] = value;
        valueToName_[value] = name;
        valueToLabel_[value] = label;
        valueToIndex_[value] = int(names_.size());
        names_.push_back(name);
        labels_.push_back(label);
        values_.push_back(value);
    }

    inline bool CEnumDescription::Serialize(IArchive& ar, int& value, const char* name, const char* label) const
    {
        lazyRegister();
        if (!ar.IsInPlace())
        {
            if (count() == 0)
            {
#ifdef SERIALIZATION_STANDALONE
                assert(0 && "Attempt to serialize enum type that is not registered with SERIALIZATION_ENUM macro");
#else
                CryFatalError("Attempt to serialize enum type that is not registered with SERIALIZATION_ENUM macro: %s", type().name());
#endif
                return false;
            }

            int index = StringListStatic::npos;
            if (ar.IsOutput())
            {
                index =  indexByValue(value);
            }
            StringListStaticValue stringListValue(ar.IsEdit() ? labels() : names(), index, &value, type());
            ar(stringListValue, name, label);
            if (ar.IsInput())
            {
                if (stringListValue.index() == StringListStatic::npos)
                {
                    return false;
                }
                value = ar.IsEdit() ? valueByLabel(stringListValue.c_str()) : this->value(stringListValue.c_str());
            }
            else if (index == StringListStatic::npos)
            {
                ar.Error(&value, type(), "Unregistered or uninitialized enumeration value.");
            }
        }
        else
        {
            return ar(value, name, label);
        }
        return true;
    }

    inline bool CEnumDescription::serializeBitVector(IArchive& ar, int& value, const char* name, const char* label) const
    {
        lazyRegister();
        if (ar.IsOutput())
        {
            StringListStatic names = nameCombination(value);
            string str;
            joinStringList(&str, names, '|');
            return ar(str, name, label);
        }
        else
        {
            string str;
            if (!ar(str, name, label))
            {
                return false;
            }
            StringList values;
            splitStringList(&values, str.c_str(), '|');
            StringList::iterator it;
            value = 0;
            for (it = values.begin(); it != values.end(); ++it)
            {
                if (!it->empty())
                {
                    value |= this->value(it->c_str());
                }
            }
            return true;
        }
    }


    inline const char* CEnumDescription::name(int value) const
    {
        lazyRegister();
        ValueToName::const_iterator it = valueToName_.find(value);
        YASLI_ESCAPE(it != valueToName_.end(), return "");
        return it->second;
    }
    inline const char* CEnumDescription::label(int value) const
    {
        lazyRegister();
        ValueToLabel::const_iterator it = valueToLabel_.find(value);
        YASLI_ESCAPE(it != valueToLabel_.end(), return "");
        return it->second;
    }

    inline StringListStatic CEnumDescription::nameCombination(int bitVector) const
    {
        lazyRegister();
        StringListStatic strings;
        for (ValueToName::const_iterator i = valueToName_.begin(); i != valueToName_.end(); ++i)
        {
            if ((bitVector & i->first) == i->first)
            {
                bitVector &= ~i->first;
                strings.push_back(i->second);
            }
        }
        YASLI_ASSERT(!bitVector && "Unregistered enum value");
        return strings;
    }

    inline StringListStatic CEnumDescription::labelCombination(int bitVector) const
    {
        lazyRegister();
        StringListStatic strings;
        for (ValueToLabel::const_iterator i = valueToLabel_.begin(); i != valueToLabel_.end(); ++i)
        {
            if (i->second && (bitVector & i->first) == i->first)
            {
                bitVector &= ~i->first;
                strings.push_back(i->second);
            }
        }
        YASLI_ASSERT(!bitVector && "Unregistered enum value");
        return strings;
    }


    inline int CEnumDescription::indexByValue(int value) const
    {
        lazyRegister();
        ValueToIndex::const_iterator it = valueToIndex_.find(value);
        if (it == valueToIndex_.end())
        {
            return -1;
        }
        else
        {
            return it->second;
        }
    }

    inline int CEnumDescription::valueByIndex(int index) const
    {
        lazyRegister();
        if (size_t(index) < values_.size())
        {
            return values_[index];
        }
        return 0;
    }

    inline const char* CEnumDescription::nameByIndex(int index) const
    {
        lazyRegister();
        if (size_t(index) < size_t(names_.size()))
        {
            return names_[size_t(index)];
        }
        return 0;
    }

    inline const char* CEnumDescription::labelByIndex(int index) const
    {
        lazyRegister();
        if (size_t(index) < size_t(labels_.size()))
        {
            return labels_[size_t(index)];
        }
        return 0;
    }

    inline int CEnumDescription::value(const char* name) const
    {
        lazyRegister();
        NameToValue::const_iterator it = nameToValue_.find(name);
        YASLI_ESCAPE(it != nameToValue_.end(), return 0);
        return it->second;
    }
    inline int CEnumDescription::valueByLabel(const char* label) const
    {
        lazyRegister();
        LabelToValue::const_iterator it = labelToValue_.find(label);
        YASLI_ESCAPE(it != labelToValue_.end(), return 0);
        return it->second;
    }

    inline void CEnumDescription::lazyRegister() const
    {
        if (m_regListHead)
        {
            NameValue* val = m_regListHead;
            while (val)
            {
                const_cast<CEnumDescription*>(this)->add(val->m_value, val->m_name, val->m_label);
                val = val->m_next;
            }
            const_cast<CEnumDescription*>(this)->m_regListHead = nullptr;
        }
    }
}
// vim:ts=4 sw=4:

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_ENUMIMPL_H
