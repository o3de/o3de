/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_XCONSOLE_H
#define CRYINCLUDE_CRYSYSTEM_XCONSOLE_H

#pragma once

#include <IConsole.h>
#include <CryCommon/StlUtils.h>
#include <CryCommon/TimeValue.h>
#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/CommandLine/CommandRegistrationBus.h>

#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Events/InputTextEventListener.h>

#include <list>
#include <map>
//forward declaration
struct INetwork;
class CSystem;
struct IFFont;

#define MAX_HISTORY_ENTRIES 50
#define LINE_BORDER 10

enum ScrollDir
{
    sdDOWN,
    sdUP,
    sdNONE
};

//////////////////////////////////////////////////////////////////////////
// Console command holds information about commands registered to console.
//////////////////////////////////////////////////////////////////////////
struct CConsoleCommand
{
    AZStd::string m_sName;            // Console command name
    AZStd::string m_sCommand;         // lua code that is executed when this command is invoked
    AZStd::string m_sHelp;            // optional help string - can be shown in the console with "<commandname> ?"
    int    m_nFlags;                  // bitmask consist of flag starting with VF_ e.g. VF_CHEAT
    ConsoleCommandFunc m_func;        // Pointer to console command.

    //////////////////////////////////////////////////////////////////////////
    CConsoleCommand()
        : m_func(nullptr)
        , m_nFlags(0) {}
    size_t sizeofThis () const {return sizeof(*this) + m_sName.capacity() + 1 + m_sCommand.capacity() + 1; }
};

//////////////////////////////////////////////////////////////////////////
// Implements IConsoleCmdArgs.
//////////////////////////////////////////////////////////////////////////
struct CConsoleCommandArgs
    : public IConsoleCmdArgs
{
    CConsoleCommandArgs(AZStd::string& line, std::vector<AZStd::string>& args)
        : m_line(line)
        , m_args(args) {};
    int GetArgCount() const override { return static_cast<int>(m_args.size()); };
    // Get argument by index, nIndex must be in 0 <= nIndex < GetArgCount()
    const char* GetArg(int nIndex) const override
    {
        assert(nIndex >= 0 && nIndex < GetArgCount());
        if (!(nIndex >= 0 && nIndex < GetArgCount()))
        {
            return nullptr;
        }
        return m_args[nIndex].c_str();
    }
    const char* GetCommandLine() const override
    {
        return m_line.c_str();
    }

private:
    std::vector<AZStd::string>& m_args;
    AZStd::string& m_line;
};



struct string_nocase_lt
{
    bool operator()(const AZStd::string& s1, const AZStd::string& s2) const
    {
        return azstricmp(s1.c_str(), s2.c_str()) < 0;
    }
};

//forward declarations
class ITexture;
struct IRenderer;


/*! engine console implementation
    @see IConsole
*/
class CXConsole
    : public IConsole
    , public AzFramework::InputChannelEventListener
    , public AzFramework::InputTextEventListener
    , public IRemoteConsoleListener
    , public AzFramework::ConsoleRequestBus::Handler
    , public AzFramework::CommandRegistrationBus::Handler
{
public:
    using ConsoleBuffer = std::deque<AZStd::string>;
    using ConsoleBufferItor = ConsoleBuffer::iterator;
    using ConsoleBufferRItor = ConsoleBuffer::reverse_iterator;

    // constructor
    CXConsole();
    // destructor
    virtual ~CXConsole();

    void SetStatus(bool bActive){ m_bConsoleActive = bActive; }
    bool GetStatus() const { return m_bConsoleActive; }
    //
    void FreeRenderResources();
    //
    void Copy();
    void Paste();

    // interface IConsole ---------------------------------------------------------
    void Release() override;

    void Init(ISystem* pSystem) override;
    ICVar* RegisterString(const char* sName, const char* sValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) override;
    ICVar* RegisterInt(const char* sName, int iValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) override;
    ICVar* RegisterFloat(const char* sName, float fValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) override;
    ICVar* Register(const char* name, float* src, float defaultvalue, int flags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = 0, bool allowModify = true) override;
    ICVar* Register(const char* name, int* src, int defaultvalue, int flags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = 0, bool allowModify = true) override;
    ICVar* Register(const char* name, const char** src, const char* defaultvalue, int flags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = 0, bool allowModify = true) override;

    void UnregisterVariable(const char* sVarName, bool bDelete = false) override;
    void SetScrollMax(int value) override;
    void AddOutputPrintSink(IOutputPrintSink* inpSink) override;
    void RemoveOutputPrintSink(IOutputPrintSink* inpSink) override;
    void ShowConsole(bool show, int iRequestScrollMax = -1) override;
    void DumpCVars(ICVarDumpSink* pCallback, unsigned int nFlagsFilter = 0) override;
    void DumpKeyBinds(IKeyBindDumpSink* pCallback) override;
    void CreateKeyBind(const char* sCmd, const char* sRes) override;
    const char* FindKeyBind(const char* sCmd) const override;
    void    SetImage(ITexture* pImage, bool bDeleteCurrent) override;
    inline ITexture* GetImage() override { return m_pImage; }
    void StaticBackground(bool bStatic) override { m_bStaticBackground = bStatic; }
    bool GetLineNo(int indwLineNo, char* outszBuffer, int indwBufferSize) const override;
    int GetLineCount() const override;
    ICVar* GetCVar(const char* name) override;
    char* GetVariable(const char* szVarName, const char* szFileName, const char* def_val) override;
    float GetVariable(const char* szVarName, const char* szFileName, float def_val) override;
    void PrintLine(const char* s) override;
    void PrintLinePlus(const char* s) override;
    bool GetStatus() override;
    void Clear() override;
    void Update() override;
    void Draw() override;
    bool AddCommand(const char* sCommand, ConsoleCommandFunc func, int nFlags = 0, const char* sHelp = NULL) override;
    bool AddCommand(const char* sName, const char* sScriptFunc, int nFlags = 0, const char* sHelp = NULL) override;
    void RemoveCommand(const char* sName) override;
    void ExecuteString(const char* command, bool bSilentMode, bool bDeferExecution = false) override;
    void ExecuteConsoleCommand(const char* command) override;
    void ResetCVarsToDefaults() override;
    void Exit(const char* command, ...) override PRINTF_PARAMS(2, 3);
    bool IsOpened() override;
    int GetNumVars() override;
    int GetNumVisibleVars() override;
    size_t GetSortedVars(AZStd::vector<AZStd::string_view>& pszArray, const char* szPrefix = 0) override;
    void FindVar(const char* substr);
    const char* AutoComplete(const char* substr) override;
    const char* AutoCompletePrev(const char* substr) override;
    const char* ProcessCompletion(const char* szInputBuffer) override;
    void RegisterAutoComplete(const char* sVarOrCommand, IConsoleArgumentAutoComplete* pArgAutoComplete) override;
    void UnRegisterAutoComplete(const char* sVarOrCommand) override;
    void ResetAutoCompletion() override;
    void ResetProgressBar(int nProgressRange) override;
    void TickProgressBar() override;
    void SetLoadingImage(const char* szFilename) override;
    void AddConsoleVarSink(IConsoleVarSink* pSink) override;
    void RemoveConsoleVarSink(IConsoleVarSink* pSink) override;
    const char* GetHistoryElement(bool bUpOrDown) override;
    void AddCommandToHistory(const char* szCommand) override;
    void SetInputLine(const char* szLine) override;
    void LoadConfigVar(const char* sVariable, const char* sValue) override;
    void EnableActivationKey(bool bEnable) override;
    void SetClientDataProbeString(const char* pName, const char* pValue) override;

    // InputChannelEventListener / InputTextEventListener
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
    bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;

    // interface IRemoteConsoleListener ------------------------------------------------------------------

    void OnConsoleCommand(const char* cmd) override;

    // interface IConsoleVarSink ----------------------------------------------------------------------

    virtual bool OnBeforeVarChange(ICVar* pVar, const char* sNewValue);
    virtual void OnAfterVarChange(ICVar* pVar);

    // interface CommandRegistration --------------------------------------------------------------------
    bool RegisterCommand(AZStd::string_view identifier, AZStd::string_view helpText, AZ::u32 commandFlags, AzFramework::CommandFunction callback) override;
    bool UnregisterCommand(AZStd::string_view identifier) override;
    void ExecuteRegisteredCommand(IConsoleCmdArgs* pArg);

    //////////////////////////////////////////////////////////////////////////

    void SetProcessingGroup(bool isGroup) { m_bIsProcessingGroup = isGroup; }
    bool GetIsProcessingGroup() const { return m_bIsProcessingGroup; }

protected: // ----------------------------------------------------------------------------------------
    void DrawBuffer(int nScrollPos, const char* szEffect);

    void RegisterVar(ICVar* pCVar, ConsoleVarFunc pChangeFunc = nullptr);

    bool ProcessInput(const AzFramework::InputChannel& inputChannel);
    void AddLine(const char* inputStr);
    void AddLinePlus(const char* inputStr);
    void AddInputUTF8(const AZStd::string& textUTF8);
    void RemoveInputChar(bool bBackSpace);
    void ExecuteInputBuffer();
    void ExecuteCommand(CConsoleCommand& cmd, AZStd::string& params, bool bIgnoreDevMode = false);

    void ScrollConsole();

    // CommandRegistration usage
    struct CommandRegistrationEntry
    {
        AzFramework::CommandFunction m_callback;
        AZStd::string m_id;
        AZStd::string m_helpText;
    };
    AZStd::unordered_map<AZStd::string, CommandRegistrationEntry> m_commandRegistrationMap;

#if ALLOW_AUDIT_CVARS
    void AuditCVars(IConsoleCmdArgs* pArg);
#endif // ALLOW_AUDIT_CVARS

#ifndef _RELEASE
    // will be removed once the HTML version is good enough
    void DumpCommandsVarsTxt(const char* prefix);
    void DumpVarsTxt(const bool includeCheat);
#endif

    void ConsoleLogInputResponse(const char* szFormat, ...) PRINTF_PARAMS(2, 3);
    void ConsoleLogInput(const char* szFormat, ...) PRINTF_PARAMS(2, 3);
    void ConsoleWarning(const char* szFormat, ...) PRINTF_PARAMS(2, 3);

    void DisplayHelp(const char* help, const char* name);
    void DisplayVarValue(ICVar* pVar);

    // Arguments:
    //   bFromConsole - true=from console, false=from outside
    void SplitCommands(const char* line, std::list<AZStd::string>& split);
    void ExecuteStringInternal(const char* command, const bool bFromConsole, const bool bSilentMode = false);
    void ExecuteDeferredCommands();

    static const char* GetFlagsString(const uint32 dwFlags);

private: // ----------------------------------------------------------

    typedef std::map<const char*, ICVar*, string_nocase_lt> ConsoleVariablesMap;        // key points into string stored in ICVar or in .exe/.dll
    typedef ConsoleVariablesMap::iterator ConsoleVariablesMapItor;

    using ConsoleVariablesVector = std::vector<std::pair<const char*, ICVar*> >;

    void LogChangeMessage(const char* name, const bool isConst, const bool isCheat, const bool isReadOnly, const bool isDeprecated,
        const char* oldValue, const char* newValue, const bool isProcessingGroup, const bool allowChange);

    void AddCheckedCVar(ConsoleVariablesVector& vector, const ConsoleVariablesVector::value_type& value);
    void RemoveCheckedCVar(ConsoleVariablesVector& vector, const ConsoleVariablesVector::value_type& value);
    static bool CVarNameLess(const std::pair<const char*, ICVar*>& lhs, const std::pair<const char*, ICVar*>& rhs);

    void PostLine(const char* lineOfText, size_t len);

    typedef std::map<AZStd::string, CConsoleCommand, string_nocase_lt> ConsoleCommandsMap;
    typedef ConsoleCommandsMap::iterator ConsoleCommandsMapItor;

    typedef std::map<AZStd::string, AZStd::string> ConsoleBindsMap;
    typedef ConsoleBindsMap::iterator ConsoleBindsMapItor;

    typedef std::map<AZStd::string, IConsoleArgumentAutoComplete*, stl::less_stricmp<AZStd::string> > ArgumentAutoCompleteMap;

    struct SConfigVar
    {
        AZStd::string m_value;
        bool m_partOfGroup;
    };
    typedef std::map<AZStd::string, SConfigVar, string_nocase_lt> ConfigVars;

    struct SDeferredCommand
    {
        AZStd::string  command;
        bool        silentMode;

        SDeferredCommand(const AZStd::string& _command, bool _silentMode)
            : command(_command)
            , silentMode(_silentMode)
        {}
    };
    using TDeferredCommandList = std::list<SDeferredCommand>;

    using ConsoleVarSinks = std::list<IConsoleVarSink*>;

    // --------------------------------------------------------------------------------

    ConsoleBuffer                                       m_dqConsoleBuffer;
    ConsoleBuffer                                       m_dqHistory;

    bool                                                        m_bStaticBackground;
    int                                                         m_nLoadingBackTexID;
    int                                                         m_nProgress;
    int                                                         m_nProgressRange;

    AZStd::string                                                  m_sInputBuffer;
    AZStd::string                          m_sReturnString;

    AZStd::string                                                  m_sPrevTab;
    int                                                         m_nTabCount;

    ConsoleCommandsMap                          m_mapCommands;                      //
    ConsoleBindsMap                                 m_mapBinds;                             //
    ConsoleVariablesMap                         m_mapVariables;                     //
    ConsoleVariablesVector          m_randomCheckedVariables;
    ConsoleVariablesVector          m_alwaysCheckedVariables;
    std::vector<IOutputPrintSink*> m_OutputSinks;                       // objects in this vector are not released

    TDeferredCommandList                        m_deferredCommands;             // A fifo of deferred commands
    bool                                                        m_deferredExecution;            // True when deferred commands are processed
    int                                                         m_waitFrames;                           // A counter which is used by wait_frames command
    CTimeValue                                          m_waitSeconds;                      // An absolute timestamp which is used by wait_seconds command
    int                                                         m_blockCounter;                     // This counter is incremented whenever a blocker command (VF_BLOCKFRAME) is executed.

    ArgumentAutoCompleteMap         m_mapArgumentAutoComplete;

    ConsoleVarSinks                                 m_consoleVarSinks;

    ConfigVars                                          m_configVars;                           // temporary data of cvars that haven't been created yet

    int                                                         m_nScrollPos;
    int                                                         m_nTempScrollMax;                   // for currently opened console, reset to m_nScrollMax
    int                                                         m_nScrollMax;                           //
    int                                                         m_nScrollLine;
    int                                                         m_nHistoryPos;
    size_t                                                      m_nCursorPos;                           // x position in characters
    ITexture*                                               m_pImage;

    float                                                       m_fRepeatTimer;                     // relative, next repeat even in .. decreses over time, repeats when 0, only valid if m_nRepeatEvent.keyId != eKI_Unknown
    AzFramework::InputChannelId                                 m_nRepeatEventId;                     // event that will be repeated

    float                                                       m_fCursorBlinkTimer;            // relative, increases over time,
    bool                                                        m_bDrawCursor;

    ScrollDir                                               m_sdScrollDir;


    AzFramework::SystemCursorState                              m_previousSystemCursorState;
    bool                                                        m_bConsoleActive;
    bool                                                        m_bActivationKeyEnable;
    bool                                                        m_bIsProcessingGroup;
    bool m_bIsConsoleKeyPressed;

    size_t                          m_nCheatHashRangeFirst;
    size_t                          m_nCheatHashRangeLast;
    bool                            m_bCheatHashDirty;
    uint64                          m_nCheatHash;

    CSystem*                                               m_pSystem;
    IFFont*                                                m_pFont;

    ICVar*                                                 m_pSysDeactivateConsole;

    static int con_display_last_messages;
    static int con_line_buffer_size;
    static int con_showonload;
    static int con_debug;
    static int con_restricted;

    friend void Command_SetWaitSeconds(IConsoleCmdArgs* Cmd);
    friend void Command_SetWaitFrames(IConsoleCmdArgs* Cmd);
#if ALLOW_AUDIT_CVARS
    friend void Command_AuditCVars(IConsoleCmdArgs* pArg);
#endif // ALLOW_AUDIT_CVARS
    friend void Command_DumpCommandsVars(IConsoleCmdArgs* Cmd);
    friend void Command_DumpVars(IConsoleCmdArgs* Cmd);
    friend class CConsoleHelpGen;
};

#endif // CRYINCLUDE_CRYSYSTEM_XCONSOLE_H
