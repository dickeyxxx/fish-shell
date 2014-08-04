/** \file fishd.c

The universal variable server. fishd is automatically started by fish
if a fishd server isn't already running. fishd reads any saved
variables from ~/.fishd, and takes care of communication between fish
instances. When no clients are running, fishd will automatically shut
down and save.

\section fishd-commands Commands

Fishd works by sending and receiving commands. Each command is ended
with a newline. These are the commands supported by fishd:

<pre>set KEY:VALUE
set_export KEY:VALUE
</pre>

These commands update the value of a variable. The only difference
between the two is that <tt>set_export</tt>-variables should be
exported to children of the process using them. When sending messages,
all values below 32 or above 127 must be escaped using C-style
backslash escapes. This means that the over the wire protocol is
ASCII. However, any conforming reader must also accept non-ascii
characters and interpret them as UTF-8. Lines containing invalid UTF-8
escape sequences must be ignored entirely.

<pre>erase KEY
</pre>

Erase the variable with the specified name.

<pre>barrier
barrier_reply
</pre>

A \c barrier command will result in a barrier_reply being added to
the end of the senders queue of unsent messages. These commands are
used to synchronize clients, since once the reply for a barrier
message returns, the sender can know that any updates available at the
time the original barrier request was sent have been received.

*/

#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <pwd.h>
#include <fcntl.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <list>

#include "fallback.h"
#include "util.h"

#include "common.h"
#include "wutil.h"
#include "env_universal_common.h"
#include "path.h"
#include "print_help.h"

#ifndef HOST_NAME_MAX
/**
 Maximum length of hostname return. It is ok if this is too short,
 getting the actual hostname is not critical, so long as the string
 is unique in the filesystem namespace.
 */
#define HOST_NAME_MAX 255
#endif

/**
   Maximum length of socket filename
*/
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 100
#endif

/**
   Fallback if MSG_DONTWAIT isn't defined. That's actually prerry bad,
   and may lead to strange fishd behaviour, but at least it should
   work most of the time.
*/
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

/* Length of a MAC address */
#define MAC_ADDRESS_MAX_LEN 6

/**
   Small greeting to show that fishd is running
*/
#define GREETING "# Fish universal variable daemon\n"

/**
   Small not about not editing ~/.fishd manually. Inserted at the top of all .fishd files.
*/
#define SAVE_MSG "# This file is automatically generated by the fishd universal variable daemon.\n# Do NOT edit it directly, your changes will be overwritten.\n"

/**
   The name of the save file. The machine identifier is appended to this.
*/
#define FILE "fishd."

/**
   Maximum length of hostname. Longer hostnames are truncated
*/
#define HOSTNAME_LEN 32

/**
   The string to append to the socket name to name the lockfile
*/
#define LOCKPOSTFIX ".lock"

/**
   The timeout in seconds on the lockfile for critical section
*/
#define LOCKTIMEOUT 1

/**
   Getopt short switches for fishd
*/
#define GETOPT_STRING "hv"

/**
   The list of connections to clients
*/
typedef std::list<connection_t> connection_list_t;
static connection_list_t connections;

/**
   The socket to accept new clients on
*/
static int sock;

/**
   Set to one when fishd should save and exit
*/
static int quit=0;

/**
   Constructs the fish socket filename
*/
static std::string get_socket_filename(void)
{
    std::string dir = common_get_runtime_path();

    if (dir == "") {
        debug(0, L"Cannot access desired socket path.");
        exit(EXIT_FAILURE);
    }

    std::string name;
    name.reserve(dir.length() + strlen(SOCK_FILENAME) + 1);
    name.append(dir);
    name.push_back('/');
    name.append(SOCK_FILENAME);

    if (name.size() >= UNIX_PATH_MAX)
    {
        debug(1, L"Filename too long: '%s'", name.c_str());
        exit(EXIT_FAILURE);
    }
    return name;
}

/**
    Constructs the legacy socket filename
*/
static std::string get_old_socket_filename(void)
{
    const char *dir = getenv("FISHD_SOCKET_DIR");
    char *uname = getenv("USER");

    if (dir == NULL)
    {
        dir = "/tmp";
    }

    if (uname == NULL)
    {
        const struct passwd *pw = getpwuid(getuid());
        uname = pw->pw_name;
    }

    std::string name;
    name.reserve(strlen(dir)+ strlen(uname)+ strlen("fishd.socket.") + 1);
    name.append(dir);
    name.append("/fishd.socket.");
    name.append(uname);

    if (name.size() >= UNIX_PATH_MAX)
    {
        debug(1, L"Filename too long: '%s'", name.c_str());
        exit(EXIT_FAILURE);
    }
    return name;
}

/**
   Signal handler for the term signal.
*/
static void handle_term(int signal)
{
    quit=1;
}


/**
 Writes a pseudo-random number (between one and maxlen) of pseudo-random
 digits into str.
 str must point to an allocated buffer of size of at least maxlen chars.
 Returns the number of digits written.
 Since the randomness in part depends on machine time it has _some_ extra
 strength but still not enough for use in concurrent locking schemes on a
 single machine because gettimeofday may not return a different value on
 consecutive calls when:
 a) the OS does not support fine enough resolution
 b) the OS is running on an SMP machine.
 Additionally, gettimeofday errors are ignored.
 Excludes chars other than digits since ANSI C only guarantees that digits
 are consecutive.
 */
static void sprint_rand_digits(char *str, int maxlen)
{
    int i, max;
    struct timeval tv;

    /*
       Seed the pseudo-random generator based on time - this assumes
       that consecutive calls to gettimeofday will return different values
       and ignores errors returned by gettimeofday.
       Cast to unsigned so that wrapping occurs on overflow as per ANSI C.
       */
    static bool seeded = false;
    if (! seeded)
    {
        (void)gettimeofday(&tv, NULL);
        unsigned long long seed = tv.tv_sec + tv.tv_usec * 1000000ULL;
        srand((unsigned int)seed);
        seeded = true;
    }
    max = (int)(1 + (maxlen - 1) * (rand() / (RAND_MAX + 1.0)));
    for (i = 0; i < max; i++)
    {
        str[i] = '0' + 10 * (rand() / (RAND_MAX + 1.0));
    }
    str[i] = 0;
}



/**
 Generate a filename unique in an NFS namespace by creating a copy of str and
 appending .{hostname}.{pid} to it.  If gethostname() fails then a pseudo-
 random string is substituted for {hostname} - the randomness of the string
 should be strong enough across different machines.  The main assumption
 though is that gethostname will not fail and this is just a "safe enough"
 fallback.
 The memory returned should be freed using free().
 */
static std::string gen_unique_nfs_filename(const std::string &filename)
{
    char hostname[HOST_NAME_MAX + 1];
    char pid_str[256];
    snprintf(pid_str, sizeof pid_str, "%ld", (long)getpid());

    if (gethostname(hostname, sizeof hostname) != 0)
    {
        sprint_rand_digits(hostname, HOST_NAME_MAX);
    }

    std::string newname(filename);
    newname.push_back('.');
    newname.append(hostname);
    newname.push_back('.');
    newname.append(pid_str);
    return newname;
}


/* Thanks to Jan Brittenson
   http://lists.apple.com/archives/xcode-users/2009/May/msg00062.html
*/
#ifdef SIOCGIFHWADDR

/* Linux */
#include <net/if.h>
static bool get_mac_address(unsigned char macaddr[MAC_ADDRESS_MAX_LEN], const char *interface = "eth0")
{
    bool result = false;
    const int dummy = socket(AF_INET, SOCK_STREAM, 0);
    if (dummy >= 0)
    {
        struct ifreq r;
        strncpy((char *)r.ifr_name, interface, sizeof r.ifr_name - 1);
        r.ifr_name[sizeof r.ifr_name - 1] = 0;
        if (ioctl(dummy, SIOCGIFHWADDR, &r) >= 0)
        {
            memcpy(macaddr, r.ifr_hwaddr.sa_data, MAC_ADDRESS_MAX_LEN);
            result = true;
        }
        close(dummy);
    }
    return result;
}

#elif defined(HAVE_GETIFADDRS)

/* OS X and BSD */
#include <ifaddrs.h>
#include <net/if_dl.h>
static bool get_mac_address(unsigned char macaddr[MAC_ADDRESS_MAX_LEN], const char *interface = "en0")
{
    // BSD, Mac OS X
    struct ifaddrs *ifap;
    bool ok = false;

    if (getifaddrs(&ifap) == 0)
    {
        for (const ifaddrs *p = ifap; p; p = p->ifa_next)
        {
            if (p->ifa_addr->sa_family == AF_LINK)
            {
                if (p->ifa_name && p->ifa_name[0] &&
                        ! strcmp((const char*)p->ifa_name, interface))
                {

                    const sockaddr_dl& sdl = *(sockaddr_dl*)p->ifa_addr;

                    size_t alen = sdl.sdl_alen;
                    if (alen > MAC_ADDRESS_MAX_LEN) alen = MAC_ADDRESS_MAX_LEN;
                    memcpy(macaddr, sdl.sdl_data + sdl.sdl_nlen, alen);
                    ok = true;
                    break;
                }
            }
        }
        freeifaddrs(ifap);
    }
    return ok;
}

#else

/* Unsupported */
static bool get_mac_address(unsigned char macaddr[MAC_ADDRESS_MAX_LEN])
{
    return false;
}

#endif

/* Function to get an identifier based on the hostname */
static bool get_hostname_identifier(std::string *result)
{
    bool success = false;
    char hostname[HOSTNAME_LEN + 1] = {};
    if (gethostname(hostname, HOSTNAME_LEN) == 0)
    {
        result->assign(hostname);
        success = true;
    }
    return success;
}

/* Get a sort of unique machine identifier. Prefer the MAC address; if that fails, fall back to the hostname; if that fails, pick something. */
static std::string get_machine_identifier(void)
{
    std::string result;
    unsigned char mac_addr[MAC_ADDRESS_MAX_LEN] = {};
    if (get_mac_address(mac_addr))
    {
        result.reserve(2 * MAC_ADDRESS_MAX_LEN);
        for (size_t i=0; i < MAC_ADDRESS_MAX_LEN; i++)
        {
            char buff[3];
            snprintf(buff, sizeof buff, "%02x", mac_addr[i]);
            result.append(buff);
        }
    }
    else if (get_hostname_identifier(&result))
    {
        /* Hooray */
    }
    else
    {
        /* Fallback */
        result.assign("nohost");
    }
    return result;
}

/**
 The number of milliseconds to wait between polls when attempting to acquire
 a lockfile
 */
#define LOCKPOLLINTERVAL 10

/**
 Attempt to acquire a lock based on a lockfile, waiting LOCKPOLLINTERVAL
 milliseconds between polls and timing out after timeout seconds,
 thereafter forcibly attempting to obtain the lock if force is non-zero.
 Returns 1 on success, 0 on failure.
 To release the lock the lockfile must be unlinked.
 A unique temporary file named by appending characters to the lockfile name
 is used; any pre-existing file of the same name is subject to deletion.
 */
static int acquire_lock_file(const std::string &lockfile_str, const int timeout, int force)
{
    int fd, timed_out = 0;
    int ret = 0; /* early exit returns failure */
    struct timespec pollint;
    struct timeval start, end;
    double elapsed;
    struct stat statbuf;
    const char * const lockfile = lockfile_str.c_str();

    /*
       (Re)create a unique file and check that it has one only link.
       */
    const std::string linkfile_str = gen_unique_nfs_filename(lockfile);
    const char * const linkfile = linkfile_str.c_str();
    (void)unlink(linkfile);
    /* OK to not use CLO_EXEC here because fishd is single threaded */
    if ((fd = open(linkfile, O_CREAT|O_RDONLY, 0600)) == -1)
    {
        debug(1, L"acquire_lock_file: open: %s", strerror(errno));
        goto done;
    }
    /*
       Don't need to check exit status of close on read-only file descriptors
       */
    close(fd);
    if (stat(linkfile, &statbuf) != 0)
    {
        debug(1, L"acquire_lock_file: stat: %s", strerror(errno));
        goto done;
    }
    if (statbuf.st_nlink != 1)
    {
        debug(1, L"acquire_lock_file: number of hardlinks on unique "
              L"tmpfile is %d instead of 1.", (int)statbuf.st_nlink);
        goto done;
    }
    if (gettimeofday(&start, NULL) != 0)
    {
        debug(1, L"acquire_lock_file: gettimeofday: %s", strerror(errno));
        goto done;
    }
    end = start;
    pollint.tv_sec = 0;
    pollint.tv_nsec = LOCKPOLLINTERVAL * 1000000;
    do
    {
        /*
             Try to create a hard link to the unique file from the
             lockfile.  This will only succeed if the lockfile does not
             already exist.  It is guaranteed to provide race-free
             semantics over NFS which the alternative of calling
             open(O_EXCL|O_CREAT) on the lockfile is not.  The lock
             succeeds if the call to link returns 0 or the link count on
             the unique file increases to 2.
             */
        if (link(linkfile, lockfile) == 0 ||
                (stat(linkfile, &statbuf) == 0 &&
                 statbuf.st_nlink == 2))
        {
            /* Successful lock */
            ret = 1;
            break;
        }
        elapsed = end.tv_sec + end.tv_usec/1000000.0 -
                  (start.tv_sec + start.tv_usec/1000000.0);
        /*
             The check for elapsed < 0 is to deal with the unlikely event
             that after the loop is entered the system time is set forward
             past the loop's end time.  This would otherwise result in a
             (practically) infinite loop.
             */
        if (timed_out || elapsed >= timeout || elapsed < 0)
        {
            if (timed_out == 0 && force)
            {
                /*
                         Timed out and force was specified - attempt to
                         remove stale lock and try a final time
                         */
                (void)unlink(lockfile);
                timed_out = 1;
                continue;
            }
            else
            {
                /*
                         Timed out and final try was unsuccessful or
                         force was not specified
                         */
                debug(1, L"acquire_lock_file: timed out "
                      L"trying to obtain lockfile %s using "
                      L"linkfile %s", lockfile, linkfile);
                break;
            }
        }
        nanosleep(&pollint, NULL);
    }
    while (gettimeofday(&end, NULL) == 0);
done:
    /* The linkfile is not needed once the lockfile has been created */
    (void)unlink(linkfile);
    return ret;
}

/**
   Acquire the lock for the socket
   Returns the name of the lock file if successful or
   NULL if unable to obtain lock.
   The returned string must be free()d after unlink()ing the file to release
   the lock
*/
static bool acquire_socket_lock(const std::string &sock_name, std::string *out_lockfile_name)
{
    bool success = false;
    std::string lockfile;
    lockfile.reserve(sock_name.size() + strlen(LOCKPOSTFIX));
    lockfile = sock_name;
    lockfile.append(LOCKPOSTFIX);
    if (acquire_lock_file(lockfile, LOCKTIMEOUT, 1))
    {
        out_lockfile_name->swap(lockfile);
        success = true;
    }
    return success;
}

/**
   Connects to the fish socket and starts listening for connections
*/
static int get_socket(void)
{
    // Cygwin has random problems involving sockets. When using Cygwin,
    // allow 20 attempts at making socket correctly.
#ifdef __CYGWIN__
    int attempts = 0;
repeat:
    attempts += 1;
#endif

    int s, len, doexit = 0;
    int exitcode = EXIT_FAILURE;
    struct sockaddr_un local;
    const std::string sock_name = get_socket_filename();
    const std::string old_sock_name = get_old_socket_filename();

    /*
       Start critical section protected by lock
    */
    std::string lockfile;
    if (! acquire_socket_lock(sock_name, &lockfile))
    {
        debug(0, L"Unable to obtain lock on socket, exiting");
        exit(EXIT_FAILURE);
    }
    debug(4, L"Acquired lockfile: %s", lockfile.c_str());

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, sock_name.c_str());
    len = sizeof(local);

    debug(1, L"Connect to socket at %s", sock_name.c_str());

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        wperror(L"socket");
        doexit = 1;
        goto unlock;
    }

    /*
       First check whether the socket has been opened by another fishd;
       if so, exit with success status
    */
    if (connect(s, (struct sockaddr *)&local, len) == 0)
    {
        debug(1, L"Socket already exists, exiting");
        doexit = 1;
        exitcode = 0;
        goto unlock;
    }

    unlink(local.sun_path);
    if (bind(s, (struct sockaddr *)&local, len) == -1)
    {
        wperror(L"bind");
        doexit = 1;
        goto unlock;
    }

    if (make_fd_nonblocking(s) != 0)
    {
        wperror(L"fcntl");
        close(s);
        doexit = 1;
    }
    else if (listen(s, 64) == -1)
    {
        wperror(L"listen");
        doexit = 1;
    }

    // Attempt to hardlink the old socket name so that old versions of fish keep working on upgrade
    // Not critical if it fails
    if (unlink(old_sock_name.c_str()) != 0 && errno != ENOENT)
    {
        debug(0, L"Could not create legacy socket path");
        wperror(L"unlink");
    }
    else if (link(sock_name.c_str(), old_sock_name.c_str()) != 0)
    {
        debug(0, L"Could not create legacy socket path");
        wperror(L"link");
    }

unlock:
    (void)unlink(lockfile.c_str());
    debug(4, L"Released lockfile: %s", lockfile.c_str());
    /*
       End critical section protected by lock
    */

    if (doexit)
    {
        // If Cygwin, only allow normal quit when made lots of attempts.
#ifdef __CYGWIN__
        if (exitcode && attempts < 20) goto repeat;
#endif
        exit_without_destructors(exitcode);
    }

    return s;
}

/**
   Event handler. Broadcasts updates to all clients.
*/
static void broadcast(fish_message_type_t type, const wchar_t *key, const wchar_t *val)
{
    message_t *msg;

    if (connections.empty())
        return;

    msg = create_message(type, key, val);

    /*
      Don't merge these loops, or try_send_all can free the message
      prematurely
    */

    for (connection_list_t::iterator iter = connections.begin(); iter != connections.end(); ++iter)
    {
        msg->count++;
        iter->unsent.push(msg);
    }

    for (connection_list_t::iterator iter = connections.begin(); iter != connections.end(); ++iter)
    {
        try_send_all(&*iter);
    }
}

/**
   Make program into a creature of the night.
*/
static void daemonize()
{
    /*
      Fork, and let parent exit
    */
    switch (fork())
    {
        case -1:
            debug(0, L"Could not put fishd in background. Quitting");
            wperror(L"fork");
            exit(1);

        case 0:
        {
            /* Ordinarily there's very limited things we will do after fork, due to multithreading. But fishd is safe because it's single threaded. So don't die in is_forked_child. */
            setup_fork_guards();

            /*
              Make fishd ignore the HUP signal.
            */
            struct sigaction act;
            sigemptyset(& act.sa_mask);
            act.sa_flags=0;
            act.sa_handler=SIG_IGN;
            sigaction(SIGHUP, &act, 0);

            /*
              Make fishd save and exit on the TERM signal.
            */
            sigfillset(& act.sa_mask);
            act.sa_flags=0;
            act.sa_handler=&handle_term;
            sigaction(SIGTERM, &act, 0);
            break;

        }

        default:
        {
            debug(0, L"Parent process exiting (This is normal)");
            exit(0);
        }
    }

    /*
      Put ourself in out own processing group
    */
    setsid();

    /*
      Close stdin and stdout. We only use stderr, anyway.
    */
    close(0);
    close(1);

}

/**
   Get environment variable value.
*/
static env_var_t fishd_env_get(const char *key)
{
    const char *env = getenv(key);
    if (env != NULL)
    {
        return env_var_t(str2wcstring(env));
    }
    else
    {
        const wcstring wkey = str2wcstring(key);
        const wchar_t *tmp = env_universal_common_get(wkey);
        return tmp ? env_var_t(tmp) : env_var_t::missing_var();
    }
}

/**
   Get the configuration directory. The resulting string needs to be
   free'd. This is mostly the same code as path_get_config(), but had
   to be rewritten to avoid dragging in additional library
   dependencies.
*/
static wcstring fishd_get_config()
{
    bool done = false;
    wcstring result;

    env_var_t xdg_dir = fishd_env_get("XDG_CONFIG_HOME");
    if (! xdg_dir.missing_or_empty())
    {
        result = xdg_dir;
        append_path_component(result, L"/fish");
        if (!create_directory(result))
        {
            done = true;
        }
    }
    else
    {
        env_var_t home = fishd_env_get("HOME");
        if (! home.missing_or_empty())
        {
            result = home;
            append_path_component(result, L"/.config/fish");
            if (!create_directory(result))
            {
                done = 1;
            }
        }
    }

    if (! done)
    {
        /* Bad juju */
        debug(0, _(L"Unable to create a configuration directory for fish. Your personal settings will not be saved. Please set the $XDG_CONFIG_HOME variable to a directory where the current user has write access."));
        result.clear();
    }

    return result;
}

/**
   Load or save all variables
*/
static bool load_or_save_variables_at_path(bool save, const std::string &path)
{
    bool result = false;

    debug(4, L"Open file for %s: '%s'",
          save?"saving":"loading",
          path.c_str());

    /* OK to not use CLO_EXEC here because fishd is single threaded */
    int fd = open(path.c_str(), save?(O_CREAT | O_TRUNC | O_WRONLY):O_RDONLY, 0600);
    if (fd >= 0)
    {
        /* Success */
        result = true;
        connection_t c(fd);

        if (save)
        {
            /* Save to the file */
            write_loop(c.fd, SAVE_MSG, strlen(SAVE_MSG));
            enqueue_all(&c);
        }
        else
        {
            /* Read from the file */
            read_message(&c);
        }

        connection_destroy(&c);
    }
    return result;
}


static std::string get_variables_file_path(const std::string &dir, const std::string &identifier)
{
    std::string name;
    name.append(dir);
    name.append("/");
    name.append(FILE);
    name.append(identifier);
    return name;
}

static bool load_or_save_variables(bool save)
{
    const wcstring wdir = fishd_get_config();
    const std::string dir = wcs2string(wdir);
    if (dir.empty())
        return false;

    const std::string machine_id = get_machine_identifier();
    const std::string machine_id_path = get_variables_file_path(dir, machine_id);
    bool success = load_or_save_variables_at_path(save, machine_id_path);
    if (! success && ! save && errno == ENOENT)
    {
        /* We failed to load, because the file was not found. Older fish used the hostname only. Try *moving* the filename based on the hostname into place; if that succeeds try again. Silently "upgraded." */
        std::string hostname_id;
        if (get_hostname_identifier(&hostname_id) && hostname_id != machine_id)
        {
            std::string hostname_path = get_variables_file_path(dir, hostname_id);
            if (0 == rename(hostname_path.c_str(), machine_id_path.c_str()))
            {
                /* We renamed - try again */
                success = load_or_save_variables_at_path(save, machine_id_path);
            }
        }
    }
    return success;
}

/**
   Load variables from disk
*/
static void load()
{
    load_or_save_variables(false /* load, not save */);
}

/**
   Save variables to disk
*/
static void save()
{
    load_or_save_variables(true /* save, not load */);
}

/**
   Do all sorts of boring initialization.
*/
static void init()
{

    sock = get_socket();
    daemonize();
    env_universal_common_init(&broadcast);

    load();
}

/**
   Clean up behind ourselves
*/
static void cleanup()
{
    if (unlink(get_old_socket_filename().c_str()) != 0)
    {
        debug(0, L"Could not remove legacy socket path");
        wperror(L"unlink");
    }
}

/**
   Main function for fishd
*/
int main(int argc, char ** argv)
{
    int child_socket;
    struct sockaddr_un remote;
    socklen_t t;
    int max_fd;
    int update_count=0;

    fd_set read_fd, write_fd;

    set_main_thread();
    setup_fork_guards();

    program_name=L"fishd";
    wsetlocale(LC_ALL, L"");

    /*
      Parse options
    */
    while (1)
    {
        static struct option
                long_options[] =
        {
            {
                "help", no_argument, 0, 'h'
            }
            ,
            {
                "version", no_argument, 0, 'v'
            }
            ,
            {
                0, 0, 0, 0
            }
        }
        ;

        int opt_index = 0;

        int opt = getopt_long(argc,
                              argv,
                              GETOPT_STRING,
                              long_options,
                              &opt_index);

        if (opt == -1)
            break;

        switch (opt)
        {
            case 0:
                break;

            case 'h':
                print_help(argv[0], 1);
                exit(0);

            case 'v':
                debug(0, L"%ls, version %s\n", program_name, FISH_BUILD_VERSION);
                exit(0);

            case '?':
                return 1;

        }
    }

    init();
    while (1)
    {
        int res;

        t = sizeof(remote);

        FD_ZERO(&read_fd);
        FD_ZERO(&write_fd);
        FD_SET(sock, &read_fd);
        max_fd = sock+1;
        for (connection_list_t::const_iterator iter = connections.begin(); iter != connections.end(); ++iter)
        {
            const connection_t &c = *iter;
            FD_SET(c.fd, &read_fd);
            max_fd = maxi(max_fd, c.fd+1);

            if (! c.unsent.empty())
            {
                FD_SET(c.fd, &write_fd);
            }
        }

        while (1)
        {
            res=select(max_fd, &read_fd, &write_fd, 0, 0);

            if (quit)
            {
                save();
                cleanup();
                exit(0);
            }

            if (res != -1)
                break;

            if (errno != EINTR)
            {
                wperror(L"select");
                cleanup();
                exit(1);
            }
        }

        if (FD_ISSET(sock, &read_fd))
        {
            if ((child_socket =
                        accept(sock,
                               (struct sockaddr *)&remote,
                               &t)) == -1)
            {
                wperror(L"accept");
                cleanup();
                exit(1);
            }
            else
            {
                debug(4, L"Connected with new child on fd %d", child_socket);

                if (make_fd_nonblocking(child_socket) != 0)
                {
                    wperror(L"fcntl");
                    close(child_socket);
                }
                else
                {
                    connections.push_front(connection_t(child_socket));
                    connection_t &newc = connections.front();
                    send(newc.fd, GREETING, strlen(GREETING), MSG_DONTWAIT);
                    enqueue_all(&newc);
                }
            }
        }

        for (connection_list_t::iterator iter = connections.begin(); iter != connections.end(); ++iter)
        {
            if (FD_ISSET(iter->fd, &write_fd))
            {
                try_send_all(&*iter);
            }
        }

        for (connection_list_t::iterator iter = connections.begin(); iter != connections.end(); ++iter)
        {
            if (FD_ISSET(iter->fd, &read_fd))
            {
                read_message(&*iter);

                /*
                  Occasionally we save during normal use, so that we
                  won't lose everything on a system crash
                */
                update_count++;
                if (update_count >= 64)
                {
                    save();
                    update_count = 0;
                }
            }
        }

        for (connection_list_t::iterator iter = connections.begin(); iter != connections.end();)
        {
            if (iter->killme)
            {
                debug(4, L"Close connection %d", iter->fd);

                while (! iter->unsent.empty())
                {
                    message_t *msg = iter->unsent.front();
                    iter->unsent.pop();
                    msg->count--;
                    if (! msg->count)
                        free(msg);
                }

                connection_destroy(&*iter);
                iter = connections.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        if (connections.empty())
        {
            debug(0, L"No more clients. Quitting");
            save();
            cleanup();
            break;
        }

    }
}

