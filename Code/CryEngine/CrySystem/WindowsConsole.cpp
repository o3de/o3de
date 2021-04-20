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

// Description : CWindowsConsole member definitions


#include "CrySystem_precompiled.h"
#include "System.h"
#include "WindowsConsole.h"

#ifdef USE_WINDOWSCONSOLE

#define WINDOWS_CONSOLE_WIDTH               128
#define WINDOWS_CONSOLE_HEIGHT              50
#define WINDOWS_CONSOLE_LOG_BUFFER_LINES    1024
#define WINDOWS_CONSOLE_LOG_SCROLL_LINES    10
#define WINDOWS_CONSOLE_TAB_SIZE            4
#define WINDOWS_CONSOLE_CRYENGINE_BLACK     0x0
#define WINDOWS_CONSOLE_CRYENGINE_WHITE     0x1
#define WINDOWS_CONSOLE_CRYENGINE_BLUE      0x2
#define WINDOWS_CONSOLE_CRYENGINE_GREEN     0x3
#define WINDOWS_CONSOLE_CRYENGINE_RED       0x4
#define WINDOWS_CONSOLE_CRYENGINE_CYAN      0x5
#define WINDOWS_CONSOLE_CRYENGINE_YELLOW    0x6
#define WINDOWS_CONSOLE_CRYENGINE_MAGENTA   0x7
#define WINDOWS_CONSOLE_CRYENGINE_ORANGE    0x8
#define WINDOWS_CONSOLE_CRYENGINE_GREY      0x9
#define WINDOWS_CONSOLE_NATIVE_BLACK        0x0
#define WINDOWS_CONSOLE_NATIVE_BROWN        0x6
#define WINDOWS_CONSOLE_NATIVE_LIGHTGREY    0x7
#define WINDOWS_CONSOLE_NATIVE_LIGHTBLUE    0x9
#define WINDOWS_CONSOLE_NATIVE_LIGHTGREEN   0xA
#define WINDOWS_CONSOLE_NATIVE_LIGHTCYAN    0xB
#define WINDOWS_CONSOLE_NATIVE_LIGHTRED     0xC
#define WINDOWS_CONSOLE_NATIVE_LIGHTMAGENTA 0xD
#define WINDOWS_CONSOLE_NATIVE_YELLOW       0xE
#define WINDOWS_CONSOLE_NATIVE_WHITE        0xF
#define WINDOWS_CONSOLE_COLOR_MASK          0xF
#define WINDOWS_CONSOLE_BGCOLOR_SHIFT       4

const uint8 CWindowsConsole::s_colorTable[ WINDOWS_CONSOLE_NUM_CRYENGINE_COLORS ] =
{
    WINDOWS_CONSOLE_NATIVE_BLACK,
    WINDOWS_CONSOLE_NATIVE_WHITE,
    WINDOWS_CONSOLE_NATIVE_LIGHTBLUE,
    WINDOWS_CONSOLE_NATIVE_LIGHTGREEN,
    WINDOWS_CONSOLE_NATIVE_LIGHTRED,
    WINDOWS_CONSOLE_NATIVE_LIGHTCYAN,
    WINDOWS_CONSOLE_NATIVE_YELLOW,
    WINDOWS_CONSOLE_NATIVE_LIGHTMAGENTA,
    WINDOWS_CONSOLE_NATIVE_BROWN,
    WINDOWS_CONSOLE_NATIVE_LIGHTGREY
};

CWindowsConsole::CWindowsConsole()
    : m_lock()
    , m_consoleScreenBufferSize()
    , m_consoleWindow()
    , m_inputBufferHandle(INVALID_HANDLE_VALUE)
    , m_screenBufferHandle(INVALID_HANDLE_VALUE)
    , m_logBuffer(0, 0, WINDOWS_CONSOLE_WIDTH, WINDOWS_CONSOLE_HEIGHT - 2, WINDOWS_CONSOLE_LOG_BUFFER_LINES, L' ', WINDOWS_CONSOLE_CRYENGINE_GREY, WINDOWS_CONSOLE_CRYENGINE_BLACK)
    , m_fullScreenBuffer(0, 0, WINDOWS_CONSOLE_WIDTH, WINDOWS_CONSOLE_HEIGHT - 2, WINDOWS_CONSOLE_HEIGHT - 2, L' ', WINDOWS_CONSOLE_CRYENGINE_GREY, WINDOWS_CONSOLE_CRYENGINE_BLACK)
    , m_statusBuffer(0, WINDOWS_CONSOLE_HEIGHT - 2, WINDOWS_CONSOLE_WIDTH, 1, 1, L' ', WINDOWS_CONSOLE_CRYENGINE_BLACK, WINDOWS_CONSOLE_CRYENGINE_GREY)
    , m_commandBuffer(0, WINDOWS_CONSOLE_HEIGHT - 1, WINDOWS_CONSOLE_WIDTH, 1, 1, L' ', WINDOWS_CONSOLE_CRYENGINE_WHITE, WINDOWS_CONSOLE_CRYENGINE_BLACK)
    , m_dirtyCellBuffers(0)
    , m_commandQueue()
    , m_commandPrompt("] ")
    , m_commandPromptLength(m_commandPrompt.length())
    , m_command()
    , m_commandCursor(0)
    , m_logLine()
    , m_progressString()
    , m_header()
    , m_updStats()
    , m_pInputThread(NULL)
    , m_pSystem(NULL)
    , m_pConsole(NULL)
    , m_pTimer(NULL)
    , m_pCVarSvMap(NULL)
    , m_pCVarSvMission(NULL)
    , m_pCVarSvGameRules(NULL)
    , m_lastStatusUpdate()
    , m_lastUpdateTime()
    , m_initialized(false)
    , m_OnUpdateCalled(false)
    , m_requireDedicatedServer(false)
{
}

CWindowsConsole::~CWindowsConsole()
{
    CleanUp();
}

Vec2_tpl< int > CWindowsConsole::BeginDraw()
{
    m_newCmds.resize(0);
    return Vec2_tpl< int >(WINDOWS_CONSOLE_WIDTH, WINDOWS_CONSOLE_HEIGHT - 2);
}

void CWindowsConsole::PutText(int x, int y, const char* pMsg)
{
    SConDrawCmd             cmd;

    cmd.x = x;
    cmd.y = y;
    cry_strcpy(cmd.text, pMsg);
    m_newCmds.push_back(cmd);
}

void CWindowsConsole::EndDraw()
{
    Lock();
    m_drawCmds.swap(m_newCmds);
    Unlock();
}

void CWindowsConsole::SetTitle(const char* title)
{
    m_title = title;

    if (m_title.empty())
    {
        SetConsoleTitle(m_header.c_str());
    }
    else
    {
        stack_string fullHeader = m_title + " - " + m_header;
        SetConsoleTitle(fullHeader);
    }
}

void CWindowsConsole::Print(const char* pInszText)
{
    Lock();

    bool                    isContinue = true;
    const char*             pInszTextPtr = pInszText;
    const char*             pLogLinePtr = m_logLine.c_str();

    while (*pLogLinePtr && isContinue)
    {
        if (*pInszTextPtr != *pLogLinePtr)
        {
            isContinue = false;
        }

        ++pInszTextPtr;
        ++pLogLinePtr;
    }

    // Do not treat lines as equal if the new line starts the same as the previous line
    isContinue = isContinue && (*pInszTextPtr == 0 || m_logLine.empty());

    if (!isContinue)
    {
        pInszTextPtr = pInszText;
        m_logBuffer.NewLine();
        m_logLine.clear();
    }

    m_logLine.append(pInszTextPtr);
    m_logBuffer.Print(pInszTextPtr);
    m_dirtyCellBuffers |= eCBB_Log;

    Unlock();
}

bool CWindowsConsole::OnError([[maybe_unused]] const char* szErrorString)
{
    return true;
}

bool CWindowsConsole::OnSaveDocument()
{
    return false;
}

bool CWindowsConsole::OnBackupDocument()
{
    return false;
}

void CWindowsConsole::OnProcessSwitch()
{
}

void CWindowsConsole::OnInitProgress(const char* sProgressMsg)
{
    if (m_initialized)
    {
        Lock();
        m_progressString = sProgressMsg;
        DrawStatus();
        Unlock();
    }
}

// the CtrlHandler will be called from a separate thread that only handles
// Ctrl messages. When the CLOSE event is sent, this function can just wait
// forever, as FreeConsole() will kill the thread. If this function returns
// immediately, windows will call TerminateProcess() and nothing will be cleaned up.
static BOOL WINAPI CtrlHandler(DWORD ctrlEvent)
{
    switch (ctrlEvent)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        return TRUE;
    case CTRL_CLOSE_EVENT:
        if ( (gEnv != nullptr) && (gEnv->pSystem != nullptr) && (gEnv->pSystem->GetIConsole() != nullptr) )
        {
            gEnv->pSystem->GetIConsole()->ExecuteString("quit", true, true);
            Sleep(INFINITE);
            return TRUE;
        }
    default:
        break;
    }
    return FALSE;
}

void CWindowsConsole::OnInit(ISystem* pSystem)
{
    if (m_requireDedicatedServer && !gEnv->IsDedicated())
    {
        return;
    }

    Lock();

    if (!m_initialized)
    {
        assert(m_pSystem == NULL);
        assert(m_pConsole == NULL);

        m_pSystem = pSystem;
        m_pConsole = pSystem->GetIConsole();

        AllocConsole();
        m_inputBufferHandle = GetStdHandle(STD_INPUT_HANDLE);
        m_screenBufferHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(m_inputBufferHandle, ENABLE_WINDOW_INPUT);
        m_consoleScreenBufferSize.X = WINDOWS_CONSOLE_WIDTH;
        m_consoleScreenBufferSize.Y = WINDOWS_CONSOLE_HEIGHT;
        m_consoleWindow.Left = 0;
        m_consoleWindow.Top = 0;
        m_consoleWindow.Right = WINDOWS_CONSOLE_WIDTH - 1;
        m_consoleWindow.Bottom = WINDOWS_CONSOLE_HEIGHT - 1;
        SetConsoleScreenBufferSize(m_screenBufferHandle, m_consoleScreenBufferSize);
        SetConsoleWindowInfo(m_screenBufferHandle, TRUE, &m_consoleWindow);
        SetConsoleTitle(m_header.c_str());

        if (m_pConsole)
        {
            m_pConsole->AddOutputPrintSink(this);
        }

        DrawCommand();

        m_pInputThread = new CWindowsConsoleInputThread(*this);
        m_pInputThread->Start();

#if !defined(NDEBUG)
        BOOL handlerInstalled =
#endif
            SetConsoleCtrlHandler(CtrlHandler, TRUE);
        CRY_ASSERT(handlerInstalled);

        m_initialized = true;
    }

    Unlock();
}

void CWindowsConsole::OnShutdown()
{
    CleanUp();
}

void CWindowsConsole::OnUpdate()
{
    if (m_initialized)
    {
        Lock();

        bool updateStatus = false;

        if (!m_OnUpdateCalled)
        {
            assert(m_pCVarSvMap == NULL);
            assert(m_pCVarSvGameRules == NULL);
            assert(m_pTimer == NULL);

            m_pCVarSvMap = m_pConsole->GetCVar("sv_map");
            m_pCVarSvGameRules = m_pConsole->GetCVar("sv_gamerules");
            m_pTimer = m_pSystem->GetITimer();
            m_OnUpdateCalled = true;

            assert(m_pCVarSvMission == NULL);
            m_pCVarSvMission = m_pConsole->GetCVar("sv_mission");
        }

        if (!m_progressString.empty())
        {
            m_progressString.clear();
            updateStatus = true;
        }

        CTimeValue              now = m_pTimer->GetAsyncTime();

        if ((now - m_lastStatusUpdate).GetSeconds() > 0.1F)
        {
            updateStatus = true;
        }

        m_lastUpdateTime = now;

        m_pSystem->GetUpdateStats(m_updStats);

        if (updateStatus)
        {
            DrawStatus();
            m_lastStatusUpdate = now;
        }

        while (m_commandQueue.size())
        {
            const string& command = m_commandQueue[0];
            Unlock();
            // 'm_pConsole' will be set to NULL when executing 'quit' command
            // Cache pointer in local variable to prevent crash when adding the last command to the history
            IConsole* pConsole = m_pConsole;
            pConsole->ExecuteString(command.c_str());
            pConsole->AddCommandToHistory(command.c_str());
            Lock();
            m_commandQueue.pop_front();
        }

        if (!m_drawCmds.empty())
        {
            DrawFull();
        }

        Repaint();

        Unlock();
    }
}

void CWindowsConsole::OnConsoleInputEvent(INPUT_RECORD inputRecord)
{
    switch (inputRecord.EventType)
    {
    case KEY_EVENT:
        OnKey(inputRecord.Event.KeyEvent);
        break;
    case WINDOW_BUFFER_SIZE_EVENT:
        OnResize(inputRecord.Event.WindowBufferSizeEvent.dwSize);
        break;
    }
}

void CWindowsConsole::OnKey(const KEY_EVENT_RECORD& event)
{
    if (event.bKeyDown)
    {
        for (uint32 i = 0; i < event.wRepeatCount; ++i)
        {
            switch (event.wVirtualKeyCode)
            {
            case VK_BACK:
                OnBackspace();
                break;
            case VK_TAB:
                OnTab();
                break;
            case VK_RETURN:
                OnReturn();
                break;
            case VK_PRIOR:
                OnPgUp();
                break;
            case VK_NEXT:
                OnPgDn();
                break;
            case VK_LEFT:
                OnLeft();
                break;
            case VK_UP:
                OnUp();
                break;
            case VK_RIGHT:
                OnRight();
                break;
            case VK_DOWN:
                OnDown();
                break;
            case VK_DELETE:
                OnDelete();
                break;
            default:
                OnChar(event.uChar.AsciiChar);
                break;
            }
        }
    }
}

void CWindowsConsole::OnResize(const COORD& size)
{
    if ((size.X != m_consoleScreenBufferSize.X) || (size.Y != m_consoleScreenBufferSize.Y))
    {
        SetConsoleScreenBufferSize(m_screenBufferHandle, m_consoleScreenBufferSize);
        SetConsoleWindowInfo(m_screenBufferHandle, TRUE, &m_consoleWindow);
    }
}

void CWindowsConsole::OnBackspace()
{
    if (m_commandCursor > 0)
    {
        m_command.erase(--m_commandCursor, 1);
        m_pConsole->ResetAutoCompletion();
        DrawCommand();
    }
}

void CWindowsConsole::OnTab()
{
    const char*             pCompletion;

    pCompletion = m_pConsole->ProcessCompletion(m_command.c_str());

    if (pCompletion)
    {
        m_command = pCompletion;
        m_commandCursor = m_command.length();
        DrawCommand();
    }
}

void CWindowsConsole::OnReturn()
{
    m_commandQueue.push_back(m_command);
    m_command.clear();
    m_pConsole->ResetAutoCompletion();
    m_commandCursor = 0;
    DrawCommand();
}

void CWindowsConsole::OnPgUp()
{
    if (m_logBuffer.Scroll(-WINDOWS_CONSOLE_LOG_SCROLL_LINES))
    {
        m_dirtyCellBuffers |= eCBB_Log;
    }
}

void CWindowsConsole::OnPgDn()
{
    if (m_logBuffer.Scroll(WINDOWS_CONSOLE_LOG_SCROLL_LINES))
    {
        m_dirtyCellBuffers |= eCBB_Log;
    }
}

void CWindowsConsole::OnLeft()
{
    if (m_commandCursor > 0)
    {
        --m_commandCursor;
        m_commandBuffer.SetCursor(m_screenBufferHandle, m_commandCursor + m_commandPromptLength);
    }
}

void CWindowsConsole::OnUp()
{
    OnHistory(m_pConsole->GetHistoryElement(true));
}

void CWindowsConsole::OnRight()
{
    if (m_commandCursor < m_command.length())
    {
        ++m_commandCursor;
        m_commandBuffer.SetCursor(m_screenBufferHandle, m_commandCursor + m_commandPromptLength);
    }
}

void CWindowsConsole::OnDown()
{
    OnHistory(m_pConsole->GetHistoryElement(false));
}

void CWindowsConsole::OnDelete()
{
    if (m_commandCursor < m_command.length())
    {
        m_command.erase(m_commandCursor, 1);
        m_pConsole->ResetAutoCompletion();
        DrawCommand();
    }
}

void CWindowsConsole::OnChar(CHAR ch)
{
    if ((ch >= ' ') && (ch <= '~'))
    {
        m_command.insert(m_commandCursor++, ch);
        m_pConsole->ResetAutoCompletion();
        DrawCommand();
    }
}

void CWindowsConsole::OnHistory(const char* pHistoryElement)
{
    if (pHistoryElement)
    {
        m_command = pHistoryElement;
    }
    else
    {
        m_command.clear();
    }

    m_commandCursor = m_command.length();
    DrawCommand();
}

void CWindowsConsole::DrawCommand()
{
    m_commandBuffer.Clear();
    m_commandBuffer.PutText(0, 0, m_commandPrompt.c_str());
    m_commandBuffer.PutText(m_commandPromptLength, 0, m_command);
    m_commandBuffer.SetCursor(m_screenBufferHandle, m_commandCursor + m_commandPromptLength);
    m_dirtyCellBuffers |= eCBB_Command;
}

void CWindowsConsole::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->Add(this);
    pSizer->Add(m_command);
    pSizer->Add(m_logLine);
    pSizer->Add(m_pInputThread);
    m_logBuffer.GetMemoryUsage(pSizer);
    m_fullScreenBuffer.GetMemoryUsage(pSizer);
    m_statusBuffer.GetMemoryUsage(pSizer);
    m_commandBuffer.GetMemoryUsage(pSizer);
}

void CWindowsConsole::SetRequireDedicatedServer(bool value)
{
    m_requireDedicatedServer = value;
}

void CWindowsConsole::SetHeader(const char* pHeader)
{
    m_header = pHeader;
    SetConsoleTitle(pHeader);
}

void CWindowsConsole::InputIdle()
{
    if (m_pTimer)
    {
        CTimeValue              now = m_pTimer->GetAsyncTime();
        float                   timePassed = (now - m_lastUpdateTime).GetSeconds();

        if (timePassed > 0.2F)
        {
            int                     nDots = ( int )(timePassed + 0.5) / 3;
            int                     nDotsMax = m_statusBuffer.Width() - 2;

            if (nDots > nDotsMax)
            {
                nDots = nDotsMax;
            }

            if (m_progressString.length() != nDots)
            {
                m_progressString.clear();
                m_progressString.append(nDots, '.');
                DrawStatus();
            }
        }
    }

    Repaint();
}

void CWindowsConsole::Lock()
{
    m_lock.Lock();
}

void CWindowsConsole::Unlock()
{
    m_lock.Unlock();
}

bool CWindowsConsole::TryLock()
{
    return m_lock.TryLock();
}

void CWindowsConsole::Repaint()
{
    if (m_dirtyCellBuffers)
    {
        if (m_dirtyCellBuffers & eCBB_Full)
        {
            m_fullScreenBuffer.Blit(m_screenBufferHandle);
            m_dirtyCellBuffers &= ~eCBB_Full;
        }
        else if (m_dirtyCellBuffers & eCBB_Log)
        {
            m_logBuffer.Blit(m_screenBufferHandle);
            m_dirtyCellBuffers &= ~eCBB_Log;
        }

        if (m_dirtyCellBuffers & eCBB_Status)
        {
            m_statusBuffer.Blit(m_screenBufferHandle);
            m_dirtyCellBuffers &= ~eCBB_Status;
        }

        if (m_dirtyCellBuffers & eCBB_Command)
        {
            m_commandBuffer.Blit(m_screenBufferHandle);
            m_dirtyCellBuffers &= ~eCBB_Command;
        }
    }
}

void CWindowsConsole::DrawStatus()
{
    const char*             pStatusLeft = NULL;
    const char*             pStatusRight = NULL;
    char                    bufferLeft[ 256 ];
    char                    bufferRight[ 256 ];

    // If we're scrolled, then the right size shows a scroll indicator.
    if (m_logBuffer.IsScrolledUp())
    {
        m_logBuffer.FmtScrollStatus(sizeof bufferRight, bufferRight);
        bufferRight[ sizeof bufferRight - 1 ] = 0;
        pStatusRight = bufferRight;
    }

    if (!m_progressString.empty())
    {
        azsnprintf(bufferLeft, sizeof bufferLeft, " %s", m_progressString.c_str());
        bufferLeft [sizeof bufferLeft - 1 ] = 0;
        pStatusLeft = bufferLeft;
    }
    else if (m_OnUpdateCalled)
    {
        // Standard status display.
        // Map name and game rules on the left.
        // Current update rate and player count on the right.

        const char*             pMapName = m_pCVarSvMap->GetString();

        const char*             pMissionName = m_pCVarSvMission ? m_pCVarSvMission->GetString() : "";
        azsnprintf(bufferLeft, sizeof bufferLeft, " mission: %s map:%s", pMissionName, pMapName);

        bufferLeft[ sizeof bufferLeft - 1 ] = 0;
        pStatusLeft = bufferLeft;

        if (!pStatusRight)
        {
            float                   updateRate = 0.f;

            if (m_pTimer != NULL)
            {
                updateRate = m_pTimer->GetFrameRate();
            }
            else
            {
                updateRate = 0.f;
            }

            char*                   pBufferRight = bufferRight;
            char* const             pBufferRightEnd = bufferRight + sizeof bufferRight;

            azstrcpy(pBufferRight, AZ_ARRAY_SIZE(bufferRight), "| ");
            pBufferRight += strlen(pBufferRight);

            if (pBufferRight < pBufferRightEnd)
            {
                if (m_pConsole != NULL)
                {
                    pBufferRight += azsnprintf(
                            pBufferRight,
                            pBufferRightEnd - pBufferRight,
                            "upd:%.1fms(%.2f..%.2f) " \
                            "rate:%.1f/s",
                            m_updStats.avgUpdateTime, m_updStats.minUpdateTime, m_updStats.maxUpdateTime,
                            updateRate);
                }
                else
                {
                    cry_strcpy(pBufferRight, pBufferRightEnd - pBufferRight, "BUSY ");
                }
            }

            bufferRight[ sizeof bufferRight - 1 ] = 0;
            pStatusRight = bufferRight;
        }
    }

    if (pStatusLeft == NULL)
    {
        pStatusLeft = "";
    }

    if (pStatusRight == NULL)
    {
        pStatusRight = "";
    }

    int                     rightWidth = strlen(pStatusRight);

    m_statusBuffer.Clear();
    m_statusBuffer.PutText(0, 0, pStatusLeft);
    m_statusBuffer.PutText(-rightWidth, 0, pStatusRight);
    m_dirtyCellBuffers |= eCBB_Status;
}

void CWindowsConsole::CleanUp()
{
    Lock();

    if (m_initialized)
    {
        if (m_pInputThread)
        {
            m_pInputThread->Cancel();

            // The input thread may continue to lock before it gets our cancel event, so
            // we need to release the lock until we confirm that it has canceled its operations.
            Unlock();
            m_pInputThread->WaitForThread();
            Lock();

            delete m_pInputThread;
            m_pInputThread = NULL;
        }

        if (m_pConsole)
        {
            m_pConsole->RemoveOutputPrintSink(this);
        }

        m_pSystem = NULL;
        m_pConsole = NULL;
        m_pTimer = NULL;
        m_pCVarSvMap = NULL;
        m_pCVarSvGameRules = NULL;
        m_inputBufferHandle = INVALID_HANDLE_VALUE;
        m_screenBufferHandle = INVALID_HANDLE_VALUE;
        m_initialized = false;
    }

    Unlock();
}

void CWindowsConsole::DrawFull()
{
    for (DynArray< SConDrawCmd >::iterator iter = m_drawCmds.begin(); iter != m_drawCmds.end(); ++iter)
    {
        m_fullScreenBuffer.PutText(iter->x, iter->y, iter->text);
    }

    m_dirtyCellBuffers |= eCBB_Full;
}

CWindowsConsole::CCellBuffer::CCellBuffer(SHORT x, short y, SHORT w, SHORT h, SHORT lines, WCHAR emptyChar, uint8 defaultFgColor, uint8 defaultBgColor)
{
    m_emptyCell.Char.UnicodeChar = emptyChar;
    m_emptyCell.Attributes = s_colorTable[ defaultFgColor ] | (s_colorTable[ defaultBgColor ] << WINDOWS_CONSOLE_BGCOLOR_SHIFT);
    m_attr = m_emptyCell.Attributes;
    m_size.X = w;
    m_size.Y = lines;
    m_screenArea.Left = x;
    m_screenArea.Top = y;
    m_screenArea.Right = x + w - 1;
    m_screenArea.Bottom = y + h - 1;
    m_position.head = 0;
    m_position.lines = 1;
    m_position.wrap = 0;
    m_position.offset = 0;
    m_position.scroll = 0;
    m_escape = false;
    m_color = false;
    m_buffer.resize(w * lines, m_emptyCell);
}

CWindowsConsole::CCellBuffer::~CCellBuffer()
{
}

void CWindowsConsole::CCellBuffer::PutText(int x, int y, const char* pMsg)
{
    SPosition               position;

    position.head = m_position.head;
    position.offset = x;
    position.lines = y;
    position.scroll = 0;
    position.wrap = 0;

    if (position.offset < 0)
    {
        position.offset += m_screenArea.Right - 1;
    }

    if (position.lines < 0)
    {
        position.lines += m_screenArea.Bottom - 1;
    }

    Print(pMsg, position);
}

void CWindowsConsole::CCellBuffer::Print(const char* pInszText)
{
    Print(pInszText, m_position);
}

void CWindowsConsole::CCellBuffer::ClearCells(TBuffer::iterator pDst, TBuffer::iterator pDstEnd)
{
    std::fill(pDst, pDstEnd, m_emptyCell);
}

bool CWindowsConsole::CCellBuffer::Scroll(SHORT numLines)
{
    bool                    result = false;
    SHORT                   newScroll = m_position.scroll + numLines;
    SHORT                   maxScroll = m_position.lines - 1 - (m_screenArea.Bottom - m_screenArea.Top);

    if (newScroll > maxScroll)
    {
        newScroll = maxScroll;
    }

    if (newScroll < 0)
    {
        newScroll = 0;
    }

    if (newScroll != m_position.scroll)
    {
        m_position.scroll = newScroll;
        result = true;
    }

    return result;
}

void CWindowsConsole::CCellBuffer::SetCursor(HANDLE hScreenBuffer, SHORT offset)
{
    COORD                   position;

    position.X = m_screenArea.Left + offset;
    position.Y = m_screenArea.Top;
    SetConsoleCursorPosition(hScreenBuffer, position);
}

void CWindowsConsole::CCellBuffer::AddCharacter(WCHAR ch, SPosition& position)
{
    if (position.offset == m_size.X)
    {
        WrapLine(position);
    }

    int32 index = (((position.head + position.lines + m_size.Y - 1) % m_size.Y) * m_size.X) + position.offset;

    CHAR_INFO&              info = m_buffer[ index ];

    info.Attributes = m_attr;
    info.Char.UnicodeChar = ch;
    ++position.offset;
}

void CWindowsConsole::CCellBuffer::WrapLine(SPosition& position)
{
    ++position.wrap;
    AdvanceLine(position);
}

void CWindowsConsole::CCellBuffer::NewLine()
{
    NewLine(m_position);
}

void CWindowsConsole::CCellBuffer::NewLine(SPosition& position)
{
    m_attr = m_emptyCell.Attributes;
    position.wrap = 0;
    AdvanceLine(position);
}

void CWindowsConsole::CCellBuffer::ClearLine(SPosition& position)
{
    ClearCells(m_buffer.begin() + ((position.head + position.lines - position.wrap) % m_size.Y) * m_size.X, m_buffer.begin() + ((position.head + position.lines + 1) % m_size.Y) * m_size.X);
    position.lines -= position.wrap;
    position.wrap = 0;
    position.offset = 0;
}

void CWindowsConsole::CCellBuffer::Tab(SPosition& position)
{
    do
    {
        AddCharacter(' ', position);
    } while (position.offset % WINDOWS_CONSOLE_TAB_SIZE);
}

void CWindowsConsole::CCellBuffer::Blit(HANDLE hScreenBuffer)
{
    COORD                   src;
    SMALL_RECT              dst;

    src.X = 0;
    src.Y = (m_position.head + m_position.scroll) % m_size.Y;
    dst = m_screenArea;
    WriteConsoleOutput(hScreenBuffer, &*m_buffer.begin(), m_size, src, &dst);

    if ((m_size.Y - src.Y) < (m_screenArea.Bottom - m_screenArea.Top + 1))
    {
        src.Y = 0;
        dst.Top = dst.Bottom + 1;
        dst.Bottom = m_screenArea.Bottom;
        WriteConsoleOutput(hScreenBuffer, &*m_buffer.begin(), m_size, src, &dst);
    }
}

void CWindowsConsole::CCellBuffer::AdvanceLine(SPosition& position)
{
    position.offset = 0;

    if (position.lines == m_size.Y)
    {
        position.head = (position.head + 1) % m_size.Y;
    }
    else
    {
        ++position.lines;

        if (position.lines > m_screenArea.Bottom - m_screenArea.Top + 1)
        {
            ++position.scroll;
        }
    }

    TBuffer::iterator       start = m_buffer.begin() + ((position.head + position.lines + m_size.Y - 1) % m_size.Y) * m_size.X;
    TBuffer::iterator       end = start + m_size.X;

    ClearCells(start, end);
}

void CWindowsConsole::CCellBuffer::SetFgColor(WORD color)
{
    m_attr = (m_attr & ~WINDOWS_CONSOLE_COLOR_MASK) | s_colorTable[ color ];
}

void CWindowsConsole::CCellBuffer::Print(const char* pInszText, SPosition& position)
{
    while (*pInszText)
    {
        switch (*pInszText)
        {
        case '$':
            if (!m_escape)
            {
                m_color = true;
                break;
            }
        case '\\':
            m_escape = !m_escape;
            if (m_escape)
            {
                break;
            }
        case 'n':
            if (m_escape)
            {
            case '\n':
                NewLine(position);
                m_escape = false;
                break;
            }
        case 'r':
            if (m_escape)
            {
            case '\r':
                ClearLine(position);
                m_escape = false;
                break;
            }
        case 't':
            if (m_escape)
            {
            case '\t':
                Tab(position);
                m_escape = false;
                break;
            }
        default:
            if (m_color)
            {
                if (isdigit(*pInszText))
                {
                    SetFgColor(*pInszText - '0');
                }

                m_color = false;
            }
            else
            {
                if (m_escape && (*pInszText != '\\'))
                {
                    AddCharacter('\\', position);
                }
                AddCharacter(*pInszText, position);
            }
            m_escape = false;
            break;
        }

        ++pInszText;
    }
}

void CWindowsConsole::CCellBuffer::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->Add(m_buffer);
}

bool CWindowsConsole::CCellBuffer::IsScrolledUp()
{
    return (m_position.lines - m_position.scroll) > (m_screenArea.Bottom - m_screenArea.Top + 1);
}

void CWindowsConsole::CCellBuffer::FmtScrollStatus(uint32 size, char* pBuffer)
{
    if (m_position.scroll)
    {
        azsnprintf(pBuffer, size, "| SCROLL: %.1f%%", 100.0F * static_cast< float >(m_position.scroll) / static_cast< float >(m_position.lines - (m_screenArea.Bottom - m_screenArea.Top + 1)));
    }
    else
    {
        cry_strcpy(pBuffer, size, "| SCROLL:TOP ");
    }
}

void CWindowsConsole::CCellBuffer::Clear()
{
    ClearCells(m_buffer.begin(), m_buffer.end());
}

SHORT CWindowsConsole::CCellBuffer::Width()
{
    return m_screenArea.Right - m_screenArea.Left + 1;
}


CWindowsConsoleInputThread::CWindowsConsoleInputThread(CWindowsConsole& console)
    :   m_WindowsConsole(console)
{
    m_handles[ eWH_Event ] = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_handles[ eWH_Console ] = m_WindowsConsole.m_inputBufferHandle;
}

CWindowsConsoleInputThread::~CWindowsConsoleInputThread()
{
    CloseHandle(m_handles[ eWH_Event ]);
}

void CWindowsConsoleInputThread::Run()
{
    bool                    cancelled = false;

    do
    {
        DWORD                   inputRecordCount = 0;
        DWORD                   waitResult;

        waitResult = WaitForMultipleObjects(eWH_NumWaitHandles, m_handles, FALSE, 100);

        switch (waitResult)
        {
        case WAIT_OBJECT_0 + eWH_Event:

            cancelled = true;
            break;

        case WAIT_OBJECT_0 + eWH_Console:

            ReadConsoleInput(m_WindowsConsole.m_inputBufferHandle, m_inputRecords, WINDOWS_CONSOLE_MAX_INPUT_RECORDS, &inputRecordCount);

        // FALL THROUGH

        case WAIT_TIMEOUT:

            if (inputRecordCount || (m_WindowsConsole.m_dirtyCellBuffers && !m_WindowsConsole.m_OnUpdateCalled))
            {
                m_WindowsConsole.Lock();

                if (inputRecordCount)
                {
                    PINPUT_RECORD           pInputRecordEnd = m_inputRecords + inputRecordCount;

                    for (PINPUT_RECORD pInputRecord = m_inputRecords; pInputRecord < pInputRecordEnd; ++pInputRecord)
                    {
                        m_WindowsConsole.OnConsoleInputEvent(*pInputRecord);
                    }

                    m_WindowsConsole.DrawCommand();
                }
                else
                {
                    m_WindowsConsole.InputIdle();
                }

                m_WindowsConsole.Unlock();
            }

            break;
        }
    } while (!cancelled);
}

void CWindowsConsoleInputThread::Cancel()
{
    SetEvent(m_handles[ eWH_Event ]);
}

#endif // def USE_WINDOWSCONSOLE
