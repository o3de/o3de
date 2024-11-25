/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorSyntaxHighlighter.hxx"
#include <Source/LUA/moc_LUAEditorSyntaxHighlighter.cpp>
#include "LUAEditorStyleMessages.h"
#include "LUAEditorBlockState.h"

namespace LUAEditor
{
    namespace
    {
        template<typename Container>
        void CreateTypes([[maybe_unused]] Container& container)
        {
        }

        template<typename Container, typename Type, typename ... Types>
        void CreateTypes(Container& container)
        {
            container.push_back(azcreate(Type, ()));
            CreateTypes<Container, Types...>(container);
        }

        enum class ParserStates : int
        {
            Null,
            Name,
            ShortComment,
            LongComment,
            Number,
            NumberHex,
            StringLiteral,
            NumStates
        };

        class BaseParserState
        {
        public:
            virtual ~BaseParserState() {}
            virtual bool IsMultilineState([[maybe_unused]] LUASyntaxHighlighter::StateMachine& machine) const { return false; }
            virtual void StartState([[maybe_unused]] LUASyntaxHighlighter::StateMachine& machine) {}
            //note you only get 13 bits of usable space here. see QTBlockState m_syntaxHighlighterStateExtra
            virtual AZ::u16 GetSaveState() const { return 0; }
            virtual void SetSaveState([[maybe_unused]] AZ::u16 state) {}
            virtual void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) = 0;
        };

        class NullParserState
            : public BaseParserState
        {
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
        };

        class NameParserState
            : public BaseParserState
        {
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
        };

        class ShortCommentParserState
            : public BaseParserState
        {
            void StartState(LUASyntaxHighlighter::StateMachine& machine) override;
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
            bool m_mightBeLong;
        };

        class NumberParserState
            : public BaseParserState
        {
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
        };

        class NumberHexParserState
            : public BaseParserState
        {
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
        };

        class LongCommentParserState
            : public BaseParserState
        {
            bool IsMultilineState([[maybe_unused]] LUASyntaxHighlighter::StateMachine& machine) const override { return true; }
            void StartState(LUASyntaxHighlighter::StateMachine& machine) override;
            AZ::u16 GetSaveState() const override { return m_bracketLevel; }
            void SetSaveState(AZ::u16 state) override;
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
            AZ::u16 m_bracketLevel;
            QString m_bracketEnd;
            bool m_endNextChar;
        };

        class StringLiteralParserState
            : public BaseParserState
        {
            bool IsMultilineState(LUASyntaxHighlighter::StateMachine& machine) const override;
            void StartState(LUASyntaxHighlighter::StateMachine& machine) override;
            AZ::u16 GetSaveState() const override { return m_bracketLevel; }
            void SetSaveState(AZ::u16 state) override;
            void Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar) override;
            AZ::u16 m_bracketLevel; //if 0 string started with a ' if 1 started with a " if 2 or more started with an long braket of level m_BracketLevel-2
            bool m_endNextChar;
            QString m_bracketEnd;
            bool m_mightBeLong;
        };
    }

    // GCC triggers a subobject linkage warning if a symbol in an anonymous namespace in included in another file
    // When unity files are enabled in CMake, this file is included in a .cxx file which triggers this warning
    // The code is actually OK as this file is a translation unit and not a header file where this warning would normally be triggered
    AZ_PUSH_DISABLE_WARNING_GCC("-Wsubobject-linkage");
    class LUASyntaxHighlighter::StateMachine
    {
    AZ_POP_DISABLE_WARNING_GCC
    public:
        StateMachine();
        ~StateMachine();
        void Parse(const QString& text);
        //extraBack is if you want to include previous chars as part as the current string after the state change
        void SetState(ParserStates state, int extraBack = 0);
        void PassState(ParserStates state); //change state but keep data captured so far, use if we are in "wrong" state
        void Reset();

        int CurrentLength() const { return m_currentChar - m_start + 1; }
        QStringRef CurrentString() const { return QStringRef(m_currentString, m_start, CurrentLength()); }
        const QString* GetFullLine() const { return m_currentString; }

        QTBlockState GetSaveState() const;
        void SetSaveState(QTBlockState state);

        void IncFoldLevel()
        {
            ++m_foldLevel;
            if (m_onIncFoldLevel)
            {
                m_onIncFoldLevel(m_foldLevel);
            }
        }
        void DecFoldLevel()
        {
            if (m_foldLevel > 0)
            {
                --m_foldLevel;
            }
            if (m_onDecFoldLevel)
            {
                m_onDecFoldLevel(m_foldLevel);
            }
        }

        ParserStates GetCurrentParserState() const { return m_currentState; }

        bool IsJoiningNames() const { return m_joinNames; }
        void SetJoiningNames(bool joinNames) { m_joinNames = joinNames; }

        template<typename T>
        void SetOnIncFoldLevel(const T& callable) { m_onIncFoldLevel = callable; }
        template<typename T>
        void SetOnDecFoldLevel(const T& callable) { m_onDecFoldLevel = callable; }

        AZStd::function<void(ParserStates state, int position, int length)> CaptureToken;

    private:
        BaseParserState* GetCurrentState() const { return m_states[static_cast<int>(m_currentState)]; }

        using StatesListType = AZStd::fixed_vector<BaseParserState*, static_cast<int>(ParserStates::NumStates)>;
        StatesListType m_states;
        ParserStates m_currentState{ ParserStates::Null };
        int m_currentChar;
        int m_start;
        const QString* m_currentString;
        int m_foldLevel;

        bool m_joinNames{ true }; //consider names seperated by . and : as one for highlighting purposes

        AZStd::function<void(int)> m_onIncFoldLevel;
        AZStd::function<void(int)> m_onDecFoldLevel;
    };

    LUASyntaxHighlighter::StateMachine::StateMachine()
    {
        CreateTypes<StatesListType, NullParserState, NameParserState, ShortCommentParserState,
            LongCommentParserState, NumberParserState, NumberHexParserState, StringLiteralParserState>(m_states);
        Reset();
    }

    LUASyntaxHighlighter::StateMachine::~StateMachine()
    {
        AZStd::for_each(m_states.begin(), m_states.end(), [](BaseParserState* state) { azdestroy(state); });
    }

    void LUASyntaxHighlighter::StateMachine::Reset()
    {
        m_start = 0;
        m_currentChar = -1;
        m_currentState = ParserStates::Null;
        m_currentString = nullptr;
        m_foldLevel = 0;
    }

    void LUASyntaxHighlighter::StateMachine::Parse(const QString& text)
    {
        m_currentString = &text;
        for (m_currentChar = 0; m_currentChar != text.length(); ++m_currentChar)
        {
            GetCurrentState()->Parse(*this, text[m_currentChar]);
        }
        //we only highlight on line at most at a time. so if this is a multiline highlight, go ahead an highlight this line as part of that now.
        if (GetCurrentState()->IsMultilineState(*this))
        {
            if (CaptureToken && m_start != m_currentChar)
            {
                CaptureToken(m_currentState, m_start, m_currentChar - m_start);
            }
        }
        else
        {
            if (m_currentState != ParserStates::Null)
            {
                SetState(ParserStates::Null);
            }
            else
            {
                if (CaptureToken && m_start != m_currentChar)
                {
                    CaptureToken(m_currentState, m_start, m_currentChar - m_start);
                }
            }
        }
    }

    void LUASyntaxHighlighter::StateMachine::SetState(ParserStates state, int extraBack)
    {
        if (m_currentState != state)
        {
            int currentChar = m_currentChar - extraBack;

            if (CaptureToken && m_start < currentChar)
            {
                CaptureToken(m_currentState, m_start, currentChar - m_start);
            }
            m_currentState = state;
            m_start = currentChar;
            GetCurrentState()->StartState(*this);
            //if going back to null state, see if this char might be the start of a new capture.
            if (m_currentState == ParserStates::Null && m_start < m_currentString->length())
            {
                GetCurrentState()->Parse(*this, m_currentString->at(m_start));
            }
        }
    }

    void LUASyntaxHighlighter::StateMachine::PassState(ParserStates state)
    {
        m_currentState = state;
        GetCurrentState()->StartState(*this);
        //if going back to null state, see if this char might be the start of a new capture.
        if (m_currentState == ParserStates::Null && m_start < m_currentString->length())
        {
            GetCurrentState()->Parse(*this, m_currentString->at(m_currentChar));
        }
    }

    ///1st bit to detect uninitialized blocks, next 14 bits store folding depth, next 3 bits store state machine state, final 14 bits store state specific user data.
    QTBlockState LUASyntaxHighlighter::StateMachine::GetSaveState() const
    {
        QTBlockState result;
        result.m_blockState.m_uninitialized = 0;
        result.m_blockState.m_folded = 0;
        result.m_blockState.m_foldLevel = m_foldLevel;
        result.m_blockState.m_syntaxHighlighterState = static_cast<int>(m_currentState);
        result.m_blockState.m_syntaxHighlighterStateExtra = GetCurrentState()->GetSaveState();
        return result;
    }

    void LUASyntaxHighlighter::StateMachine::SetSaveState(QTBlockState state)
    {
        static_assert(static_cast<int>(ParserStates::NumStates) <= 8, "We are only using 3 bits for state in lua parser currently");

        Reset();
        if (!state.m_blockState.m_uninitialized)
        {
            m_currentState = static_cast<ParserStates>(state.m_blockState.m_syntaxHighlighterState);
            GetCurrentState()->SetSaveState(state.m_blockState.m_syntaxHighlighterStateExtra);
            m_foldLevel = state.m_blockState.m_foldLevel;
        }
    }

    void NullParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar)
    {
        if (nextChar.isLetter() || nextChar == '_')
        {
            machine.SetState(ParserStates::Name);
        }
        else if (nextChar.isNumber() || nextChar == '-' || nextChar == '+')
        {
            machine.SetState(ParserStates::Number);
        }
        else if (nextChar == '\'' || nextChar == '"' || nextChar == '[')
        {
            machine.SetState(ParserStates::StringLiteral);
        }
        else if (nextChar == '{')
        {
            machine.IncFoldLevel();
        }
        else if (nextChar == '}')
        {
            machine.DecFoldLevel();
        }
    }

    void NameParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar)
    {
        if (!nextChar.isLetterOrNumber() && nextChar != '_' && (!machine.IsJoiningNames() || (nextChar != '.' && nextChar != ':')))
        {
            machine.SetState(ParserStates::Null);
        }
    }

    void ShortCommentParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar)
    {
        if (machine.CurrentLength() == 3 && nextChar != '[')
        {
            m_mightBeLong = false;
        }
        else if (machine.CurrentLength() >= 4 && machine.CurrentLength() < USHRT_MAX && m_mightBeLong) //we cant catch longer than 16k level comments.
        {
            if (nextChar != '=' && nextChar != '[')
            {
                m_mightBeLong = false;
            }
            else if (nextChar == '[')
            {
                machine.PassState(ParserStates::LongComment);
            }
        }
    }

    void ShortCommentParserState::StartState([[maybe_unused]] LUASyntaxHighlighter::StateMachine& machine)
    {
        m_mightBeLong = true;
    }

    void LongCommentParserState::StartState(LUASyntaxHighlighter::StateMachine& machine)
    {
        auto token = machine.CurrentString();
        int start = token.indexOf('[');
        AZ_Assert(start != -1, "Shouldn't have been able to get to this state without opening long bracket");
        int finish = token.indexOf('[', start + 1);
        AZ_Assert(finish != -1 && finish != start, "Shouldn't have been able to get to this state without opening long bracket");
        m_bracketLevel = static_cast<AZ::u16>(finish - start - 1);

        m_bracketEnd = "]";
        for (int i = 0; i < m_bracketLevel; ++i)
        {
            m_bracketEnd.append('=');
        }
        m_bracketEnd.append(']');
        m_endNextChar = false;
    }

    void LongCommentParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, [[maybe_unused]] const QChar& nextChar)
    {
        if (m_endNextChar)
        {
            machine.SetState(ParserStates::Null);
            return;
        }

        QStringRef token = machine.CurrentString();
        if (token.endsWith(m_bracketEnd))
        {
            if (machine.GetFullLine()->size() >= token.size())
            {
                machine.SetState(ParserStates::Null, -1);
            }
            else
            {
                m_endNextChar = true;
            }
        }
    }

    void LongCommentParserState::SetSaveState(AZ::u16 state)
    {
        m_bracketLevel = state;
        m_bracketEnd = "]";
        for (int i = 0; i < m_bracketLevel; ++i)
        {
            m_bracketEnd.append('=');
        }
        m_bracketEnd.append(']');
        m_endNextChar = false;
    }

    void NumberParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar)
    {
        auto currentString = machine.CurrentString();
        if (currentString.endsWith("--"))
        {
            machine.SetState(ParserStates::ShortComment, 1);
            return;
        }

        auto lower = nextChar.toLower();

        if (lower == 'x')
        {
            machine.PassState(ParserStates::NumberHex);
        }
        else if (!(nextChar.isNumber() || nextChar == '.' || lower == 'e'))
        {
            if (currentString.length() == 2 && (currentString.at(0) == '+' || currentString.at(0) == '-'))
            {
                machine.PassState(ParserStates::Null);
            }
            else
            {
                machine.SetState(ParserStates::Null);
            }
        }
    }

    void NumberHexParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar)
    {
        auto lower = nextChar.toLower();

        if (!(nextChar.isNumber() || nextChar == '.' || lower == 'p'))
        {
            machine.SetState(ParserStates::Null);
        }
    }

    void StringLiteralParserState::StartState(LUASyntaxHighlighter::StateMachine& machine)
    {
        m_endNextChar = false;
        m_mightBeLong = false;
        auto currentString = machine.CurrentString();
        AZ_Assert(!currentString.isEmpty(), "Sting literal string shouldn't be empty")
        if (currentString.at(0) == '\'')
        {
            m_bracketLevel = 0;
        }
        if (currentString.at(0) == '"')
        {
            m_bracketLevel = 1;
        }
        if (currentString.at(0) == '[')
        {
            m_bracketLevel = 2;
            m_mightBeLong = true;
            m_bracketEnd = "]";
        }
    }

    void StringLiteralParserState::Parse(LUASyntaxHighlighter::StateMachine& machine, const QChar& nextChar)
    {
        if (m_endNextChar)
        {
            machine.SetState(ParserStates::Null);
            return;
        }

        if (m_mightBeLong)
        {
            if (nextChar == '[')
            {
                m_mightBeLong = false; //it actually is long, = length will already be in m_bracketLevel
                m_bracketEnd += ']';
                return;
            }
            else if (nextChar == '=')
            {
                ++m_bracketLevel;
                m_bracketEnd += '=';
            }
            else
            {
                machine.PassState(ParserStates::Null); //turns out we weren't actually a string literal
                return;
            }
        }

        if (m_bracketLevel == 0 && nextChar == '\'' && !machine.CurrentString().endsWith("\\'"))
        {
            m_endNextChar = true;
        }
        if (m_bracketLevel == 1 && nextChar == '"' && !machine.CurrentString().endsWith("\\\""))
        {
            m_endNextChar = true;
        }
        if (m_bracketLevel >= 2 && machine.CurrentString().endsWith(m_bracketEnd))
        {
            m_endNextChar = true;
        }
    }

    bool StringLiteralParserState::IsMultilineState(LUASyntaxHighlighter::StateMachine& machine) const
    {
        auto currentString = machine.GetFullLine();

        if ((m_bracketLevel > 1 && !m_endNextChar && !m_mightBeLong) || currentString->endsWith(QString("\\")) || currentString->endsWith(QString("\\z")))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void StringLiteralParserState::SetSaveState(AZ::u16 state)
    {
        m_bracketLevel = state;
        if (m_bracketLevel >= 2)
        {
            m_bracketEnd = "]";
            for (int i = 0; i < m_bracketLevel - 2; ++i)
            {
                m_bracketEnd.append('=');
            }
            m_bracketEnd.append(']');
        }
        m_endNextChar = false;
        m_mightBeLong = false;
    }

    LUASyntaxHighlighter::LUASyntaxHighlighter(QWidget* parent)
        : QSyntaxHighlighter(parent)
        , m_machine(azcreate(StateMachine, ()))
    {
        AddBlockKeywords();
        BuildRegExes();
    }

    LUASyntaxHighlighter::LUASyntaxHighlighter(QTextDocument* parent)
        : QSyntaxHighlighter(parent)
        , m_machine(azcreate(StateMachine, ()))
    {
        AddBlockKeywords();
        BuildRegExes();
    }

    void LUASyntaxHighlighter::BuildRegExes()
    {
        auto colors =
            AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        // Order of declaration matters as some rules can stop the next ones from being processed

        // Match against ; : , . = * - + / < >
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(R"([\;\:\,\.\=\*\-\+\/\<\>])");
            rule.colorCB = [colors]()
            {
                return colors->GetSpecialCharacterColor();
            };
            m_highlightingRules.push_back(rule);
        }

        // Match against parenthesis and brackets
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(R"([\(\)\{\}\[\]])");
            rule.colorCB = [colors]()
            {
                return colors->GetBracketColor();
            };
            m_highlightingRules.push_back(rule);
        }

        // Match methods and definitions. Any word which ends with '('
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(R"(\b[A-Za-z0-9_]+(?=\())");
            rule.colorCB = [colors]()
            {
                return colors->GetMethodColor();
            };
            m_highlightingRules.push_back(rule);
        }

        // Match any word which ends with ':'
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(R"(\b[A-Za-z0-9_]+(?=\:))");
            rule.colorCB = [colors]()
            {
                return colors->GetLibraryColor();
            };
            m_highlightingRules.push_back(rule);
        }

        // Match against local, self, true, false and nil keywords
        {
            HighlightingRule rule;
            rule.stopProcessingMoreRulesAfterThis = true;
            rule.pattern = QRegularExpression(R"(\bself\b|\blocal\b|\btrue\b|\bfalse\b|\bnil\b)");
            rule.colorCB = [colors]()
            {
                return colors->GetSpecialKeywordColor();
            };
            m_highlightingRules.push_back(rule);
        }

        // Match against reserved keywords such as function, then, if, etc
        const HighlightedWords::LUAKeywordsType* keywords = nullptr;
        HighlightedWords::Bus::BroadcastResult(keywords, &HighlightedWords::Bus::Events::GetLUAKeywords);
        if (keywords)
        {
            QString pattern;
            for (const AZStd::string& keyword : *keywords)
            {
                pattern += "\\b";
                pattern += keyword.c_str();
                pattern += "\\b|";
            }
            pattern.chop(1); // remove last |

            HighlightingRule rule;
            rule.stopProcessingMoreRulesAfterThis = true;
            rule.pattern = QRegularExpression(pattern);
            rule.colorCB = [colors]()
            {
                return colors->GetKeywordColor();
            };
            m_highlightingRules.push_back(rule);
        }
    }

    void LUASyntaxHighlighter::AddBlockKeywords()
    {
        //these don't catch tables. that is handled in the null machine state.
        m_LUAStartBlockKeywords.clear();
        AZStd::string startKeywords[] = {
            {"do"}, {"if"}, {"function"}, {"repeat"}
        };
        m_LUAStartBlockKeywords.insert(std::begin(startKeywords), std::end(startKeywords));

        m_LUAEndBlockKeywords.clear();
        AZStd::string endKeywords[] = {
            {"end"}, {"until"}
        };
        m_LUAEndBlockKeywords.insert(std::begin(endKeywords), std::end(endKeywords));
    }

    LUASyntaxHighlighter::~LUASyntaxHighlighter()
    {
        azdestroy(m_machine);
    }

    void LUASyntaxHighlighter::highlightBlock(const QString& text)
    {
        m_machine->SetJoiningNames(true);
        m_machine->SetOnIncFoldLevel([](int) {});
        m_machine->SetOnDecFoldLevel([](int) {});

        auto colors = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        const HighlightedWords::LUAKeywordsType* libraryFuncs = nullptr;
        HighlightedWords::Bus::BroadcastResult(libraryFuncs, &HighlightedWords::Bus::Events::GetLUALibraryFunctions);

        auto cBlock = currentBlock();

        QTBlockState currentState;
        currentState.m_qtBlockState = currentBlockState();
        QTextCharFormat textFormat;

        textFormat.setFont(colors->GetFont());

        textFormat.setForeground(colors->GetTextColor());
        setFormat(0, cBlock.length(), textFormat);

        QTextCharFormat spaceFormat = QTextCharFormat();
        spaceFormat.setForeground(colors->GetTextWhitespaceColor());

        QRegExp tabsAndSpaces("( |\t)+");
        int index = tabsAndSpaces.indexIn(text);
        while (index >= 0)
        {
            int length = tabsAndSpaces.matchedLength();
            setFormat(index, length, spaceFormat);
            index = tabsAndSpaces.indexIn(text, index + length);
        }

        QTBlockState prevState;
        prevState.m_qtBlockState = previousBlockState();
        m_machine->SetSaveState(prevState);
        auto startingState = m_machine->GetCurrentParserState();

        m_machine->CaptureToken = [&](ParserStates state, int position, int length)
            {
                if (state == ParserStates::Name)
                {
                    const AZStd::string dhText(text.mid(position, length).toUtf8().constData());
                    if (libraryFuncs && libraryFuncs->find(dhText) != libraryFuncs->end())
                    {
                        textFormat.setForeground(colors->GetLibraryColor());
                        setFormat(position, length, textFormat);
                    }
                    else
                    {
                        textFormat.setForeground(colors->GetTextColor());
                        setFormat(position, length, textFormat);
                    }

                    if (m_LUAStartBlockKeywords.find(dhText) != m_LUAStartBlockKeywords.end())
                    {
                        m_machine->IncFoldLevel();
                    }
                    if (m_LUAEndBlockKeywords.find(dhText) != m_LUAEndBlockKeywords.end())
                    {
                        m_machine->DecFoldLevel();
                    }
                }
                if (state == ParserStates::ShortComment || state == ParserStates::LongComment)
                {
                    textFormat.setForeground(colors->GetCommentColor());
                    setFormat(position, length, textFormat);
                }
                if (state == ParserStates::Number || state == ParserStates::NumberHex)
                {
                    textFormat.setForeground(colors->GetNumberColor());
                    setFormat(position, length, textFormat);
                }
                if (state == ParserStates::StringLiteral)
                {
                    textFormat.setForeground(colors->GetStringLiteralColor());
                    setFormat(position, length, textFormat);
                }

                const bool canRunRegEx = state == ParserStates::Null || state == ParserStates::Name;
                if (!canRunRegEx)
                    return;

                // Special case to allow to lint methods via regex
                const int nextCharPos = position + length;
                const bool nextCharNeededForRegEx = text.at(nextCharPos) == '(' || text.at(nextCharPos) == ':';

                const QString dhText = nextCharNeededForRegEx ? text.mid(position, length + 1) : text.mid(position, length);
                for (const HighlightingRule& rule : m_highlightingRules)
                {
                    bool hasMatch = false;
                    QRegularExpressionMatchIterator i = rule.pattern.globalMatch(dhText);
                    while (i.hasNext())
                    {
                        hasMatch = true;
                        QRegularExpressionMatch match = i.next();
                        textFormat.setForeground(rule.colorCB());
                        setFormat(position + match.capturedStart(), match.capturedLength(), textFormat);
                    }

                    if (hasMatch && rule.stopProcessingMoreRulesAfterThis)
                        return;
                }
            };
        m_machine->Parse(text);

        // Selected bracket matching highlighting
        if (m_openBracketPos >= 0 && m_closeBracketPos >= 0)
        {
            if (cBlock.contains(m_openBracketPos))
            {
                setFormat(m_openBracketPos - cBlock.position(), 1, colors->GetSelectedBracketColor());
            }
            if (cBlock.contains(m_closeBracketPos))
            {
                setFormat(m_closeBracketPos - cBlock.position(), 1, colors->GetSelectedBracketColor());
            }
        }
        else if (m_openBracketPos >= 0)
        {
            if (cBlock.contains(m_openBracketPos))
            {
                setFormat(m_openBracketPos - cBlock.position(), 1, colors->GetUnmatchedBracketColor());
            }
            if (cBlock.contains(m_closeBracketPos))
            {
                setFormat(m_closeBracketPos - cBlock.position(), 1, colors->GetUnmatchedBracketColor());
            }
        }

        auto endingState = m_machine->GetCurrentParserState();
        if (startingState != ParserStates::LongComment && endingState == ParserStates::LongComment)
        {
            m_machine->IncFoldLevel();
        }
        if (startingState != ParserStates::StringLiteral && endingState == ParserStates::StringLiteral) //should only be true if a long string literal
        {
            m_machine->IncFoldLevel();
        }
        if (startingState == ParserStates::LongComment && endingState != ParserStates::LongComment)
        {
            m_machine->DecFoldLevel();
        }
        if (startingState == ParserStates::StringLiteral && endingState != ParserStates::StringLiteral) //should only be true if a long string literal
        {
            m_machine->DecFoldLevel();
        }

        QTBlockState newState = m_machine->GetSaveState();
        newState.m_blockState.m_folded = currentState.m_blockState.m_uninitialized ? 0 : currentState.m_blockState.m_folded;
        setCurrentBlockState(newState.m_qtBlockState);
    }

    void LUASyntaxHighlighter::SetBracketHighlighting(int openBracketPos, int closeBracketPos)
    {
        auto oldOpenBracketPos = m_openBracketPos;
        auto oldcloseBracketPos = m_closeBracketPos;
        m_openBracketPos = openBracketPos;
        m_closeBracketPos = closeBracketPos;
        auto openBlock = document()->findBlock(m_openBracketPos);
        auto closeBlock = document()->findBlock(m_closeBracketPos);

        if (openBlock.isValid())
        {
            rehighlightBlock(openBlock);
        }
        if (closeBlock.isValid())
        {
            rehighlightBlock(closeBlock);
        }

        if (oldOpenBracketPos >= 0)
        {
            openBlock = document()->findBlock(oldOpenBracketPos);
            if (openBlock.isValid())
            {
                rehighlightBlock(openBlock);
            }
        }
        if (oldcloseBracketPos >= 0)
        {
            closeBlock = document()->findBlock(oldcloseBracketPos);
            if (closeBlock.isValid())
            {
                rehighlightBlock(closeBlock);
            }
        }
    }

    //This code is also getting the list of lua names that are currently in scope
    QList<QTextEdit::ExtraSelection> LUASyntaxHighlighter::HighlightMatchingNames(const QTextCursor& cursor, const QString&) const
    {
        m_machine->SetJoiningNames(false);
        m_machine->SetOnIncFoldLevel([](int) {});
        m_machine->SetOnDecFoldLevel([](int) {});

        const HighlightedWords::LUAKeywordsType* keywords = nullptr;
        HighlightedWords::Bus::BroadcastResult(keywords, &HighlightedWords::Bus::Events::GetLUAKeywords);
        const HighlightedWords::LUAKeywordsType* libraryFuncs = nullptr;
        HighlightedWords::Bus::BroadcastResult(libraryFuncs, &HighlightedWords::Bus::Events::GetLUALibraryFunctions);

        auto syntaxSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);
        auto font = syntaxSettings->GetFont();

        QList<QTextEdit::ExtraSelection> list;
        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        QTextCharFormat textFormat;
        textFormat.setFont(font);
        textFormat.setBackground(syntaxSettings->GetCurrentIdentifierColor());
        selection.format = textFormat;

        QTBlockState mState;
        mState.m_qtBlockState = -1;
        m_machine->SetSaveState(mState);

        QString searchString;
        ParserStates matchState = ParserStates::Null;

        int currentScopeBlock = -1;

        QStringList luaNames;
        for (auto block = document()->begin(); block != document()->end(); block = block.next())
        {
            auto text = block.text();
            auto cursorPos = cursor.position() - block.position();
            int delayedFold = 0;
            m_machine->CaptureToken = [&](ParserStates state, int position, int length)
                {
                    if (cursorPos >= position && cursorPos <= position + length)
                    {
                        if (state == ParserStates::Name)
                        {
                            QString match = text.mid(position, length);
                            AZStd::string dhText(match.toUtf8().constData());
                            if (!(keywords && keywords->find(dhText) != keywords->end()) &&
                                !(libraryFuncs && libraryFuncs->find(dhText) != libraryFuncs->end()))
                            {
                                searchString = AZStd::move(match);
                                matchState = state;
                                currentScopeBlock = block.blockNumber();
                            }
                        }
                    }

                    if (state == ParserStates::Name)
                    {
                        AZStd::string dhText(text.mid(position, length).toUtf8().constData());
                        if (m_LUAStartBlockKeywords.find(dhText) != m_LUAStartBlockKeywords.end())
                        {
                            m_machine->IncFoldLevel();
                        }
                        if (m_LUAEndBlockKeywords.find(dhText) != m_LUAEndBlockKeywords.end())
                        {
                            m_machine->DecFoldLevel();
                        }
                        if (length > 1)
                        {
                            luaNames.push_back(text.mid(position, length));
                        }
                    }
                };
            auto startingState = m_machine->GetCurrentParserState();
            m_machine->Parse(text);
            while (delayedFold > 0)
            {
                m_machine->IncFoldLevel();
                --delayedFold;
            }

            auto endingState = m_machine->GetCurrentParserState();
            if (startingState != ParserStates::LongComment && endingState == ParserStates::LongComment)
            {
                m_machine->IncFoldLevel();
            }
            if (startingState != ParserStates::StringLiteral && endingState == ParserStates::StringLiteral) //should only be true if a long string literal
            {
                m_machine->IncFoldLevel();
            }
            if (startingState == ParserStates::LongComment && endingState != ParserStates::LongComment)
            {
                m_machine->DecFoldLevel();
            }
            if (startingState == ParserStates::StringLiteral && endingState != ParserStates::StringLiteral) //should only be true if a long string literal
            {
                m_machine->DecFoldLevel();
            }
        }
        m_machine->SetOnIncFoldLevel([](int) {});
        m_machine->SetOnDecFoldLevel([](int) {});
        if (matchState != ParserStates::Null)
        {
            for (auto block = document()->begin(); block != document()->end(); block = block.next())
            {
                auto text = block.text();

                m_machine->CaptureToken = [&](ParserStates state, int position, int length)
                    {
                        QString token = text.mid(position, length);
                        if (state == matchState)
                        {
                            if (token == searchString)
                            {
                                selection.cursor.setPosition(position + block.position());
                                selection.cursor.setPosition(position + block.position() + length, QTextCursor::MoveMode::KeepAnchor);
                                list.push_back(selection);
                            }
                        }
                    };

                m_machine->Parse(text);
            }
        }

        if (currentScopeBlock != -1 && currentScopeBlock != m_currentScopeBlock)
        {
            LUANamesInScopeChanged(luaNames);
            m_currentScopeBlock = currentScopeBlock;
        }

        return list;
    }

    QString LUASyntaxHighlighter::GetLUAName(const QTextCursor& cursor)
    {
        auto block = document()->findBlock(cursor.position());
        if (!block.isValid())
        {
            return "";
        }

        auto prevBlock = block.previous();
        QTBlockState prevState;
        if (prevBlock.isValid())
        {
            prevState.m_qtBlockState = prevBlock.userState();
        }
        else
        {
            prevState.m_qtBlockState = -1;
        }
        m_machine->SetSaveState(prevState);
        m_machine->SetJoiningNames(true);
        m_machine->SetOnIncFoldLevel([](int) {});
        m_machine->SetOnDecFoldLevel([](int) {});
        auto cursorPos = cursor.position() - block.position();
        auto text = block.text();

        QString result;
        m_machine->CaptureToken = [&](ParserStates state, int position, int length)
            {
                if (cursorPos >= position && cursorPos <= position + length)
                {
                    if (state == ParserStates::Name)
                    {
                        result = text.mid(position, length);
                    }
                }
            };

        m_machine->Parse(block.text());

        return result;
    }
}
