/** \file wutil.c
	Wide character equivalents of various standard unix
	functions.
*/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>
#include <string>


#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

#include "fallback.h"
#include "util.h"

#include "common.h"
#include "wutil.h"

typedef std::string cstring;

/**
   Minimum length of the internal covnersion buffers
*/
#define TMP_LEN_MIN 256

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
/**
   Fallback length of MAXPATHLEN. Just a hopefully sane value...
*/
#define PATH_MAX 4096
#endif
#endif

/**
   For wgettext: Number of string_buffer_t in the ring of buffers
*/
#define BUFF_COUNT 4

/**
   For wgettext: The ring of string_buffer_t
*/
static string_buffer_t buff[BUFF_COUNT];
/**
   For wgettext: Current position in the ring
*/
static int curr_buff=0;

/**
   For wgettext: Buffer used by translate_wcs2str
*/
static char *wcs2str_buff=0;
/**
   For wgettext: Size of buffer used by translate_wcs2str
*/
static size_t wcs2str_buff_count=0;


void wutil_init()
{
}

void wutil_destroy()
{
}

bool wreaddir_resolving(DIR *dir, const std::wstring &dir_path, std::wstring &out_name, bool *out_is_dir)
{
    struct dirent *d = readdir( dir );
    if ( !d ) return false;
    
    out_name = str2wcstring(d->d_name);
    if (out_is_dir) {
        bool is_dir;
        if (d->d_type == DT_DIR) {
            is_dir = true;
        } else if (d->d_type == DT_LNK) {
            /* We want to treat symlinks to directories as directories. Use stat to resolve it. */
            cstring fullpath = wcs2string(dir_path);
            fullpath.push_back('/');
            fullpath.append(d->d_name);
            struct stat buf;
            if (stat(fullpath.c_str(), &buf) != 0) {
                is_dir = false;
            } else {
                is_dir = !! (S_ISDIR(buf.st_mode));
            }
        } else {
            is_dir = false;
        }
        *out_is_dir = is_dir;
    }
    return true;
}

bool wreaddir(DIR *dir, std::wstring &out_name)
{
    struct dirent *d = readdir( dir );
    if ( !d ) return false;
    
    out_name = str2wcstring(d->d_name);
    return true;
}


wchar_t *wgetcwd( wchar_t *buff, size_t sz )
{
	char *buffc = (char *)malloc( sz*MAX_UTF8_BYTES);
	char *res;
	wchar_t *ret = 0;
		
	if( !buffc )
	{
		errno = ENOMEM;
		return 0;
	}
	
	res = getcwd( buffc, sz*MAX_UTF8_BYTES );
	if( res )
	{
		if( (size_t)-1 != mbstowcs( buff, buffc, sizeof( wchar_t ) * sz ) )
		{
			ret = buff;
		}	
	}
	
	free( buffc );
	
	return ret;
}

int wchdir( const wcstring &dir )
{
    cstring tmp = wcs2string(dir);
	return chdir( tmp.c_str() );
}

FILE *wfopen(const wcstring &path, const char *mode)
{
	cstring tmp = wcs2string(path);
	return fopen(tmp.c_str(), mode);
}

FILE *wfreopen(const wcstring &path, const char *mode, FILE *stream)
{
    cstring tmp = wcs2string(path);
    return freopen(tmp.c_str(), mode, stream);
}

int wopen(const wcstring &pathname, int flags, ...)
{
    cstring tmp = wcs2string(pathname);
	int res=-1;
	va_list argp;	
    if( ! (flags & O_CREAT) )
    {
        res = open(tmp.c_str(), flags);
    }
    else
    {
        va_start( argp, flags );
        res = open(tmp.c_str(), flags, va_arg(argp, int) );
        va_end( argp );
    }
    return res;
}

int wcreat(const wcstring &pathname, mode_t mode)
{
    cstring tmp = wcs2string(pathname);
    return creat(tmp.c_str(), mode);
}

DIR *wopendir(const wcstring &name)
{
    cstring tmp = wcs2string(name);
    return opendir(tmp.c_str());
}

int wstat(const wcstring &file_name, struct stat *buf)
{
    cstring tmp = wcs2string(file_name);
    return stat(tmp.c_str(), buf);
}

int lwstat(const wcstring &file_name, struct stat *buf)
{
   // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    cstring tmp = wcs2string(file_name);
    return lstat(tmp.c_str(), buf);
}


int waccess(const wcstring &file_name, int mode)
{
    cstring tmp = wcs2string(file_name);
    return access(tmp.c_str(), mode);
}

int wunlink(const wcstring &file_name)
{
    cstring tmp = wcs2string(file_name);
    return unlink(tmp.c_str());
}

void wperror(const wcstring &s)
{
	int e = errno;
	if( !s.empty() )
	{
		fwprintf( stderr, L"%ls: ", s.c_str() );
	}
	fwprintf( stderr, L"%s\n", strerror( e ) );
}

#ifdef HAVE_REALPATH_NULL

wchar_t *wrealpath(const wcstring &pathname, wchar_t *resolved_path)
{
	cstring tmp = wcs2string(pathname);
	char *narrow_res = realpath( tmp.c_str(), 0 );	
	wchar_t *res;	

	if( !narrow_res )
		return 0;
		
	if( resolved_path )
	{
		wchar_t *tmp2 = str2wcs( narrow_res );
		wcslcpy( resolved_path, tmp2, PATH_MAX );
		free( tmp2 );
		res = resolved_path;		
	}
	else
	{
		res = str2wcs( narrow_res );
	}

    free( narrow_res );

	return res;
}

#else

wchar_t *wrealpath(const wchar_t *pathname, wchar_t *resolved_path)
{
    cstring tmp = wcs2string(pathname);
	char narrow_buff[PATH_MAX];
	char *narrow_res = realpath( tmp.c_str(), narrow_buff );
	wchar_t *res;	

	if( !narrow_res )
		return 0;
		
	if( resolved_path )
	{
		wchar_t *tmp2 = str2wcs( narrow_res );
		wcslcpy( resolved_path, tmp2, PATH_MAX );
		free( tmp2 );
		res = resolved_path;		
	}
	else
	{
		res = str2wcs( narrow_res );
	}
	return res;
}

#endif


wcstring wdirname( const wcstring &path )
{
    char *tmp = wcs2str(path.c_str());
    char *narrow_res = dirname( tmp );
    wcstring result = format_string(L"%s", narrow_res);
    free(tmp);
    return result;
}

wcstring wbasename( const wcstring &path )
{
    char *tmp = wcs2str(path.c_str());
    char *narrow_res = basename( tmp );
    wcstring result = format_string(L"%s", narrow_res);
    free(tmp);
    return result;
}

/* Really init wgettext */
static void wgettext_really_init() {
	for( size_t i=0; i<BUFF_COUNT; i++ )
	{
		sb_init( &buff[i] );
	}
	bindtextdomain( PACKAGE_NAME, LOCALEDIR );
	textdomain( PACKAGE_NAME );
}

/**
   For wgettext: Internal init function. Automatically called when a translation is first requested.
*/
static void wgettext_init_if_necessary()
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, wgettext_really_init);
}


/**
   For wgettext: Wide to narrow character conversion. Internal implementation that
   avoids exessive calls to malloc
*/
static char *wgettext_wcs2str( const wchar_t *in )
{
	size_t len = MAX_UTF8_BYTES*wcslen(in)+1;
	if( len > wcs2str_buff_count )
	{
		wcs2str_buff = (char *)realloc( wcs2str_buff, len );
		if( !wcs2str_buff )
		{
			DIE_MEM();
		}
	}
	
	return wcs2str_internal( in, wcs2str_buff);
}

const wchar_t *wgettext( const wchar_t *in )
{
    ASSERT_IS_MAIN_THREAD();

	if( !in )
		return in;
	
    wgettext_init_if_necessary();
	
	char *mbs_in = wgettext_wcs2str( in );	
	char *out = gettext( mbs_in );
	wchar_t *wres=0;
	
	sb_clear( &buff[curr_buff] );
	
	sb_printf( &buff[curr_buff], L"%s", out );
	wres = (wchar_t *)buff[curr_buff].buff;
	curr_buff = (curr_buff+1)%BUFF_COUNT;
	
	return wres;
}

wcstring wgettext2(const wcstring &in) {
    wgettext_init_if_necessary();
    std::string mbs_in = wcs2string(in);	
	char *out = gettext( mbs_in.c_str() );
    wcstring result = format_string(L"%s", out);
    return result;
}

const wchar_t *wgetenv( const wcstring &name )
{
    ASSERT_IS_MAIN_THREAD();
    cstring name_narrow = wcs2string(name);
	char *res_narrow = getenv( name_narrow.c_str() );
	static wcstring out;

	if( !res_narrow )
		return 0;
	
    out = format_string(L"%s", res_narrow);
	return out.c_str();
	
}

int wmkdir( const wcstring &name, int mode )
{
	cstring name_narrow = wcs2string(name);
	return mkdir( name_narrow.c_str(), mode );
}

int wrename( const wcstring &old, const wcstring &newv )
{
    cstring old_narrow = wcs2string(old);
	cstring new_narrow =wcs2string(newv);
	return rename( old_narrow.c_str(), new_narrow.c_str() );
}
