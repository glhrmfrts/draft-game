// Copyright

#include <Windows.h>

#define GameLibraryPath "Draft.dll"
#define TempLibraryPath "_temp.dll"
#define GameLibraryReloadTime 1.0f

struct game_library
{
    HMODULE                 Library;
	FILETIME                LoadTime;
    game_init_func          *GameInit;
    game_render_func        *GameRender;
    game_destroy_func       *GameDestroy;
    game_process_event_func *GameProcessEvent;
};

static inline FILETIME
GetFileLastWriteTime(char *Filename)
{
	FILETIME LastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
	{
		LastWriteTime = Data.ftLastWriteTime;
	}

	return LastWriteTime;
}

static void
LoadGameLibrary(game_library &Lib)
{
	//CopyFile(GameLibraryPath, TempLibraryPath, FALSE);
    Lib.Library = LoadLibrary(GameLibraryPath);
    if (Lib.Library)
    {
		Lib.LoadTime = GetFileLastWriteTime(GameLibraryPath);
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
    FILETIME LastWriteTime = GetFileLastWriteTime(GameLibraryPath);
    return (CompareFileTime(&LastWriteTime, &Lib.LoadTime) == 1);
}
