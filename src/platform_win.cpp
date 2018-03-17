// Copyright

#include <Windows.h>
#include "memory.cpp"

#ifdef DRAFT_EDITOR
#define GameLibraryPath "DraftEditor.dll"
#else
#define GameLibraryPath "Draft.dll"
#endif

#define GameLibraryReloadTime 1.0f

struct game_library
{
	memory_arena                  Arena;
    HMODULE                       Library;
    uint64                        LoadTime;
    const char                    *Path;
	const char                    *TempPath;
    game_init_func                *GameInit;
	game_reload_func              *GameReload;
	game_unload_func              *GameUnload;
    game_update_func              *GameUpdate;
    game_render_func              *GameRender;
    game_destroy_func             *GameDestroy;
    game_process_event_func       *GameProcessEvent;
	std::vector<platform_thread *> Threads;
};

static FILETIME Uint64ToFileTime(uint64 t)
{
    ULARGE_INTEGER LargeInteger = {};
    LargeInteger.QuadPart = t;

    FILETIME Result = {};
    Result.dwLowDateTime = LargeInteger.LowPart;
    Result.dwHighDateTime = LargeInteger.HighPart;
    return Result;
}

static uint64 FileTimeToUint64(FILETIME *FileTime)
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.LowPart = FileTime->dwLowDateTime;
    LargeInteger.HighPart = FileTime->dwHighDateTime;
    return (uint64)LargeInteger.QuadPart;
}

PLATFORM_GET_FILE_LAST_WRITE_TIME(PlatformGetFileLastWriteTime)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    return FileTimeToUint64(&LastWriteTime);
}

PLATFORM_COMPARE_FILE_TIME(PlatformCompareFileTime)
{
    FILETIME FileTime1 = Uint64ToFileTime(t1);
    FILETIME FileTime2 = Uint64ToFileTime(t2);
    int32 Result = int32(CompareFileTime(&FileTime1, &FileTime2));
    return Result;
}

PLATFORM_GET_MILLISECONDS(PlatformGetMilliseconds)
{
    SYSTEMTIME Time;
    FILETIME FileTime;
    GetSystemTime(&Time);
    SystemTimeToFileTime(&Time, &FileTime);
    return FileTimeToUint64(&FileTime)/10000;
}

PLATFORM_CREATE_THREAD(PlatformCreateThread)
{
	auto lib = (game_library *)g->GameLibrary;
	auto result = PushStruct<platform_thread>(lib->Arena);
	result->Name = name;
	result->Arg = arg;
	result->Func = (platform_thread::func *)GetProcAddress(lib->Library, name);
	result->Thread = std::thread(result->Func, arg);
	lib->Threads.push_back(result);
	return result;
}

static void LoadGameLibrary(game_library &Lib, const char *Path, const char *TempPath)
{
    CopyFile(Path, TempPath, FALSE);
	Println("CopyFile");
	Println(GetLastError());

    Lib.Path = Path;
	Lib.TempPath = TempPath;
    Lib.Library = LoadLibrary(TempPath);

    if (Lib.Library)
    {
        Lib.LoadTime = PlatformGetFileLastWriteTime(Path);
        printf("Loading game library\n");

        Lib.GameInit = (game_init_func *)GetProcAddress(Lib.Library, "GameInit");
		Lib.GameReload = (game_reload_func *)GetProcAddress(Lib.Library, "GameReload");
		Lib.GameUnload = (game_unload_func *)GetProcAddress(Lib.Library, "GameUnload");
        Lib.GameUpdate = (game_update_func *)GetProcAddress(Lib.Library, "GameUpdate");
        Lib.GameRender = (game_render_func *)GetProcAddress(Lib.Library, "GameRender");
        Lib.GameDestroy = (game_destroy_func *)GetProcAddress(Lib.Library, "GameDestroy");
        Lib.GameProcessEvent = (game_process_event_func *)GetProcAddress(Lib.Library, "GameProcessEvent");

		for (auto t : Lib.Threads)
		{
			t->Func = (platform_thread::func *)GetProcAddress(Lib.Library, t->Name);
			t->Thread = std::thread(t->Func, t->Arg);
		}
    }
    else
    {
		printf("Missing game library: %d", GetLastError());
        exit(EXIT_FAILURE);
    }
}

static void UnloadGameLibrary(game_library &Lib)
{
	Lib.GameInit = NULL;
	Lib.GameReload = NULL;
	Lib.GameUnload = NULL;
	Lib.GameUpdate = NULL;
	Lib.GameRender = NULL;
	Lib.GameDestroy = NULL;
	Lib.GameProcessEvent = NULL;

	for (auto t : Lib.Threads)
	{
		t->Thread.join();
		t->Func = NULL;
	}

	while (GetModuleHandle(Lib.TempPath) != 0)
	{
		Println("FreeLibrary");
		Println(FreeLibrary(Lib.Library));
	}

	Println("DeleteFile");
	Println(DeleteFile(Lib.TempPath));
	Println(GetLastError());

    Lib.Library = 0;
}

static bool
GameLibraryChanged(game_library &Lib)
{
    uint64 LastWriteTime = PlatformGetFileLastWriteTime(Lib.Path);
    bool Result = (PlatformCompareFileTime(LastWriteTime, Lib.LoadTime) > 0);
    return Result;
}
