// Copyright

#include <Windows.h>

#define GameLibraryPath "Draft.dll"
#define TempLibraryPath "_temp.dll"
#define GameLibraryReloadTime 1.0f

struct game_library
{
    HMODULE                 Library;
    uint64                  LoadTime;
    game_init_func          *GameInit;
    game_render_func        *GameRender;
    game_destroy_func       *GameDestroy;
    game_process_event_func *GameProcessEvent;
};

static inline FILETIME
Uint64ToFileTime(uint64 t)
{
    ULARGE_INTEGER LargeInteger = {};
    LargeInteger.QuadPart = t;
    
    FILETIME Result = {};
    Result.dwLowDateTime = LargeInteger.LowPart;
    Result.dwHighDateTime = LargeInteger.HighPart;
    return Result;
}

PLATFORM_GET_FILE_LAST_WRITE_TIME(PlatformGetFileLastWriteTime)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    ULARGE_INTEGER LargeInteger;
    LargeInteger.LowPart = LastWriteTime.dwLowDateTime;
    LargeInteger.HighPart = LastWriteTime.dwHighDateTime;
    return (uint64)LargeInteger.QuadPart;
}

PLATFORM_COMPARE_FILE_TIME(PlatformCompareFileTime)
{
    FILETIME FileTime1 = Uint64ToFileTime(t1);
    FILETIME FileTime2 = Uint64ToFileTime(t2);
    int32 Result = int32(CompareFileTime(&FileTime1, &FileTime2));
    return Result;
}

static void
LoadGameLibrary(game_library &Lib)
{
    //CopyFile(GameLibraryPath, TempLibraryPath, FALSE);
    Lib.Library = LoadLibrary(GameLibraryPath);
    if (Lib.Library)
    {
        Lib.LoadTime = PlatformGetFileLastWriteTime(GameLibraryPath);
        printf("Loading game library\n");

        Lib.GameInit = (game_init_func *)GetProcAddress(Lib.Library, "GameInit");
        Lib.GameRender = (game_render_func *)GetProcAddress(Lib.Library, "GameRender");
        Lib.GameDestroy = (game_destroy_func *)GetProcAddress(Lib.Library, "GameDestroy");
        Lib.GameProcessEvent = (game_process_event_func *)GetProcAddress(Lib.Library, "GameProcessEvent");
    }
    else
    {
        Println("Missing game library");
        exit(EXIT_FAILURE);
    }
}

static void
UnloadGameLibrary(game_library &Lib)
{
    FreeLibrary(Lib.Library);
    Lib.Library = 0;
    Lib.GameInit = NULL;
    Lib.GameRender = NULL;
    Lib.GameDestroy = NULL;
    Lib.GameProcessEvent = NULL;
}

static bool
GameLibraryChanged(game_library &Lib)
{
    uint64 LastWriteTime = PlatformGetFileLastWriteTime(GameLibraryPath);
    bool Result = (PlatformCompareFileTime(LastWriteTime, Lib.LoadTime) > 0);
    return Result;
}