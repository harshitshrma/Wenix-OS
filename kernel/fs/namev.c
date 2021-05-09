
#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"


/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
void getnormalpath(const char *old);
char final[MAXPATHLEN];

int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        //NOT_YET_IMPLEMENTED("VFS: lookup");
        dbg(DBG_TEMP, "Lookup entry\n");
        KASSERT(NULL != dir); /* the "dir" argument must be non-NULL */
        KASSERT(NULL != name); /* the "name" argument must be non-NULL */
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        if (dir->vn_ops->lookup == NULL)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            dbg(DBG_TEMP, "Lookup exit\n");
            return -ENOTDIR;  
        }
        const char *temp = ".";
        if (!strcmp(name,temp))
        {
            *result = dir;
            vref(*result);
            dbg(DBG_PRINT, "(GRADING2B)\n");
            dbg(DBG_TEMP, "Lookup exit\n");
            return 0;
        }

        int newlook = dir->vn_ops->lookup(dir, name, len, result);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        dbg(DBG_TEMP, "Lookup exit\n");
        return newlook;
    
}

/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        //NOT_YET_IMPLEMENTED("VFS: dir_namev");
        dbg(DBG_TEMP, "dir_namev entry\n");
        int res=1;
        int error=0;
        int next=0;
        int start=0;
        int curlen=0;
        KASSERT(NULL != pathname); /* the "pathname" argument must be non-NULL */
        KASSERT(NULL != namelen); /* the "namelen" argument must be non-NULL */
        KASSERT(NULL != name); /* the "name" argument must be non-NULL */
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        int length = strlen(pathname);

        if(*pathname=='\0')
        {
            *res_vnode=NULL;
            dbg(DBG_PRINT, "(GRADING2B)\n");
            dbg(DBG_TEMP, "dir_namev exit\n");
            return -EINVAL;
        }

        vnode_t *parnode=NULL;
        vnode_t *curnode=NULL;

        if(*pathname=='/')
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            curnode = vfs_root_vn;
            while(*pathname == '/')
            {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                pathname++; 
            }
        }
        else if (base==NULL)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            curnode= curproc->p_cwd;
        }

        vref(curnode);

        if(*pathname=='\0')
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            *namelen=1;
            *name=".";
            *res_vnode = curnode;
            dbg(DBG_TEMP, "dir_namev exit\n");
            return 0;
        }

        while(res >=0 && pathname[next] != '\0')
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            if(parnode!=NULL)
            {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                vput(parnode);
            }

            parnode = curnode;
            start = next;

            //finding end of current name
            while(pathname[next] != '/' && pathname[next] != '\0')
            {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                next++;
            }

            curlen=next-start;

            if(next-start>NAME_LEN)
            {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                error=-ENAMETOOLONG;
                break;
            }

            res = lookup(parnode,(pathname+start),next-start,&curnode);
            if(res==-ENOTDIR)
            {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                error=-ENOTDIR;
                break;
            }

            while(pathname[next]=='/')
            {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                next++;
            }

        }
        if(res<0 && res!=-ENOENT)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(parnode);
            dbg(DBG_TEMP, "dir_namev exit\n");
            return res;
        }
        else if(error!=0)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(parnode);
            dbg(DBG_TEMP, "dir_namev exit\n");
            return error;
        }
        else if(pathname[next]!='\0')
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(parnode);
            dbg(DBG_TEMP, "dir_namev exit\n");
            return -ENOENT;
        }

        if(res==0)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(curnode);
        }

        *namelen = curlen;
        *name=(pathname+start);
        *res_vnode=parnode;
        dbg(DBG_PRINT, "(GRADING2B)\n");
        dbg(DBG_TEMP, "dir_namev exit\n");
        return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <wenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        //NOT_YET_IMPLEMENTED("VFS: open_namev");
    dbg(DBG_TEMP, "open_namev entry\n");
    size_t namelen;

    const char *name;
    vnode_t *parnode =  NULL;


    //int error = 0;
    int res = 0;
    res = dir_namev(pathname,&namelen,&name, base, &parnode);
    if (!res)
    {
        dbg(DBG_PRINT, "(GRADING2B)\n");
        res = lookup(parnode, name, namelen, res_vnode);
        if (!res)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(parnode);
            dbg(DBG_TEMP, "open_namev exit\n");
            return 0;
        }
        else if (res == -ENOENT && (flag & O_CREAT)) 
        { 
            KASSERT(NULL != parnode->vn_ops->create);
            dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
            dbg(DBG_PRINT, "(GRADING2B)\n");
            int newfile = parnode->vn_ops->create(parnode, name, namelen, res_vnode);
            vput(parnode);
            dbg(DBG_TEMP, "open_namev exit\n");
            return 0;
        }
        else if(res!=0)
        {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(parnode);
            dbg(DBG_TEMP, "open_namev exit\n");
            return res;
        }
    }
    dbg(DBG_PRINT, "(GRADING2B)\n");
    dbg(DBG_TEMP, "open_namev exit\n");
    return res;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
