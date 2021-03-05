// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/////////////////////////////////////////////////////////////////////////
/// @file  optparser.h
///
/// @brief Option parser template
/////////////////////////////////////////////////////////////////////////


#pragma once

#include <OpenImageIO/strutil.h>
#include <string>

OIIO_NAMESPACE_BEGIN


/// Parse a string of the form "name=value" and then call
/// system.attribute (name, value), with appropriate type conversions.
template<class C>
inline bool
optparse1(C& system, const std::string& opt)
{
    std::string::size_type eq_pos = opt.find_first_of("=");
    if (eq_pos == std::string::npos) {
        // malformed option
        return false;
    }
    std::string name(opt, 0, eq_pos);
    // trim the name
    while (name.size() && name[0] == ' ')
        name.erase(0);
    while (name.size() && name[name.size() - 1] == ' ')
        name.erase(name.size() - 1);
    std::string value(opt, eq_pos + 1, std::string::npos);
    if (name.empty())
        return false;
    char v = value.size() ? value[0] : ' ';
    if ((v >= '0' && v <= '9') || v == '+' || v == '-') {  // numeric
        if (strchr(value.c_str(), '.'))
            return system.attribute(name, Strutil::stof(value));  // float
        else
            return system.attribute(name, Strutil::stoi(value));  // int
    }
    // otherwise treat it as a string

    // trim surrounding double quotes
    if (value.size() >= 2 && (value[0] == '\"' || value[0] == '\'')
        && value[value.size() - 1] == value[0])
        value = std::string(value, 1, value.size() - 2);

    return system.attribute(name, value);
}



/// Parse a string with comma-separated name=value directives, calling
/// system.attribute(name,value) for each one, with appropriate type
/// conversions.  Examples:
///    optparser(texturesystem, "verbose=1");
///    optparser(texturesystem, "max_memory_MB=32.0");
///    optparser(texturesystem, "a=1,b=2,c=3.14,d=\"a string\"");
template<class C>
inline bool
optparser(C& system, const std::string& optstring)
{
    bool ok    = true;
    size_t len = optstring.length();
    size_t pos = 0;
    while (pos < len) {
        std::string opt;
        char inquote = 0;
        while (pos < len) {
            unsigned char c = optstring[pos];
            if (c == inquote) {
                // Ending a quote
                inquote = 0;
                opt += c;
                ++pos;
            } else if (c == '\"' || c == '\'') {
                // Found a quote
                inquote = c;
                opt += c;
                ++pos;
            } else if (c == ',' && !inquote) {
                // Hit a comma and not inside a quote -- we have an option
                ++pos;  // skip the comma
                break;  // done with option
            } else {
                // Anything else: add to the option
                opt += c;
                ++pos;
            }
        }
        // At this point, opt holds an option
        ok &= optparse1(system, opt);
    }
    return ok;
}


OIIO_NAMESPACE_END
