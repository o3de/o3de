/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

// NOTE: this file is included in LuxCore so any external dependency must be
// avoided here

#ifndef _LUXRAYS_PROPERTIES_H
#define _LUXRAYS_PROPERTIES_H

#include <map>
#include <vector>
#include <string>
#include <istream>
#include <cstdarg>
#include <stdexcept>

#include <luxrays/utils/exportdefs.h>

namespace luxrays {

//------------------------------------------------------------------------------
// Blob class
//------------------------------------------------------------------------------

CPP_EXPORT class CPP_API Blob {
public:
    Blob(const Blob &blob);
    Blob(const char *data, const size_t size);
    Blob(const std::string &base64Data);
    ~Blob();

    const char *GetData() const { return data; }
    size_t GetSize() const { return size; }

    std::string ToString() const;

    Blob &operator=(const Blob &blob);

private:
    char *data;
    size_t size;
};

CPP_EXPORT CPP_API std::ostream &operator<<(std::ostream &os, const Blob &blob);

//------------------------------------------------------------------------------
// Property class
//------------------------------------------------------------------------------

/*!
 * \brief Value that can be stored in a Property.
 *
 * The current list of allowed data types is:
 * - bool
 * - int
 * - unsigned int
 * - float
 * - double
 * - unsigned long long
 * - string
 * - Blob
 */

// I was using boost::variant for PropertyValue in the past but I want to avoid
// Boost requirement for application using LuxCore API

CPP_EXPORT class CPP_API PropertyValue {
public:
    typedef enum {
        NONE_VAL,
        BOOL_VAL,
        INT_VAL,
        UINT_VAL,
        FLOAT_VAL,
        DOUBLE_VAL,
        ULONGLONG_VAL,
        STRING_VAL,
        BLOB_VAL
    } DataType;

    PropertyValue();
    PropertyValue(const PropertyValue &propVal);
    PropertyValue(const bool val);
    PropertyValue(const int val);
    PropertyValue(const unsigned int val);
    PropertyValue(const float val);
    PropertyValue(const double val);
    PropertyValue(const unsigned long long val);
    PropertyValue(const std::string &val);
    PropertyValue(const Blob &val);
    ~PropertyValue();

    template<class T> T Get() const;

    DataType GetValueType() const;
    
    PropertyValue &operator=(const PropertyValue &propVal);

private:
    static void Copy(const PropertyValue &prop0Val, PropertyValue &prop1Val);

    DataType dataType;

    union {
        bool boolVal;
        int intVal;
        unsigned int uintVal;
        float floatVal;
        double doubleVal;
        unsigned long long ulonglongVal;
        std::string *stringVal;
        Blob *blobVal;
    } data;
};

// Get<>() basic types specializations
template<> CPP_API bool PropertyValue::Get<bool>() const;
template<> CPP_API int PropertyValue::Get<int>() const;
template<> CPP_API unsigned int PropertyValue::Get<unsigned int>() const;
template<> CPP_API float PropertyValue::Get<float>() const;
template<> CPP_API double PropertyValue::Get<double>() const;
template<> CPP_API unsigned long long PropertyValue::Get<unsigned long long>() const;
template<> CPP_API std::string PropertyValue::Get<std::string>() const;
template<> CPP_API const Blob &PropertyValue::Get<const Blob &>() const;

/*!
 * \brief A vector of values that can be stored in a Property.
 */
typedef std::vector<PropertyValue> PropertyValues;

/*!
 * \brief A generic container for values.
 *
 * A Property is a container associating a vector of values to a string name. The
 * vector of values can include items with different data types. Check
 * \ref PropertyValue for a list of allowed types.
 */
CPP_EXPORT class CPP_API Property {
public:
    /*!
     * \brief Constructs a new empty property.
     *
     * Constructs a new empty property where the property name is initialized to
     * the empty string (i.e. "") and the vector of values is empty too.
     */
    Property();
    /*!
     * \brief Constructs a new empty property with a given name.
     *
     * Constructs a new empty property where the property name is initialized
     * to \p propName and the vector of values is empty too.
     *
     * \param propName is the name of the new property.
     */
    Property(const std::string &propName);
    /*!
     * \brief Constructs a new property with a given name and value.
     *
     * Constructs a new property where the property name is initialized to
     * \p propName and the vector of values has one single element with the value
     * of \p val.
     *
     * \param propName is the name of the new property.
     * \param val is the value of the new property.
     */
    Property(const std::string &propName, const PropertyValue &val);
    /*!
     * \brief Constructs a new property with a given name and values.
     *
     * Constructs a new property where the property name is initialized to
     * \p propName and the vector of values is initialize with the values
     * of \p vals.
     *
     * \param propName is the name of the new property.
     * \param vals is the value of the new property.
     */
    Property(const std::string &propName, const PropertyValues &vals);
    ~Property();

    /*!
     * \brief Returns the name of a property.
     *
     * \return the name of the property
     */
    const std::string &GetName() const { return name; }
    /*!
     * \brief Return a new property with a prefix added to the name.
     * 
     * \param prefix is the string to add to the name.
     *
     * \return a new property.
     */
    Property AddedNamePrefix(const std::string &prefix) const {
        Property newProp(prefix + name);
        newProp.values.insert(newProp.values.begin(), values.begin(), values.end());

        return newProp;
    }
    /*!
     * \brief Return a new property with a new name.
     * 
     * \param prefix is the string to use for the new name.
     *
     * \return a new property.
     */
    Property Renamed(const std::string &newName) const {
        Property newProp(newName);
        newProp.values.insert(newProp.values.begin(), values.begin(), values.end());

        return newProp;
    }
    /*!
     * \brief Returns the number of values associated to this property.
     *
     * \return the number of values in this property.
     */
    size_t GetSize() const { return values.size(); }
    /*!
     * \brief Removes any values associated to the property.
     *
     * \return a reference to the modified property.
     */
    Property &Clear();
    /*!
     * \brief Returns the value at the specified position.
     * 
     * \param index is the position of the value to return.
     * 
     * \return the value at specified position (casted or translated to the type
     * required).
     * 
     * \throws std::runtime_error if the index is out of bound.
     */
    template<class T> T Get(const unsigned int index) const {
        if (index >= values.size())
            throw std::runtime_error("Out of bound error for property: " + name);

        return values[index].Get<T>();
    }
    /*!
     * \brief Returns the type of the value at the specified position.
     * 
     * \param index is the position of the value.
     * 
     * \return the type information of the value at specified position.
     * 
     * \throws std::runtime_error if the index is out of bound.
     */
    const PropertyValue::DataType GetValueType(const unsigned int index) const {
        if (index >= values.size())
            throw std::runtime_error("Out of bound error for property: " + name);

        return values[index].GetValueType();
    }
    /*!
     * \brief Parses all values as a representation of the specified type.
     *
     * The current list of supported data types is:
     * - bool
     * - int
     * - unsigned int
     * - float
     * - double
     * - unsigned longlong
     * - string
     * - Blob
     * 
     * \return the value at first position (casted or translated to the type
     * required).
     * 
     * \throws std::runtime_error if the property has the wrong number of values
     * for the specified data type.
     */
    template<class T> T Get() const {
        throw std::runtime_error("Unsupported data type in property: " + name);
    }
    /*!
     * \brief Sets the value at the specified position.
     * 
     * \param index is the position of the value to set.
     * \param val is the new value to set.
     *
     * \return a reference to the modified property.
     *
     * \throws std::runtime_error if the index is out of bound.
     */
    template<class T> Property &Set(const unsigned int index, const T &val) {
        if (index >= values.size())
            throw std::runtime_error("Out of bound error for property: " + name);

        values[index] = val;

        return *this;
    }
    /*!
     * \brief Adds an item at the end of the list of values associated with the
     * property.
     * 
     * \param val is the value to append.
     *
     * \return a reference to the modified property.
     */
    template<class T> Property &Add(const T &val) {
        values.push_back(val);
        return *this;
    }
    /*!
     * \brief Returns a string with all values associated to the property.
     * 
     * \return a string with all values.
     */
    std::string GetValuesString() const;
    /*!
     * \brief Initialize the property from a string (ex. "a.b.c = 1 2")
     */
    void FromString(std::string &s);
    /*!
     * \brief Returns a string with the name of the property followed by " = "
     * and by all values associated to the property.
     * 
     * \return a string with the name and all values.
     */
    std::string ToString() const;
    /*!
     * \brief Adds a value to a property.
     *
     * It can be used to write expressions like:
     * 
     * > Property("test1.prop1")("aa")
     * 
     * \param val0 is the value to assign.
     * 
     * \return a reference to the modified property.
     */
    template<class T0> Property &operator()(const T0 &val0) {
        return Add(val0);
    }
    /*!
     * \brief Adds a value to a property.
     *
     * It can be used to write expressions like:
     * 
     * > Property("test1.prop1")(1.f, 2.f)
     * 
     * \param val0 is the value to assign as first item.
     * \param val1 is the value to assign as second item.
     * 
     * \return a reference to the modified property.
     */
    template<class T0, class T1> Property &operator()(const T0 &val0, const T1 &val1) {
        return Add(val0).Add(val1);
    }
    /*!
     * \brief Adds a value to a property.
     *
     * It can be used to write expressions like:
     * 
     * > Property("test1.prop1")(1.f, 2.f, 3.f)
     * 
     * \param val0 is the value to assign as first item.
     * \param val1 is the value to assign as second item.
     * \param val2 is the value to assign as third item.
     * 
     * \return a reference to the modified property.
     */
    template<class T0, class T1, class T2> Property &operator()(const T0 &val0, const T1 &val1, const T2 &val2) {
        return Add(val0).Add(val1).Add(val2);
    }
    /*!
     * \brief Adds a value to a property.
     *
     * It can be used to write expressions like:
     * 
     * > Property("test1.prop1")(1.f, 2.f, 3.f, 4.f)
     * 
     * \param val0 is the value to assign as first item.
     * \param val1 is the value to assign as second item.
     * \param val2 is the value to assign as third item.
     * \param val3 is the value to assign as forth item.
     * 
     * \return a reference to the modified property.
     */
    template<class T0, class T1, class T2, class T3> Property &operator()(const T0 &val0, const T1 &val1, const T2 &val2, const T3 &val3) {
        return Add(val0).Add(val1).Add(val2).Add(val3);
    }
    /*!
     * \brief Adds a vector of values to a property.
     * 
     * \param vals is the value to assign.
     * 
     * \return a reference to the modified property.
     */
    template<class T0> Property &operator()(const std::vector<T0> &vals) {
        for (size_t i = 0; i < vals.size(); ++i)
            values.push_back(vals[i]);

        return *this; 
    }
    /*!
     * \brief Initializes a property with (only) the given value.
     * 
     * \return a reference to the modified property.
     */
    template<class T> Property &operator=(const T &val) {
        values.clear();
        return Add(val);
    }

    /*!
     * \brief Required to work around the problem of char* to bool conversion
     * (instead of char* to string).
     */
    Property &Add(const char *val) {
        values.push_back(std::string(val));
        return *this;
    }
    /*!
     * \brief Required to work around the problem of char* to bool conversion
     * (instead of char* to string).
     */
    Property &Add(char *val) {
        values.push_back(std::string(val));
        return *this;
    }
    Property &operator()(const char *val) {
        return Add(std::string(val));
    }
    /*!
     * \brief Required to work around the problem of char* to bool conversion
     * (instead of char* to string).
     */
    Property &operator()(char *val) {
        return Add(std::string(val));
    }
    /*!
     * \brief Required to work around the problem of char* to bool conversion
     * (instead of char* to string).
     */
    Property &operator=(const char *val) {
        values.clear();
        return Add(std::string(val));
    }
    /*!
     * \brief Required to work around the problem of char* to bool conversion
     * (instead of char* to string).
     */
    Property &operator=(char *val) {
        values.clear();
        return Add(std::string(val));
    }

    static unsigned int CountFields(const std::string &name);
    static std::string ExtractField(const std::string &name, const unsigned int index);
    static std::string ExtractPrefix(const std::string &name, const unsigned int count);
    static std::string PopPrefix(const std::string &name);

private:
    std::string name;
    PropertyValues values;
};

// Get<>() basic types specializations
template<> CPP_API bool Property::Get<bool>() const;
template<> CPP_API int Property::Get<int>() const;
template<> CPP_API unsigned int Property::Get<unsigned int>() const;
template<> CPP_API float Property::Get<float>() const;
template<> CPP_API double Property::Get<double>() const;
template<> CPP_API unsigned long long Property::Get<unsigned long long>() const;
template<> CPP_API std::string Property::Get<std::string>() const;
template<> CPP_API const Blob &Property::Get<const Blob &>() const;

inline std::ostream &operator<<(std::ostream &os, const Property &p) {
    os << p.ToString();

    return os;
}

//------------------------------------------------------------------------------
// Properties class
//------------------------------------------------------------------------------

/*!
 * \brief A container for multiple Property.
 *
 * Properties is a container for instances of Property class. It keeps also
 * track of the insertion order.
 */
CPP_EXPORT class CPP_API Properties {
public:
    Properties() { }
    /*!
     * \brief Sets the list of Property from a text file .
     * 
     * \param fileName is the name of the file to read.
     */
    Properties(const std::string &fileName);
    ~Properties() { }

    /*!
     * \brief Returns the number of Property in this container.
     *
     * \return the number of Property.
     */
    unsigned int GetSize() const;

    // The following 2 methods perform the same action

    /*!
     * \brief Sets a single Property.
     * 
     * \param prop is the Property to set.
     * 
     * \return a reference to the modified properties.
     */
    Properties &Set(const Property &prop);
    /*!
     * \brief Sets a single Property.
     * 
     * \param prop is the Property to set.
     * 
     * \return a reference to the modified properties.
     */
    Properties &operator<<(const Property &prop);
    /*!
     * \brief Sets the list of Property.
     * 
     * \param prop is the list of Property to set.
     * 
     * \return a reference to the modified properties.
     */
    Properties &Set(const Properties &prop);
    /*!
     * \brief Sets the list of Property.
     * 
     * \param props is the list of Property to set.
     * 
     * \return a reference to the modified properties.
     */
    Properties &operator<<(const Properties &props);
    /*!
     * \brief Sets the list of Property while adding a prefix to all names.
     * 
     * \param props is the list of Property to set.
     * 
     * \return a reference to the modified properties.
     */
    Properties &Set(const Properties &props, const std::string &prefix);
    /*!
     * \brief Sets the list of Property coming from a stream.
     * 
     * \param stream is the input stream to read.
     * 
     * \return a reference to the modified properties.
     */
    Properties &SetFromStream(std::istream &stream);
    /*!
     * \brief Sets the list of Property coming from a file.
     * 
     * \param fileName is the name of the file to read.
     * 
     * \return a reference to the modified properties.
     */
    Properties &SetFromFile(const std::string &fileName);
    /*!
     * \brief Sets the list of Property coming from a std::string.
     * 
     * \param propDefinitions is the list of Property to add in text format.
     * 
     * \return a reference to the modified properties.
     */
    Properties &SetFromString(const std::string &propDefinitions);

    /*!
     * \brief Save all properties to a file
     * 
     * \param fileName of the file to create.
     */
    void Save(const std::string &fileName);

    /*!
     * \brief Removes all Property from the container.
     * 
     * \return a reference to the modified properties.
     */
    Properties &Clear();

    /*!
     * \brief Returns all Property names defined.
     * 
     * \return a reference to all Property names defined.
     */
    const std::vector<std::string> &GetAllNames() const;
    /*!
     * \brief Returns all Property names that start with a specific prefix.
     *
     * This method is used to iterate over all properties.
     * 
     * \param prefix of the Property names to return.
     *
     * \return a vector of Property names.
     */
    std::vector<std::string> GetAllNames(const std::string &prefix) const;
    /*!
     * \brief Returns all Property unique names that match the passed regular
     * expression.
     *
     * \param regularExpression to use for the pattern matching.
     *
     * \return a vector of Property names.
     */
    std::vector<std::string> GetAllNamesRE(const std::string &regularExpression) const;
    /*!
     * \brief Returns all Property unique names that start with a specific prefix.
     *
     * For instance, given the the following names:
     * - test.prop1.subprop1
     * - test.prop1.subprop2
     * - test.prop2.subprop1
     * 
     * GetAllUniqueSubNames("test") will return:
     * - test.prop1
     * - test.prop2
     *
     * \param prefix of the Property names to return.
     *
     * \return a vector of Property names.
     */
    std::vector<std::string> GetAllUniqueSubNames(const std::string &prefix) const;
    /*!
     * \brief Returns if there are at least a Property starting for specific prefix.
     *
     * \param prefix of the Property to look for.
     *
     * \return true if there is at least on Property starting for the prefix.
     */
    bool HaveNames(const std::string &prefix) const;
    /*!
     * \brief Returns all a copy of all Property with a name starting with a specific prefix.
     * 
     * \param prefix of the Property names to use.
     *
     * \return a copy of all Property matching the prefix.
     */
    bool HaveNamesRE(const std::string &regularExpression) const;
    /*!
     * \brief Returns a copy of all Property with a name matching the passed
     * regular expression.
     * 
     * \param regularExpression to use for the pattern matching.
     *
     * \return a copy of all Property matching the prefix.
     */
    Properties GetAllProperties(const std::string &prefix) const;
    /*!
     * \brief Returns a property.
     *
     * \param propName is the name of the Property to return.
     *
     * \return a Property.
     *
     * \throws std::runtime_error if the Property doesn't exist.
     */
    const Property &Get(const std::string &propName) const;
    /*!
     * \brief Returns a Property with the same name of the passed Property if
     * it has been defined or the passed Property itself (i.e. the default values).
     *
     * \param defaultProp has the Property to look for and the default values in
     * case it has not been defined.
     *
     * \return a Property.
     */
    const Property &Get(const Property &defaultProp) const;

    /*!
     * \brief Returns if a Property with the given name has been defined.
     *
     * \param propName is the name of the Property to check.
     *
     * \return if defined or not.
     */
    bool IsDefined(const std::string &propName) const;

    /*!
     * \brief Deletes a Property with the given name.
     *
     * \param propName is the name of the Property to delete.
     */
    void Delete(const std::string &propName);
    /*!
     * \brief Deletes all listed Property.
     *
     * \param propNames is the list of the Property to delete.
     */
    void DeleteAll(const std::vector<std::string> &propNames);

    /*!
     * \brief Converts all Properties in a string.
     *
     * \return a string with the definition of all properties.
     */
    std::string ToString() const;

private:
    // This vector is used, among other things, to keep track of the insertion order
    std::vector<std::string> names;
    std::map<std::string, Property> props;
};

CPP_EXPORT CPP_API Properties operator<<(const Property &prop0, const Property &prop1);
CPP_EXPORT CPP_API Properties operator<<(const Property &prop0, const Properties &props);

inline std::ostream &operator<<(std::ostream &os, const Properties &p) {
    os << p.ToString();

    return os;
}

}

#endif /* _LUXRAYS_PROPERTIES_H */
