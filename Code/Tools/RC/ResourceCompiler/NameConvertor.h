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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_NAMECONVERTOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_NAMECONVERTOR_H
#pragma once

#include "StringHelpers.h"
#include "IRCLog.h"  // RCLog

class NameConvertor
{
private:
    struct Rule
    {
        string mask;
        string format;
    };
    std::vector<Rule> m_rules;

public:
    bool HasRules() const
    {
        return !m_rules.empty();
    }

    bool SetRules(const string& rules)
    {
        m_rules.clear();

        std::vector<string> tmpPairs;
        std::vector<string> tmpPair;

        StringHelpers::Split(rules, ";", false, tmpPairs);

        for (size_t i = 0; i < tmpPairs.size(); ++i)
        {
            tmpPair.clear();
            StringHelpers::Split(tmpPairs[i], ",", false, tmpPair);

            if (tmpPair.size() != 2 || tmpPair[0].empty() || tmpPair[1].empty())
            {
                RCLogError("%s: Syntax error in converting rule: '%s'", __FUNCTION__, tmpPairs[i].c_str());
                m_rules.clear();
                return false;
            }

            Rule r;
            r.mask = tmpPair[0];
            r.format = tmpPair[1];
            m_rules.push_back(r);
        }

        return true;
    }

    string GetConvertedName(const string& name) const
    {
        if (name.empty())
        {
            RCLogError("Empty name sent to %s", __FUNCTION__);
            return string();
        }

        if (m_rules.empty())
        {
            return name;
        }

        const Rule* pRule = 0;
        std::vector<string> tokens;

        for (size_t i = 0; i < m_rules.size(); ++i)
        {
            if (StringHelpers::MatchesWildcardsIgnoreCase(name, m_rules[i].mask))
            {
                if (!StringHelpers::MatchesWildcardsIgnoreCaseExt(name, m_rules[i].mask, tokens))
                {
                    RCLogError("Unexpected failure in %s", __FUNCTION__);
                    return string();
                }
                pRule = &m_rules[i];
                break;
            }
        }

        if (!pRule)
        {
            return name;
        }

        string newName = pRule->format;

        size_t scanFromPos = 0;
        for (;; )
        {
            const size_t startPos = newName.find('{', scanFromPos);
            if (startPos == newName.npos)
            {
                break;
            }

            const size_t endPos = newName.find('}', startPos + 1);
            if (endPos == newName.npos)
            {
                break;
            }

            const string indexStr = newName.substr(startPos + 1, endPos - startPos - 1);

            bool bBadSyntax = indexStr.empty();
            for (size_t i = 0; i < indexStr.length(); ++i)
            {
                if (!isdigit(indexStr[i]))
                {
                    bBadSyntax = true;
                }
            }
            if (bBadSyntax)
            {
                RCLogError("%s: Syntax error in element {%s} in input string %s", __FUNCTION__, indexStr.c_str(), pRule->format.c_str());
                return string();
            }

            const int index = atoi(indexStr.c_str());
            if ((index < 0) || (index > tokens.size()))
            {
                RCLogError("%s: Bad index specified in {%s} in input string %s", __FUNCTION__, indexStr.c_str(), pRule->format.c_str());
                return string();
            }

            const string& replaceWith = (index == 0) ? name : tokens[index - 1];
            newName = newName.replace(startPos, endPos - startPos + 1, replaceWith);
            scanFromPos = startPos + replaceWith.size();
        }

        return newName;
    }
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_NAMECONVERTOR_H
