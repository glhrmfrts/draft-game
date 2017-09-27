// Copyright
//
// @TODO: transform defun into def + loadfun

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define IsDigit(Char) (Char >= '0' && Char <= '9')
#define IsSymbol(Char) ((Char >= 'a' && Char <= 'z') || \
                        (Char >= 'A' && Char <= 'Z') || \
                        (Char == '_') || \
                        (Char == '-') || \
                        (Char == '.'))

#define Is(Value, T) (Value.Type == ValueType_##T)

#define NATIVE_FUNC(name) void name(void *Data, sl_vm *Vm, sl_value *Args, int ArgCount)

typedef uint8_t uint8;
typedef uint32_t uint32;

enum sl_token_type
{
    TokenType_EOF,
    TokenType_LeftParen,
    TokenType_RightParen,
    TokenType_LeftBracket,
    TokenType_RightBracket,
    TokenType_String,
    TokenType_Number,
    TokenType_Symbol,
    TokenType_Hash,
};

enum sl_opcode
{
    OpCode_Halt,
    OpCode_Defun,
    OpCode_Def,
    OpCode_Defonce,
    OpCode_Set,
    OpCode_FuncCall,
    OpCode_LoadString,
    OpCode_LoadNumber,
    OpCode_LoadSymbol,
    OpCode_LoadFunc,
    OpCode_Return,
};

struct sl_lexer
{
    const char *Source;
    const char *Ptr;

    sl_token_type TokenType;
    int StringSize;
    union
    {
        float NumberVal;
        char *StringVal;
    };
};

struct sl_code
{
    int Size = 0;
    int Capacity = 0;
    uint8 *Data = NULL;
};

struct sl_string
{
    int Size = 0;
    char *Value;
};

#define FuncMaxArgs 8
struct sl_func
{
    sl_code Code;
    int StringIndex;
    int Args[FuncMaxArgs];
};

struct sl_script
{
    std::vector<sl_string> Strings;
    std::vector<float> Numbers;
    std::vector<sl_func *> Funcs;
    sl_code Code;
    char *Filename;
};

enum sl_value_type
{
    ValueType_Nil,
    ValueType_Number,
    ValueType_String,
    ValueType_Func,
    ValueType_NativeFunc,
    ValueType_Custom,
    ValueTypeMax,
};

static const char *ValueTypeStrings[] = {
    "nil", "number", "string", "func", "native_func", "custom"
};

struct sl_vm;
struct sl_value;

typedef NATIVE_FUNC(native_func);

struct sl_native
{
    native_func *Func;
    void *Data;
};

struct sl_value
{
    sl_value_type Type = ValueType_Nil;
    union
    {
        sl_string *String;
        sl_func *Func;
        sl_native *Native;
        void *Custom;
        float Number;
    };
};

#define MaxVars 255
struct sl_call_frame
{
    sl_value Vars[MaxVars];
    uint8 *CodePtr = NULL;
    sl_call_frame *Parent = NULL;
};

struct sl_vm
{
    std::unordered_map<std::string, sl_value> Globals;

    sl_value Stack[MaxVars];
    int StackTop = 0;
    sl_call_frame *CurrentFrame = NULL;
    sl_script *CurrentScript = NULL;
};

static void ParseExpr(sl_script *Script, sl_code *Code, sl_lexer *Lexer);

static void ParseSymbol(sl_lexer *Lexer)
{
    const char *Beg = Lexer->Ptr;
    while (IsSymbol(*Lexer->Ptr) || IsDigit(*Lexer->Ptr))
    {
        Lexer->Ptr++;
    }

    int Size = Lexer->Ptr - Beg;
    char *Str = new char[Size + 1];
    memcpy(Str, Beg, Size);
    Str[Size] = '\0';

    if (Lexer->StringVal)
    {
        // @TODO
        //delete[] Lexer->StringVal;
    }
    Lexer->StringVal = Str;
    Lexer->StringSize = Size;
}

static void NextToken(sl_lexer *Lexer)
{
    while (*Lexer->Ptr == ' ' ||
           *Lexer->Ptr == '\n' ||
           *Lexer->Ptr == '\t')
    {
        Lexer->Ptr++;
    }

    if (!(*Lexer->Ptr))
    {
        Lexer->TokenType = TokenType_EOF;
        return;
    }

    switch (*Lexer->Ptr)
    {
    case '(':
        Lexer->TokenType = TokenType_LeftParen;
        Lexer->Ptr++;
        break;

    case ')':
        Lexer->TokenType = TokenType_RightParen;
        Lexer->Ptr++;
        break;

    case '[':
        Lexer->TokenType = TokenType_LeftBracket;
        Lexer->Ptr++;
        break;

    case ']':
        Lexer->TokenType = TokenType_RightBracket;
        Lexer->Ptr++;
        break;

    case '#':
        Lexer->TokenType = TokenType_Hash;
        Lexer->Ptr++;
        break;

    case '"':
    {
        Lexer->TokenType = TokenType_String;
        Lexer->Ptr++;

        const char *Beg = Lexer->Ptr;
        while (*Lexer->Ptr != '"')
        {
            Lexer->Ptr++;
        }

        int Size = Lexer->Ptr - Beg;
        char *Str = new char[Size + 1];
        memcpy(Str, Beg, Size);
        Str[Size] = '\0';

        if (Lexer->StringVal)
        {
            delete[] Lexer->StringVal;
        }
        Lexer->StringVal = Str;
        Lexer->StringSize = Size;

        Lexer->Ptr++;
        break;
    }

    case '\'':
    {
        Lexer->TokenType = TokenType_String;
        Lexer->Ptr++;

        if (IsSymbol(*Lexer->Ptr))
        {
            ParseSymbol(Lexer);
        }
        break;
    }

    default:
        if (IsDigit(*Lexer->Ptr))
        {
            Lexer->TokenType = TokenType_Number;

            const char *Beg = Lexer->Ptr;
            while (IsDigit(*Lexer->Ptr) || *Lexer->Ptr == '.')
            {
                Lexer->Ptr++;
            }

            int Size = Lexer->Ptr - Beg;
            char *Str = new char[Size + 1];
            memcpy(Str, Beg, Size);
            Str[Size] = '\0';

            Lexer->NumberVal = (float)atof(Str);
            delete[] Str;
        }
        else if (IsSymbol(*Lexer->Ptr))
        {
            Lexer->TokenType = TokenType_Symbol;
            ParseSymbol(Lexer);
        }
        break;
    }
}

static void InitLexer(sl_lexer *Lexer, const char *Source)
{
    Lexer->Source = Source;
    Lexer->Ptr = Source;
    Lexer->StringVal = NULL;
    NextToken(Lexer);
}

static int AddString(sl_script *Script, const char *Value, int Size)
{
    // @TODO: use arena for allocation
    for (int i = 0; i < Script->Strings.size(); i++)
    {
        auto &Str = Script->Strings[i];
        if (strcmp(Value, Str.Value) == 0)
        {
            return i;
        }
    }

    sl_string Str;
    Str.Size = Size;
    Str.Value = new char[Size+1];
    memcpy(Str.Value, Value, Size+1);

    Script->Strings.push_back(Str);
    return Script->Strings.size() - 1;
}

static int AddNumber(sl_script *Script, float Value)
{
    for (int i = 0; i < Script->Numbers.size(); i++)
    {
        float Num = Script->Numbers[i];
        if (Value == Num)
        {
            return i;
        }
    }
    Script->Numbers.push_back(Value);
    return Script->Numbers.size() - 1;
}

static void Write(sl_code *Code, uint8 Val)
{
    if (Code->Size >= Code->Capacity)
    {
        if (!Code->Capacity)
        {
            Code->Capacity = 4;
        }
        else
        {
            Code->Capacity *= 2;
        }
        uint8 *Data = new uint8[Code->Capacity];
        if (Code->Data)
        {
            memcpy(Data, Code->Data, Code->Size);
            delete[] Code->Data;
        }

        Code->Data = Data;
    }

    Code->Data[Code->Size++] = Val;
}

static int Emit(sl_code *Code, sl_opcode OpCode, uint8 Arg = 0)
{
    Write(Code, (uint8)OpCode);
    Write(Code, Arg);
    return Code->Size / 2;
}

static void Modify(sl_code *Code, int Index, uint8 Arg)
{
    int ByteIndex = Index*2 + 1;
    Code->Data[ByteIndex] = Arg;
}

static void AddDefOp(sl_script *Script, sl_code *Code, sl_lexer *Lexer, sl_opcode OpCode)
{
    int StrIndex = AddString(Script, Lexer->StringVal, Lexer->StringSize);
    NextToken(Lexer);
    ParseExpr(Script, Code, Lexer);
    NextToken(Lexer);
    Emit(Code, OpCode, (uint8)StrIndex);
}

static bool ParseReserved(sl_script *Script, sl_code *Code, sl_lexer *Lexer)
{
    switch (Lexer->StringSize)
    {
    case 7:
        if (strcmp("defonce", Lexer->StringVal) == 0)
        {
            NextToken(Lexer);
            if (Lexer->TokenType != TokenType_Symbol)
            {
                printf("error: defonce expecting symbol\n");
                return true;
            }

            AddDefOp(Script, Code, Lexer, OpCode_Defonce);
            return true;
        }
        break;

    case 5:
        if (strcmp("defun", Lexer->StringVal) == 0)
        {
            NextToken(Lexer);
            if (Lexer->TokenType != TokenType_Symbol)
            {
                printf("error: defun expecting symbol\n");
                return true;
            }

            sl_func *Func = new sl_func;
            Func->StringIndex = AddString(Script, Lexer->StringVal, Lexer->StringSize);
            NextToken(Lexer);
            if (Lexer->TokenType == TokenType_LeftBracket)
            {
                NextToken(Lexer);
                int ArgIndex = 0;
                while (Lexer->TokenType == TokenType_Symbol)
                {
                    if (ArgIndex >= FuncMaxArgs)
                    {
                        printf("error: function '%s': can't have more than %d arguments\n",
                               Script->Strings[Func->StringIndex],
                               FuncMaxArgs);
                    }
                    Func->Args[ArgIndex] = AddString(Script, Lexer->StringVal, Lexer->StringSize);
                    NextToken(Lexer);
                    ArgIndex++;
                }

                for (int i = ArgIndex-1; i >= 0; --i)
                {
                    Emit(&Func->Code, OpCode_Def, Func->Args[i]);
                }

                if (Lexer->TokenType != TokenType_RightBracket)
                {
                    printf("error: function '%s': expecting ']' to close arguments\n",
                           Script->Strings[Func->StringIndex].Value);
                }
                NextToken(Lexer);
            }
            else
            {
                printf("error: function '%s': expecting function arguments\n",
                       Script->Strings[Func->StringIndex].Value);
            }

            while (Lexer->TokenType != TokenType_RightParen)
            {
                ParseExpr(Script, &Func->Code, Lexer);
            }
            NextToken(Lexer);
            Emit(&Func->Code, OpCode_Return);

            Script->Funcs.push_back(Func);
            Emit(Code, OpCode_Defun, (uint8)(Script->Funcs.size() - 1));
            return true;
        }
        break;

    case 3:
        if (strcmp("def", Lexer->StringVal) == 0)
        {
            NextToken(Lexer);
            if (Lexer->TokenType != TokenType_Symbol)
            {
                printf("error: def expecting symbol\n");
                return true;
            }

            AddDefOp(Script, Code, Lexer, OpCode_Def);
            return true;
        }
        if (strcmp("set", Lexer->StringVal) == 0)
        {
            NextToken(Lexer);
            if (Lexer->TokenType != TokenType_Symbol)
            {
                printf("error: def expecting symbol\n");
                return true;
            }

            AddDefOp(Script, Code, Lexer, OpCode_Set);
            return true;
        }
        break;
    }

    return false;
}

static void ParseExpr(sl_script *Script, sl_code *Code, sl_lexer *Lexer)
{
    switch (Lexer->TokenType)
    {
    case TokenType_LeftParen:
    {
        NextToken(Lexer);
        if (Lexer->TokenType == TokenType_Symbol)
        {
            if (ParseReserved(Script, Code, Lexer))
            {
                return;
            }
        }

        int ArgCount = 0;
        while (Lexer->TokenType != TokenType_RightParen)
        {
            ParseExpr(Script, Code, Lexer);
            ArgCount++;
        }
        NextToken(Lexer);

        if (ArgCount > 0)
        {
            Emit(Code, OpCode_FuncCall, (uint8)ArgCount - 1);
        }
        break;
    }

    case TokenType_Hash:
    {
        NextToken(Lexer);

        sl_func *Func = new sl_func;
        Func->StringIndex = AddString(Script, "#", 1);
        ParseExpr(Script, &Func->Code, Lexer);
        Emit(&Func->Code, OpCode_Return);

        Script->Funcs.push_back(Func);
        Emit(Code, OpCode_LoadFunc, (uint8)(Script->Funcs.size() - 1));
        break;
    }

    case TokenType_String:
    {
        int StrIndex = AddString(Script, Lexer->StringVal, Lexer->StringSize);
        Emit(Code, OpCode_LoadString, (uint8)StrIndex);
        NextToken(Lexer);
        break;
    }

    case TokenType_Number:
    {
        int NumIndex = AddNumber(Script, Lexer->NumberVal);
        Emit(Code, OpCode_LoadNumber, (uint8)NumIndex);
        NextToken(Lexer);
        break;
    }

    case TokenType_Symbol:
    {
        int StrIndex = AddString(Script, Lexer->StringVal, Lexer->StringSize);
        Emit(Code, OpCode_LoadSymbol, (uint8)StrIndex);
        NextToken(Lexer);
        break;
    }

    default:
        break;
    }
}

static void DisasmCode(sl_script *Script, sl_code *Code, int Indent = 0)
{
    for (int i = 0; i < Code->Size; i += 2)
    {
        sl_opcode OpCode = (sl_opcode)Code->Data[i];
        int Arg = (int)Code->Data[i + 1];

        for (int i = 0; i < Indent; i++)
        {
            printf("\t");
        }

        printf("%d", OpCode);
        printf("\t");
        switch (OpCode)
        {
        case OpCode_Defun:
        {
            sl_func *Func = Script->Funcs[Arg];
            printf("Defun index:%d (%s)", Arg, Script->Strings[Func->StringIndex].Value);
            break;
        }

        case OpCode_Def:
            printf("Def index:%d (%s)", Arg, Script->Strings[Arg].Value);
            break;

        case OpCode_Defonce:
            printf("Defonce index:%d (%s)", Arg, Script->Strings[Arg].Value);
            break;

        case OpCode_Set:
            printf("Set index:%d (%s)", Arg, Script->Strings[Arg].Value);
            break;

        case OpCode_FuncCall:
            printf("FuncCall args:%d", Arg);
            break;

        case OpCode_LoadString:
            printf("LoadString index:%d (%s)", Arg, Script->Strings[Arg].Value);
            break;

        case OpCode_LoadNumber:
            printf("LoadNumber index:%d (%.4f)", Arg, Script->Numbers[Arg]);
            break;

        case OpCode_LoadSymbol:
            printf("LoadSymbol index:%d (%s)", Arg, Script->Strings[Arg].Value);
            break;

        case OpCode_LoadFunc:
            printf("LoadFunc index:%d", Arg);
            break;

        case OpCode_Return:
            printf("Return");
            break;

        case OpCode_Halt:
            printf("Halt");
            break;
        }

        printf("\n");
    }
}

void Disasm(sl_script *Script)
{
    printf("simple_lisp:\t%s\n\n", Script->Filename);
    printf("strings:\t");
    for (auto &Str : Script->Strings)
    {
        printf("%s ", Str.Value);
    }
    printf("\n\n");

    printf("numbers:\t");
    for (float Num : Script->Numbers)
    {
        printf("%.4f ", Num);
    }
    printf("\n\n");

    printf("funcs:\n");
    for (auto Func : Script->Funcs)
    {
        printf("\t%s code (%d):\n", Script->Strings[Func->StringIndex].Value, Func->Code.Size/2);

        DisasmCode(Script, &Func->Code, 2);
        printf("\n");
    }
    printf("\n");

    printf("code (%d):\n", Script->Code.Size/2);
    DisasmCode(Script, &Script->Code);
}

void CompileScript(sl_script *Script, const char *Source)
{
    sl_lexer Lexer;
    InitLexer(&Lexer, Source);

    while (Lexer.TokenType != TokenType_EOF)
    {
        ParseExpr(Script, &Script->Code, &Lexer);
    }
    Emit(&Script->Code, OpCode_Halt);
}

inline sl_value CreateNumber(float Number)
{
    sl_value Result;
    Result.Type = ValueType_Number;
    Result.Number = Number;
    return Result;
}

inline sl_value CreateString(sl_string *String)
{
    sl_value Result;
    Result.Type = ValueType_String;
    Result.String = String;
    return Result;
}

inline sl_value CreateCustom(void *Custom)
{
    sl_value Result;
    Result.Type = ValueType_Custom;
    Result.Custom = Custom;
    return Result;
}

static void PushCallFrame(sl_vm *Vm, uint8 *Code)
{
    sl_call_frame *Frame = new sl_call_frame;
    Frame->CodePtr = Code;
    Frame->Parent = Vm->CurrentFrame;
    Vm->CurrentFrame = Frame;
}

static void StackPush(sl_vm *Vm, sl_value Value)
{
    Vm->Stack[Vm->StackTop++] = Value;
}

static sl_value StackPop(sl_vm *Vm)
{
    if (Vm->StackTop > 0)
    {
        sl_value Result = Vm->Stack[--Vm->StackTop];
        return Result;
    }
    return sl_value{};
}

void RegisterNativeFunc(sl_vm *Vm, const std::string &Name, native_func *Func, void *Data)
{
    sl_value Value;
    Value.Type = ValueType_NativeFunc;
    Value.Native = new sl_native;
    Value.Native->Func = Func;
    Value.Native->Data = Data;
    Vm->Globals[Name] = Value;
}

void Execute(sl_vm *Vm, sl_script *Script, sl_code *Code, bool StopOnReturn = false)
{
    PushCallFrame(Vm, Code->Data);
    for (;;)
    {
        sl_call_frame *Frame = Vm->CurrentFrame;
        sl_opcode OpCode = (sl_opcode)*Frame->CodePtr++;
        int Arg = (int)*Frame->CodePtr++;

        switch (OpCode)
        {
        case OpCode_Def:
        {
            Frame->Vars[Arg] = StackPop(Vm);
            break;
        }

        case OpCode_Defonce:
        {
            if (Frame->Vars[Arg].Type == ValueType_Nil)
            {
                Frame->Vars[Arg] = StackPop(Vm);
            }
            break;
        }

        case OpCode_Set:
        {
            sl_call_frame *LookFrame = Frame;
            bool Set = false;
            while (LookFrame)
            {
                if (!Is(LookFrame->Vars[Arg], Nil))
                {
                    LookFrame->Vars[Arg] = StackPop(Vm);
                    Set = true;
                }
                LookFrame = LookFrame->Parent;
            }
            if (!Set)
            {
                std::string Str(Script->Strings[Arg].Value);
                Vm->Globals[Str] = StackPop(Vm);
            }
            break;
        }

        case OpCode_Defun:
        {
            sl_value Value;
            Value.Type = ValueType_Func;
            Value.Func = Script->Funcs[Arg];
            Frame->Vars[Value.Func->StringIndex] = Value;
            break;
        }

        case OpCode_LoadNumber:
        {
            sl_value Value;
            Value.Type = ValueType_Number;
            Value.Number = Script->Numbers[Arg];
            StackPush(Vm, Value);
            break;
        }

        case OpCode_LoadString:
        {
            sl_value Value;
            Value.Type = ValueType_String;
            Value.String = &Script->Strings[Arg];
            StackPush(Vm, Value);
            break;
        }

        case OpCode_LoadSymbol:
        {
            sl_call_frame *LookFrame = Frame;
            bool Found = false;
            while (LookFrame)
            {
                if (!Is(LookFrame->Vars[Arg], Nil))
                {
                    StackPush(Vm, LookFrame->Vars[Arg]);
                    Found = true;
                }

                LookFrame = LookFrame->Parent;
            }
            if (!Found)
            {
                std::string Str(Script->Strings[Arg].Value);
                if (Vm->Globals.find(Str) != Vm->Globals.end())
                {
                    StackPush(Vm, Vm->Globals[Str]);
                }
                else
                {
                    StackPush(Vm, sl_value{});
                }
            }
            break;
        }

        case OpCode_LoadFunc:
        {
            sl_value Value;
            Value.Type = ValueType_Func;
            Value.Func = Script->Funcs[Arg];
            StackPush(Vm, Value);
            break;
        }

        case OpCode_FuncCall:
        {
            sl_value *Args = new sl_value[Arg];
            for (int i = Arg - 1; i >= 0; --i)
            {
                Args[i] = StackPop(Vm);
            }

            sl_value Func = StackPop(Vm);
            if (Func.Type == ValueType_NativeFunc)
            {
                Func.Native->Func(Func.Native->Data, Vm, Args, Arg);
            }
            else if (Func.Type == ValueType_Func)
            {
                for (int i = 0; i < Arg; i++)
                {
                    StackPush(Vm, Args[i]);
                }
                PushCallFrame(Vm, Func.Func->Code.Data);
            }
            break;
        }

        case OpCode_Return:
        {
            sl_call_frame *Parent = Vm->CurrentFrame->Parent;
            delete Vm->CurrentFrame;

            Vm->CurrentFrame = Parent;
            if (StopOnReturn)
            {
                goto end;
            }
            break;
        }

        case OpCode_Halt:
            goto end;

        default:
            break;
        }
    }

end:
    return;
}

inline void Execute(sl_vm *Vm, sl_script *Script)
{
    Vm->CurrentScript = Script;
    Execute(Vm, Script, &Script->Code, false);
}

static long int
GetFileSize(FILE *handle)
{
    long int result = 0;

    fseek(handle, 0, SEEK_END);
    result = ftell(handle);
    rewind(handle);

    return result;
}

static const char *
ReadFile(const char *filename)
{
    FILE *handle;
#ifdef _WIN32
    fopen_s(&handle, filename, "r");
#else
    handle = fopen(filename, "r");
#endif

    long int length = GetFileSize(handle);
    char *buffer = new char[length + 1];
    int i = 0;
    char c = 0;
    while ((c = fgetc(handle)) != EOF)
    {
        buffer[i++] = c;
        if (i >= length) break;
    }

    buffer[i] = '\0';
    fclose(handle);

    return (const char *)buffer;
}

NATIVE_FUNC(Println)
{
    for (int i = 0; i < ArgCount; i++)
    {
        sl_value Arg = Args[i];
        switch (Arg.Type)
        {
        case ValueType_Nil:
            printf("nil");
            break;

        case ValueType_String:
            printf("%s", Arg.String->Value);
            break;

        case ValueType_Number:
            printf("%.4f", Arg.Number);
            break;

        default:
            printf("println unimplemented for this type\n");
            break;
        }

        if (i < ArgCount - 1)
        {
            printf(" ");
        }
    }
    printf("\n");
}
