/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_CRYCOMMON_ICONSOLEMOCK_H
#define CRYINCLUDE_CRYCOMMON_ICONSOLEMOCK_H
#pragma once

#include <IConsole.h>
#include <AzTest/AzTest.h>

// Implements the Console interface
class ConsoleMock
    : public ::IConsole
{
public:
    MOCK_METHOD0(Release, void());
    MOCK_METHOD1(Init, void(ISystem * pSystem));
    MOCK_METHOD5(RegisterString, ICVar * (const char* sName, const char* sValue, int nFlags, const char* help, ConsoleVarFunc pChangeFunc));
    MOCK_METHOD5(RegisterInt, ICVar * (const char* sName, int iValue, int nFlags, const char* help, ConsoleVarFunc pChangeFunc));
    MOCK_METHOD5(RegisterFloat, ICVar * (const char* sName, float fValue, int nFlags, const char* help, ConsoleVarFunc pChangeFunc));
    MOCK_METHOD7(Register, ICVar * (const char* name, float* src, float defaultvalue, int nFlags, const char* help, ConsoleVarFunc pChangeFunc, bool allowModify));
    MOCK_METHOD7(Register, ICVar * (const char* name, int* src, int defaultvalue, int nFlags, const char* help, ConsoleVarFunc pChangeFunc, bool allowModify));
    MOCK_METHOD7(Register, ICVar * (const char* name, const char** src, const char* defaultvalue, int nFlags, const char* help, ConsoleVarFunc pChangeFunc, bool allowModify));
    MOCK_METHOD2(UnregisterVariable, void(const char* sVarName, bool bDelete ));
    MOCK_METHOD1(SetScrollMax, void(int value));
    MOCK_METHOD1(AddOutputPrintSink, void(IOutputPrintSink * inpSink));
    MOCK_METHOD1(RemoveOutputPrintSink, void(IOutputPrintSink * inpSink));
    MOCK_METHOD2(ShowConsole, void(bool show, int iRequestScrollMax));
    MOCK_METHOD2(DumpCVars, void(ICVarDumpSink * pCallback, unsigned int nFlagsFilter));
    MOCK_METHOD2(CreateKeyBind, void(const char* sCmd, const char* sRes));
    MOCK_METHOD2(SetImage, void (ITexture * pImage, bool bDeleteCurrent));
    MOCK_METHOD0(GetImage, ITexture * ());
    MOCK_METHOD1(StaticBackground, void (bool bStatic));
    MOCK_METHOD1(SetLoadingImage, void(const char* szFilename));
    MOCK_CONST_METHOD3(GetLineNo, bool(int indwLineNo, char* outszBuffer, int indwBufferSize));
    MOCK_CONST_METHOD0(GetLineCount, int ());
    MOCK_METHOD1(GetCVar, ICVar * (const char* name));
    MOCK_METHOD3(GetVariable, char*(const char* szVarName, const char* szFileName, const char* def_val));
    MOCK_METHOD3(GetVariable, float (const char* szVarName, const char* szFileName, float def_val));
    MOCK_METHOD1(PrintLine, void (const char* s));
    MOCK_METHOD1(PrintLinePlus, void (const char* s));
    MOCK_METHOD0(GetStatus, bool());
    MOCK_METHOD0(Clear, void());
    MOCK_METHOD0(Update, void());
    MOCK_METHOD0(Draw, void());
    MOCK_METHOD4(AddCommand, bool (const char* sCommand, ConsoleCommandFunc func, int nFlags, const char* sHelp));
    MOCK_METHOD4(AddCommand, bool (const char* sName, const char* sScriptFunc, int nFlags, const char* sHelp));
    MOCK_METHOD1(RemoveCommand, void (const char* sName));
    MOCK_METHOD3(ExecuteString, void (const char* command, bool bSilentMode, bool bDeferExecution ));
    MOCK_METHOD0(IsOpened, bool ());
    MOCK_METHOD0(GetNumVars, int());
    MOCK_METHOD0(GetNumVisibleVars, int());
    MOCK_METHOD2(GetSortedVars, size_t (AZStd::vector<AZStd::string_view>& pszArray, const char* szPrefix));
    MOCK_METHOD1(AutoComplete, const char*(const char* substr));
    MOCK_METHOD1(AutoCompletePrev, const char*(const char* substr));
    MOCK_METHOD1(ProcessCompletion, const char*(const char* szInputBuffer));
    MOCK_METHOD2(RegisterAutoComplete, void (const char* sVarOrCommand, IConsoleArgumentAutoComplete * pArgAutoComplete));
    MOCK_METHOD1(UnRegisterAutoComplete, void (const char* sVarOrCommand));
    MOCK_METHOD0(ResetAutoCompletion, void ());
    MOCK_METHOD1(ResetProgressBar, void (int nProgressRange));
    MOCK_METHOD0(TickProgressBar, void ());
    MOCK_METHOD1(SetInputLine, void (const char* szLine));
    MOCK_METHOD1(DumpKeyBinds, void (IKeyBindDumpSink * pCallback));
    MOCK_CONST_METHOD1(FindKeyBind, const char*(const char* sCmd));
    MOCK_METHOD1(AddConsoleVarSink, void (IConsoleVarSink * pSink));
    MOCK_METHOD1(RemoveConsoleVarSink, void (IConsoleVarSink * pSink));
    MOCK_METHOD1(GetHistoryElement, const char*(bool bUpOrDown));
    MOCK_METHOD1(AddCommandToHistory, void (const char* szCommand));
    MOCK_METHOD2(LoadConfigVar, void (const char* sVariable, const char* sValue));
    MOCK_METHOD1(EnableActivationKey, void (bool bEnable));
    MOCK_METHOD2(SetClientDataProbeString, void (const char* pName, const char* pValue));

    // can't mock variadic methods, so just override here.
    void Exit([[maybe_unused]] const char* command, ...) override {};
};

#endif // CRYINCLUDE_CRYCOMMON_ICONSOLEMOCK_H
