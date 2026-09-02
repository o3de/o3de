#ifndef RAPIDXML_UTILS_HPP_INCLUDED
#define RAPIDXML_UTILS_HPP_INCLUDED

// Copyright (C) 2006, 2009 Marcin Kalicinski
// Version 1.13
// Revision $DateTime: 2009/05/13 01:46:17 $
//! \file rapidxml_utils.hpp This file contains high-level rapidxml utilities that can be useful
//! in certain simple scenarios. They should probably not be used if maximizing performance is the main objective.

// Warning for including RapidXML directly, not through AZCore. Please make sure 
// you always include it trough AZCore. If you really can't define RAPIDXML_SKIP_AZCORE_ERROR
#ifndef RAPIDXML_SKIP_AZCORE_ERROR
#error You should include rapidxml via AZCore/XML/rapidxml.h ..., not directly! 
#endif
#include "rapidxml.h"

namespace AZ {
namespace rapidxml
{
    //! Counts children of node. Time complexity is O(n).
    //! \return Number of children of node
    template<class Ch>
    inline std::size_t count_children(xml_node<Ch> *node)
    {
        xml_node<Ch> *child = node->first_node();
        std::size_t count = 0;
        while (child)
        {
            ++count;
            child = child->next_sibling();
        }
        return count;
    }

    //! Counts attributes of node. Time complexity is O(n).
    //! \return Number of attributes of node
    template<class Ch>
    inline std::size_t count_attributes(xml_node<Ch> *node)
    {
        xml_attribute<Ch> *attr = node->first_attribute();
        std::size_t count = 0;
        while (attr)
        {
            ++count;
            attr = attr->next_attribute();
        }
        return count;
    }

}
} // namespace AZ

#endif
