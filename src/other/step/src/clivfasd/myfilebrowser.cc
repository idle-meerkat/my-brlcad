/*
 * Copyright (c) 1987, 1988, 1989 Stanford University
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Stanford not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Stanford makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * STANFORD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL STANFORD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * MyFileBrowser implementation.
 */
#include <string.h>
#include <memory.h>
#include <stdlib.h> // for bcopy() function
#include <stream.h>
#include <myfilebrowser.h>

//#include "/proj/pdevel/autoc/arch.sunCC.iv/scl_cf.h"
// This is an scl configuration file generated by the configure script 
// generated using gnu's autoconf
#include <scl_cf.h>

#ifdef HAVE_SYSENT_H
#include <sysent.h>
#endif

#ifdef HAVE_UNISTD_H
// needed at least for getuid()
#include <unistd.h>
#endif

// sys/stat.h is apparently needed for ObjectCenter?
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

// to help ObjectCenter
#ifndef HAVE_MEMMOVE
extern "C"
{
void * memmove(void *__s1, const void *__s2, size_t __n);
}
#endif

//#ifdef __GNUC__
// needed at least for getuid()
//#include <unistd.h>
//#endif

//#ifdef __SUNCPLUSPLUS__
// needed at least for getuid()
//#include <sysent.h>
//#include <unistd.h>
//#endif

//#ifdef __OBJECTCENTER__
// needed at least for getuid()
//#include <sysent.h>
//#include <sys/stat.h>

//extern "C"
//{
//void * memmove(void *__s1, const void *__s2, size_t __n);
//}
//#endif

#include <pwd.h>
//#include <os/auth.h>
//#include <os/fs.h>
#include <sys/param.h>

#include <scldir.h>
#include <stat.h>

//#include <sys/dir.h>
//#include <sys/stat.h>
//#include <InterViews/Std/sys/dir.h>
//#include <InterViews/Std/sys/stat.h>

/*****************************************************************************/

class MyFBDirectory {       
public:
    MyFBDirectory(const char* name);
    virtual ~MyFBDirectory();

    boolean LoadDirectory(const char*);
    const char* Normalize(const char*);
    const char* ValidDirectories(const char*);

    int Index(const char*);
    const char* File(int index);
    int Count();

    boolean IsADirectory(const char*);
private:
    const char* Home(const char* = nil);
    const char* ElimDot(const char*);
    const char* ElimDotDot(const char*);
    const char* InterpSlashSlash(const char*);
    const char* InterpTilde(const char*);
    const char* ExpandTilde(const char*, int);
    const char* RealPath(const char*);

    boolean Reset(const char*);
    void Clear();
    void Check(int index);
    void Insert(const char*, int index);
    void Append(const char*);
    void Remove(int index);
    virtual int Position(const char*);
private:
    char** strbuf;
    int strcount;
    int strbufsize;
};

inline int MyFBDirectory::Count () { return strcount; }
inline void MyFBDirectory::Append (const char* s) { Insert(s, strcount); }
inline const char* MyFBDirectory::File (int index) { 
    return (0 <= index && index < strcount) ? strbuf[index] : nil;
}

// evidently strdup is defined with sun but not with gnu, changed to not use it
//#ifndef __SUNCPLUSPLUS__
//char* strdup (const char* s) {
//#endif
char* mystrdup (const char* s) {
    char* dup = new char[strlen(s) + 1];
    strcpy(dup, s);
    return dup;
}

MyFBDirectory::MyFBDirectory (const char* name) {
    const int defaultSize = 256;

    strbufsize = defaultSize;
    strbuf = new char*[strbufsize];
    strcount = 0;
    LoadDirectory(name);
}

MyFBDirectory::~MyFBDirectory () {
    Clear();
}

const char* MyFBDirectory::RealPath (const char* path) {
    const char* realpath;

    if (*path == '\0') {
        realpath = "./";
    } else {
        realpath = InterpTilde(InterpSlashSlash(path));
    }
    return realpath;
}

boolean MyFBDirectory::LoadDirectory (const char* name) {
    char buf[MAXPATHLEN+2];
    const char* path = buf;

    strcpy(buf, ValidDirectories(RealPath(name)));
    return Reset(buf);
}

int MyFBDirectory::Index (const char* name) {
    for (int i = 0; i < strcount; ++i) {
        if (strcmp(strbuf[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

boolean MyFBDirectory::Reset (const char* path) {
    boolean successful = IsADirectory(path);
    if (successful) {
	DIR* dir = opendir(path);
        Clear();

        for (struct dirent* d = readdir(dir); d != NULL; d = readdir(dir)) {
/*
//#if defined(SYSV)
//        for (struct dirent* d = readdir(dir); d != NULL; d = readdir(dir)) {
//#else
//        for (struct direct* d = readdir(dir); d != NULL; d = readdir(dir)) {
//#endif
*/
            Insert(d->d_name, Position(d->d_name));
        }
        closedir(dir);
    }
    return successful; 
}

boolean MyFBDirectory::IsADirectory (const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

const char* MyFBDirectory::Home (const char* name) {
    struct passwd* pw =
        (name == nil) ? getpwuid(getuid()) : getpwnam((char*)name);
    return (pw == nil) ? nil : pw->pw_dir;
}

inline boolean DotSlash (const char* path) {
    return 
        path[0] != '\0' && path[0] == '.' &&
        (path[1] == '/' || path[1] == '\0');
}

inline boolean DotDotSlash (const char* path) {
    return 
        path[0] != '\0' && path[1] != '\0' &&
        path[0] == '.' && path[1] == '.' &&
        (path[2] == '/' || path[2] == '\0');
}

const char* MyFBDirectory::Normalize (const char* path) {
    static char newpath[MAXPATHLEN+1];
    const char* buf;

    buf = InterpSlashSlash(path);
    buf = ElimDot(buf);
    buf = ElimDotDot(buf);
    buf = InterpTilde(buf);

    if (*buf == '\0') {
        strcpy(newpath, "./");

    } else if (!DotSlash(buf) && !DotDotSlash(buf) && *buf != '/') {
        strcpy(newpath, "./");
        strcat(newpath, buf);

    } else if (IsADirectory(buf) && buf[strlen(buf)-1] != '/') {
        strcpy(newpath, buf);
        strcat(newpath, "/");

    } else {
        strcpy(newpath, buf);
    }
    return newpath;
}

const char* MyFBDirectory::ValidDirectories (const char* path) {
    static char buf[MAXPATHLEN+1];
    strcpy(buf, path);
    int i = strlen(path);

    while (!IsADirectory(RealPath(buf)) && i >= 0) {
        for (--i; buf[i] != '/' && i >= 0; --i);
        buf[i+1] = '\0';
    }
    return buf;
}

const char* MyFBDirectory::InterpSlashSlash (const char* path) {
    for (int i = strlen(path)-1; i > 0; --i) {
        if (path[i] == '/' && path[i-1] == '/') {
            return &path[i];
        }
    }
    return path;
}

const char* MyFBDirectory::ElimDot (const char* path) {
    static char newpath[MAXPATHLEN+1];
    const char* src;
    char* dest = newpath;

    for (src = path; src < &path[strlen(path)]; ++src) {
        if (!DotSlash(src)) {
            *dest++ = *src;

        } else if (*(dest-1) == '/') {
            ++src;

        } else {
            *dest++ = *src;
        }            
    }
    *dest = '\0';
    return newpath;
}

static boolean CollapsedDotDotSlash (const char* path, const char*& start) {
    if (path == start || *(start-1) != '/') {
        return false;

    } else if (path == start-1 && *path == '/') {
        return true;

    } else if (path == start-2) {               // NB: won't handle '//' right
        start = path;
        return *start != '.';

    } else if (path < start-2 && !DotDotSlash(start-3)) {
        for (start -= 2; path <= start; --start) {
            if (*start == '/') {
                ++start;
                return true;
            }
        }
        start = path;
        return true;
    }
    return false;
}

const char* MyFBDirectory::ElimDotDot (const char* path) {
    static char newpath[MAXPATHLEN+1];
    const char* src;
    char* dest = newpath;

    for (src = path; src < &path[strlen(path)]; ++src) {
        if (DotDotSlash(src) && CollapsedDotDotSlash(newpath, 
						     (const char *&)dest)) {
// DAS PORT gnu 2.6				     (const char *)dest)) {
            src += 2;
        } else {
            *dest++ = *src;
        }
    }
    *dest = '\0';
    return newpath;
}

const char* MyFBDirectory::InterpTilde (const char* path) {
    static char realpath[MAXPATHLEN+1];
    const char* beg = strrchr(path, '~');
    boolean validTilde = beg != NULL && (beg == path || *(beg-1) == '/');

    if (validTilde) {
        const char* end = strchr(beg, '/');
        int length = (end == nil) ? strlen(beg) : end - beg;
        const char* expandedTilde = ExpandTilde(beg, length);

        if (expandedTilde == nil) {
            validTilde = false;
        } else {
            strcpy(realpath, expandedTilde);
            if (end != nil) {
                strcat(realpath, end);
            }
        }
    }
    return validTilde ? realpath : path;
}

const char* MyFBDirectory::ExpandTilde (const char* tildeName, int length) {
    const char* name = nil;

    if (length > 1) {
        static char buf[MAXNAMLEN+1];
        strncpy(buf, tildeName+1, length-1);
        buf[length-1] = '\0';
        name = buf;
    }
    return Home(name);
}        

void MyFBDirectory::Check (int index) {
    char** newstrbuf;

    if (index >= strbufsize) {
        strbufsize = (index+1) * 2;
        newstrbuf = new char*[strbufsize];
//        bcopy(strbuf, newstrbuf, strcount*sizeof(char*)); // non ANSI 
	// the 1st two operands are reversed from bcopy. memcpy() is ANSI 
 	// memcpy is o.k. since memory areas do not overlap - DAS
        memcpy(newstrbuf, strbuf, strcount*sizeof(char*)); // ANSI 
        delete strbuf;
        strbuf = newstrbuf;
    }
}

// this makes a copy of 's' before it inserts it.
void MyFBDirectory::Insert (const char* s, int index) {
    char** spot;
    index = (index < 0) ? strcount : index;

    if (index < strcount) {
        Check(strcount+1);
        spot = &strbuf[index];
//        bcopy(spot, spot+1, (strcount - index)*sizeof(char*)); // non ANSI 
	// the 1st two operands are reversed from bcopy. memcpy() is ANSI 
 	// memcpy is not always o.k. since memory areas overlap - DAS
//        memcpy(spot+1, spot, (strcount - index)*sizeof(char*)); // ANSI
        memmove(spot+1, spot, (strcount - index)*sizeof(char*)); // ANSI
     } else {
        Check(index);
        spot = &strbuf[index];
    }
    char* string = mystrdup(s);
    *spot = string;
    ++strcount;
}

void MyFBDirectory::Remove (int index) {
    if (index < --strcount) {
        char** spot = &strbuf[index];
        delete *spot;
//          bcopy(spot+1, spot, (strcount - index)*sizeof(char*));
	// the 1st two operands are reversed from bcopy. memcpy() is ANSI 
 	// memcpy is not always o.k. since memory areas overlap - DAS
//        memcpy(spot, spot+1, (strcount - index)*sizeof(char*)); // ANSI
        memmove(spot, spot+1, (strcount - index)*sizeof(char*)); // ANSI
    }
}

void MyFBDirectory::Clear () {
    for (int i = 0; i < strcount; ++i) {
        delete strbuf[i];
    }
    strcount = 0;
}

int MyFBDirectory::Position (const char* s) {
    int i;

    for (i = 0; i < strcount; ++i) {
        if (strcmp(s, strbuf[i]) < 0) {
            return i;
        }
    }
    return strcount;
}

/*****************************************************************************/

MyFileBrowser::MyFileBrowser (
    ButtonState* bs, const char* dir_, int r, int c,
    boolean u, int h, const char* d, int restrictBool
) : StringBrowser(bs, r, c, u, h, d) {

    // set restrict before Update() is called
    restrict = restrictBool;
    Init(dir_);
    Update();
}

MyFileBrowser::MyFileBrowser (
    const char* name, ButtonState* bs, const char* dir_, int r, int c,
    boolean u, int h, const char* d, int restrictBool
) : StringBrowser(name, bs, r, c, u, h, d) {

    // set restrict before Update() is called
    restrict = restrictBool;
    Init(dir_);
    Update();
}

void MyFileBrowser::Init (const char* d) {
    dir = new MyFBDirectory(d);
    lastpath = mystrdup(ValidDirectories(Normalize(d)));
}

MyFileBrowser::~MyFileBrowser () {
    delete dir;
    delete lastpath;
}

static const char* Concat (const char* path, const char* file) {
    static char buf[MAXPATHLEN+1];

    strcpy(buf, path);
    if (path[strlen(path)-1] != '/') {
        strcat(buf, "/");
    }
    return strcat(buf, file);
}

boolean MyFileBrowser::IsADirectory (const char* path) {
    return dir->IsADirectory(Normalize(path));
}

boolean MyFileBrowser::SetDirectory (const char* path) {
    boolean successful = true;
    path = ValidDirectories(path);
    const char* normpath = Normalize(path);

    if (strcmp(normpath, lastpath) != 0) {
        char* newnormpath = mystrdup(normpath);
        successful = dir->LoadDirectory(newnormpath);

        if (successful) {
            delete lastpath;
            lastpath = newnormpath;
            Update();
        } else {
            delete newnormpath;
        }
    }
    return successful;
}

const char* MyFileBrowser::ValidDirectories (const char* path) {
    return dir->ValidDirectories(path);
}

const char* MyFileBrowser::Normalize (const char* path) {
    return dir->Normalize(path);
}

const char* MyFileBrowser::Path (int index) {
    const char* s = String(index);

    return (s == nil ) ? nil : Normalize(Concat(lastpath, s));
}

void MyFileBrowser::UpdateDir()
{
    boolean successful = dir->LoadDirectory(lastpath);
    if(successful)
	Update();
}

void MyFileBrowser::Update () {
    Clear();

    for (int i = 0; i < dir->Count(); ++i) {
        const char* name = dir->File(i);

        if (dir->IsADirectory(Concat(lastpath, name))) {
            char buf[MAXPATHLEN+1];
            strcpy(buf, name);
            strcat(buf, "/");
//	    if(buf[0] != '.' || !restrict)
	    if(!restrict)
		Append(buf);
        } else {
	    if(name[0] != '.' || !restrict)
		Append(name);
        }
    }
}
