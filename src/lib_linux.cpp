// Copyright

#include <ctime>
#include <dlfcn.h>
#include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#include <unistd.h>        // close
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat

#define GameLibraryPath       "./draft.so"
#define TempLibraryPath       "./_temp.so"
#define GameLibraryReloadTime 1.0f

struct game_library
{
    void *                  Library;
	time_t                  LoadTime;
    game_init_func          *GameInit;
    game_render_func        *GameRender;
    game_destroy_func       *GameDestroy;
    game_process_event_func *GameProcessEvent;
};

static inline time_t
GetFileLastWriteTime(char *Filename)
{
	struct stat Stat;
    stat(Filename, &Stat);

	return Stat.st_mtime;
}

static void
LoadGameLibrary(game_library &Lib)
{
    int Source = open(GameLibraryPath, O_RDONLY, 0);
    int Dest = open(TempLibraryPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // struct required, rationale: function stat() exists also
    struct stat StatSource;
    fstat(Source, &StatSource);

    sendfile(Dest, Source, 0, StatSource.st_size);

    close(Source);
    close(Dest);

    Lib.Library = dlopen(TempLibraryPath, RTLD_LAZY);
    if (Lib.Library)
    {
		Lib.LoadTime = GetFileLastWriteTime(GameLibraryPath);
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

static void
UnloadGameLibrary(game_library &Lib)
{
	dlclose(Lib.Library);
	Lib.Library = 0;
	Lib.GameInit = NULL;
	Lib.GameRender = NULL;
	Lib.GameDestroy = NULL;
	Lib.GameProcessEvent = NULL;
}

static bool
GameLibraryChanged(game_library &Lib)
{
    time_t LastWriteTime = GetFileLastWriteTime(GameLibraryPath);
    return (difftime(LastWriteTime, Lib.LoadTime) > 0.0);
}
