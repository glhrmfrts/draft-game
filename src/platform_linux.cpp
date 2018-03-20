// Copyright

#include <ctime>
#include <dlfcn.h>
#include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#include <unistd.h>        // close
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat
#include <sys/time.h>
#include "memory.cpp"

#define GameLibraryPath       "./draft.so"
#define GameLibraryReloadTime 1.0f

struct game_library
{
    memory_arena                   Arena;
    void *                         Library;
    uint64                         LoadTime;
    const char                    *Path;
    game_init_func                *GameInit;
	game_reload_func              *GameReload;
	game_unload_func              *GameUnload;
    game_update_func              *GameUpdate;
    game_render_func              *GameRender;
    game_destroy_func             *GameDestroy;
    game_process_event_func       *GameProcessEvent;
    std::vector<platform_thread *> Threads;
};

PLATFORM_GET_FILE_LAST_WRITE_TIME(PlatformGetFileLastWriteTime)
{
    struct stat Stat;
    stat(Filename, &Stat);

    return (uint64)Stat.st_mtime;
}

PLATFORM_COMPARE_FILE_TIME(PlatformCompareFileTime)
{
    return (int32)std::ceil((int32)difftime((time_t)t1, (time_t)t2));
}

PLATFORM_GET_MILLISECONDS(PlatformGetMilliseconds)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    long ms = std::round(time.tv_nsec / 1.0e6);
    return ms;
}

PLATFORM_CREATE_THREAD(PlatformCreateThread)
{
    auto lib = (game_library *)g->GameLibrary;
    auto result = PushStruct<platform_thread>(lib->Arena);
    result->Name = name;
    result->Arg = arg;
    result->Func = (platform_thread::func *)dlsym(lib->Library, name);
    result->Thread = std::thread(result->Func, arg);
    lib->Threads.push_back(result);
    return result;
}

void LoadGameLibrary(game_library &Lib, const char *Path, const char *TempPath)
{
    int Source = open(Path, O_RDONLY, 0);
    int Dest = open(TempPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // struct required, rationale: function stat() exists also
    struct stat StatSource;
    fstat(Source, &StatSource);

    sendfile(Dest, Source, 0, StatSource.st_size);

    close(Source);
    close(Dest);

    Lib.Path = Path;
    Lib.Library = dlopen(TempPath, RTLD_LAZY);
    if (Lib.Library)
    {
        Lib.LoadTime = PlatformGetFileLastWriteTime(Path);
        printf("Loading game library\n");

        Lib.GameInit = (game_init_func *)dlsym(Lib.Library, "GameInit");
        Lib.GameReload = (game_reload_func *)dlsym(Lib.Library, "GameReload");
        Lib.GameUnload = (game_unload_func *)dlsym(Lib.Library, "GameUnload");
        Lib.GameUpdate = (game_update_func *)dlsym(Lib.Library, "GameUpdate");
        Lib.GameRender = (game_render_func *)dlsym(Lib.Library, "GameRender");
        Lib.GameDestroy = (game_destroy_func *)dlsym(Lib.Library, "GameDestroy");
        Lib.GameProcessEvent = (game_process_event_func *)dlsym(Lib.Library, "GameProcessEvent");

        for (auto t : Lib.Threads)
        {
            t->Func = (platform_thread::func *)dlsym(Lib.Library, t->Name);
            t->Thread = std::thread(t->Func, t->Arg);
        }
    }
    else
    {
        Println("Missing game library");
        exit(EXIT_FAILURE);
    }
}

void UnloadGameLibrary(game_library &Lib)
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

    dlclose(Lib.Library);
    Lib.Library = 0;
}

bool GameLibraryChanged(game_library &Lib)
{
    uint64 LastWriteTime = PlatformGetFileLastWriteTime(Lib.Path);
    return (PlatformCompareFileTime(LastWriteTime, Lib.LoadTime) > 0);
}
