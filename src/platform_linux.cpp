// Copyright

#include <ctime>
#include <dlfcn.h>
#include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#include <unistd.h>        // close
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat
#include <sys/time.h>

#define GameLibraryPath       "./draft.so"
#define GameLibraryReloadTime 1.0f

struct game_library
{
    void *                  Library;
    uint64                  LoadTime;
    const char              *Path;
    game_init_func          *GameInit;
    game_render_func        *GameRender;
    game_destroy_func       *GameDestroy;
    game_process_event_func *GameProcessEvent;
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
    timeval time;
    gettimeofday(&time, 0);
    return time.tv_sec*1000;
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
        Lib.GameRender = (game_render_func *)dlsym(Lib.Library, "GameRender");
        Lib.GameDestroy = (game_destroy_func *)dlsym(Lib.Library, "GameDestroy");
        Lib.GameProcessEvent = (game_process_event_func *)dlsym(Lib.Library, "GameProcessEvent");
    }
    else
    {
        Println("Missing game library");
        exit(EXIT_FAILURE);
    }
}

void UnloadGameLibrary(game_library &Lib)
{
    dlclose(Lib.Library);
    Lib.Library = 0;
    Lib.GameInit = NULL;
    Lib.GameRender = NULL;
    Lib.GameDestroy = NULL;
    Lib.GameProcessEvent = NULL;
}

bool GameLibraryChanged(game_library &Lib)
{
    uint64 LastWriteTime = PlatformGetFileLastWriteTime(Lib.Path);
    return (PlatformCompareFileTime(LastWriteTime, Lib.LoadTime) > 0);
}
