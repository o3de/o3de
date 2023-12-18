/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "CmdLine.h"


void CCmdLine::PushCommand(const AZStd::string& sCommand, const AZStd::string& sParameter)
{
    if (sCommand.empty())
    {
        return;
    }

    ECmdLineArgType type = eCLAT_Normal;
    const char* szCommand = sCommand.c_str();

    if (sCommand[0] == '-')
    {
        type = eCLAT_Pre;
        ++szCommand;

        // Handle cmd line parameters that use -- properly
        if (szCommand[0] == '-')
        {
            ++szCommand;
        }
    }
    else if (sCommand[0] == '+')
    {
        type = eCLAT_Post;
        ++szCommand;
    }

    m_args.push_back(CCmdLineArg(szCommand, sParameter.c_str(), type));
}

CCmdLine::CCmdLine(const char* commandLine)
{
    m_sCmdLine = commandLine;

    char* src = (char*)commandLine;

    AZStd::string command, parameter;

    for (;; )
    {
        if (*src == '\0')
        {
            break;
        }

        AZStd::string arg = Next(src);

        if (m_args.empty())
        {
            // this is the filename, convert backslash to forward slash
            AZ::StringFunc::Replace(arg, '\\', '/');
            m_args.push_back(CCmdLineArg("filename", arg.c_str(), eCLAT_Executable));
        }
        else if(!arg.empty())
        {
            bool bSecondCharIsNumber = false;

            if ((arg.length() > 1) && arg[1] >= '0' && arg[1] <= '9')
            {
                bSecondCharIsNumber = true;
            }

            if ((arg[0] == '-' && !bSecondCharIsNumber)
                || (arg[0] == '+' && !bSecondCharIsNumber)
                || command.empty())                                             // separator or first parameter
            {
                PushCommand(command, parameter);

                command = arg;
                parameter = "";
            }
            else
            {
                if (parameter.empty())
                {
                    parameter = arg;
                }
                else
                {
                    parameter += AZStd::string(" ") + arg;
                }
            }
        }
    }

    PushCommand(command, parameter);
}

CCmdLine::~CCmdLine()
{
}

const ICmdLineArg* CCmdLine::GetArg(int n) const
{
    if ((n >= 0) && (n < (int)m_args.size()))
    {
        return &m_args[n];
    }

    return 0;
}

int CCmdLine::GetArgCount() const
{
    return (int)m_args.size();
}

const ICmdLineArg* CCmdLine::FindArg(const ECmdLineArgType ArgType, const char* name, bool caseSensitive) const
{
    if (caseSensitive)
    {
        for (std::vector<CCmdLineArg>::const_iterator it = m_args.begin(); it != m_args.end(); ++it)
        {
            if (it->GetType() == ArgType)
            {
                if (!strcmp(it->GetName(), name))
                {
                    return &(*it);
                }
            }
        }
    }
    else
    {
        for (std::vector<CCmdLineArg>::const_iterator it = m_args.begin(); it != m_args.end(); ++it)
        {
            if (it->GetType() == ArgType)
            {
                if (!_stricmp(it->GetName(), name))
                {
                    return &(*it);
                }
            }
        }
    }

    return 0;
}


AZStd::string CCmdLine::Next(char*& src)
{
    char ch = 0;
    char* org = src;

    ch = *src++;
    while (ch)
    {
        switch (ch)
        {
        case '\'':
        case '\"':
            org = src;

            while ((*src++ != ch) && *src)
            {
                ;
            }

            return AZStd::string(org, src - 1);

        case '[':
            org = src;
            while ((*src++ != ']') && *src)
            {
                ;
            }
            return AZStd::string(org, src - 1);

        case ' ':
            ch = *src++;
            continue;
        default:
            org = src - 1;
            for (; *src != ' ' && *src != '\t' && *src; ++src)
            {
                ;
            }

            return AZStd::string(org, src);
        }
    }

    return AZStd::string();
}


