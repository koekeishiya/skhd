#ifndef SERVICE_H
#define SERVICE_H

#include <spawn.h>
#include <sys/stat.h>

#define MAXLEN 512

#define _PATH_LAUNCHCTL   "/bin/launchctl"
#define _NAME_SKHD_PLIST "com.koekeishiya.skhd"
#define _PATH_SKHD_PLIST "%s/Library/LaunchAgents/"_NAME_SKHD_PLIST".plist"

#define _SKHD_PLIST \
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
    "<plist version=\"1.0\">\n" \
    "<dict>\n" \
    "    <key>Label</key>\n" \
    "    <string>"_NAME_SKHD_PLIST"</string>\n" \
    "    <key>ProgramArguments</key>\n" \
    "    <array>\n" \
    "        <string>%s</string>\n" \
    "    </array>\n" \
    "    <key>EnvironmentVariables</key>\n" \
    "    <dict>\n" \
    "        <key>PATH</key>\n" \
    "        <string>%s</string>\n" \
    "    </dict>\n" \
    "    <key>RunAtLoad</key>\n" \
    "    <true/>\n" \
    "    <key>KeepAlive</key>\n" \
    "    <true/>\n" \
    "    <key>StandardOutPath</key>\n" \
    "    <string>/tmp/skhd_%s.out.log</string>\n" \
    "    <key>StandardErrorPath</key>\n" \
    "    <string>/tmp/skhd_%s.err.log</string>\n" \
    "    <key>ThrottleInterval</key>\n" \
    "    <integer>30</integer>\n" \
    "    <key>ProcessType</key>\n" \
    "    <string>Interactive</string>\n" \
    "    <key>Nice</key>\n" \
    "    <integer>-20</integer>\n" \
    "</dict>\n" \
    "</plist>"

static bool file_exists(char *filename);

static int safe_exec(char *const argv[])
{
    pid_t pid;
    int status = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL);
    if (status) return 1;

    while ((waitpid(pid, &status, 0) == -1) && (errno == EINTR)) {
        usleep(1000);
    }

    if (WIFSIGNALED(status)) {
        return 1;
    } else if (WIFSTOPPED(status)) {
        return 1;
    } else {
        return WEXITSTATUS(status);
    }
}

static inline char *cfstring_copy(CFStringRef string)
{
    CFIndex num_bytes = CFStringGetMaximumSizeForEncoding(CFStringGetLength(string), kCFStringEncodingUTF8);
    char *result = malloc(num_bytes + 1);
    if (!result) return NULL;

    if (!CFStringGetCString(string, result, num_bytes + 1, kCFStringEncodingUTF8)) {
        free(result);
        result = NULL;
    }

    return result;
}

extern CFURLRef CFCopyHomeDirectoryURLForUser(void *user);
static char *populate_plist_path(void)
{
    CFURLRef homeurl_ref = CFCopyHomeDirectoryURLForUser(NULL);
    CFStringRef home_ref = homeurl_ref ? CFURLCopyFileSystemPath(homeurl_ref, kCFURLPOSIXPathStyle) : NULL;
    char *home = home_ref ? cfstring_copy(home_ref) : NULL;

    if (!home) {
        error("skhd: unable to retrieve home directory! abort..\n");
    }

    int size = strlen(_PATH_SKHD_PLIST)-2 + strlen(home) + 1;
    char *result = malloc(size);
    if (!result) {
        error("skhd: could not allocate memory for plist path! abort..\n");
    }

    memset(result, 0, size);
    snprintf(result, size, _PATH_SKHD_PLIST, home);

    return result;
}

static char *populate_plist(int *length)
{
    char *user = getenv("USER");
    if (!user) {
        error("skhd: 'env USER' not set! abort..\n");
    }

    char *path_env = getenv("PATH");
    if (!path_env) {
        error("skhd: 'env PATH' not set! abort..\n");
    }

    char exe_path[4096];
    unsigned int exe_path_size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &exe_path_size) < 0) {
        error("skhd: unable to retrieve path of executable! abort..\n");
    }

    int size = strlen(_SKHD_PLIST)-8 + strlen(exe_path) + strlen(path_env) + (2*strlen(user)) + 1;
    char *result = malloc(size);
    if (!result) {
        error("skhd: could not allocate memory for plist contents! abort..\n");
    }

    memset(result, 0, size);
    snprintf(result, size, _SKHD_PLIST, exe_path, path_env, user, user);
    *length = size-1;

    return result;
}


static inline bool directory_exists(char *filename)
{
    struct stat buffer;

    if (stat(filename, &buffer) != 0) {
        return false;
    }

    return S_ISDIR(buffer.st_mode);
}

static inline void ensure_directory_exists(char *skhd_plist_path)
{
    //
    // NOTE(koekeishiya): Temporarily remove filename.
    // We know the filepath will contain a slash, as
    // it is controlled by us, so don't bother checking
    // the result..
    //

    char *last_slash = strrchr(skhd_plist_path, '/');
    *last_slash = '\0';

    if (!directory_exists(skhd_plist_path)) {
        mkdir(skhd_plist_path, 0755);
    }

    //
    // NOTE(koekeishiya): Restore original filename.
    //

    *last_slash = '/';
}

static int service_install_internal(char *skhd_plist_path)
{
    int skhd_plist_length;
    char *skhd_plist = populate_plist(&skhd_plist_length);
    ensure_directory_exists(skhd_plist_path);

    FILE *handle = fopen(skhd_plist_path, "w");
    if (!handle) return 1;

    size_t bytes = fwrite(skhd_plist, skhd_plist_length, 1, handle);
    int result = bytes == 1 ? 0 : 1;
    fclose(handle);

    return result;
}

static int service_install(void)
{
    char *skhd_plist_path = populate_plist_path();

    if (file_exists(skhd_plist_path)) {
        error("skhd: service file '%s' is already installed! abort..\n", skhd_plist_path);
    }

    return service_install_internal(skhd_plist_path);
}

static int service_uninstall(void)
{
    char *skhd_plist_path = populate_plist_path();

    if (!file_exists(skhd_plist_path)) {
        error("skhd: service file '%s' is not installed! abort..\n", skhd_plist_path);
    }

    return unlink(skhd_plist_path) == 0 ? 0 : 1;
}

static int service_start(void)
{
    char *skhd_plist_path = populate_plist_path();

    if (!file_exists(skhd_plist_path)) {
        warn("skhd: service file '%s' is not installed! attempting installation..\n", skhd_plist_path);

        int result = service_install_internal(skhd_plist_path);
        if (result) {
            error("skhd: service file '%s' could not be installed! abort..\n", skhd_plist_path);
        }
    }

    const char *const args[] = { _PATH_LAUNCHCTL, "load", "-w", skhd_plist_path, NULL };
    return safe_exec((char *const*)args);
}

static int service_restart(void)
{
    char *skhd_plist_path = populate_plist_path();

    if (!file_exists(skhd_plist_path)) {
        error("skhd: service file '%s' is not installed! abort..\n", skhd_plist_path);
    }

    char skhd_service_id[MAXLEN];
    snprintf(skhd_service_id, sizeof(skhd_service_id), "gui/%d/%s", getuid(), _NAME_SKHD_PLIST);

    const char *const args[] = { _PATH_LAUNCHCTL, "kickstart", "-k", skhd_service_id, NULL };
    return safe_exec((char *const*)args);
}

static int service_stop(void)
{
    char *skhd_plist_path = populate_plist_path();

    if (!file_exists(skhd_plist_path)) {
        error("skhd: service file '%s' is not installed! abort..\n", skhd_plist_path);
    }

    const char *const args[] = { _PATH_LAUNCHCTL, "unload", "-w", skhd_plist_path, NULL };
    return safe_exec((char *const*)args);
}

#endif
