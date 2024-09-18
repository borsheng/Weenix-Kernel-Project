/******************************************************************************/
/* Important Spring 2024 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

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
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        KASSERT(NULL != dir); /* the "dir" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        KASSERT(NULL != name); /* the "name" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        KASSERT(NULL != result); /* the "result" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        if (dir->vn_ops->lookup == NULL) {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                // the directory does not support lookup operations
                return -ENOTDIR;
        }

        if (len > NAME_LEN) {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                // name length exceeds the max allowed
                return -ENAMETOOLONG;
        }

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return dir->vn_ops->lookup(dir, name, len, result);
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
        KASSERT(NULL != pathname); /* the "pathname" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        KASSERT(NULL != namelen); /* the "namelen" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        KASSERT(NULL != name); /* the "name" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        KASSERT(NULL != res_vnode); /* the "res_vnode" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        vnode_t *dir_vnode;
        if (base != NULL) {
                // use base if provide
                dir_vnode = base;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        } else {
                // use current working directory 
                dir_vnode = curproc->p_cwd;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        int path_index = 0;
        int ret = 0;
        int pathname_len = strlen(pathname);

        // handle case where pathname starts with '/'.
        if (pathname[0] == '/') {
                // set to root vnode for absolute paths.
                dir_vnode = vfs_root_vn;
                path_index = 1;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        KASSERT(NULL != dir_vnode); /* pathname resolution must start with a valid directory */
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        vref(dir_vnode);

        // start index of basename in pathname
        int start_index = path_index;
        vnode_t *previous_vnode = NULL;

        for (; path_index < pathname_len; path_index++) {
                if (pathname[path_index] == '/') {
                        previous_vnode = dir_vnode;
                        int segment_length = path_index - start_index;
                        ret = lookup(previous_vnode, pathname + start_index, segment_length, &dir_vnode);

                        if (ret < 0) {
                                // release the vnode if lookup fails
                                vput(previous_vnode);
                                dbg(DBG_PRINT, "(GRADING2B)\n");
                                return ret;
                        }

                        *namelen = (size_t)(segment_length);
                        *name = pathname + start_index;

                        // ensure the found vnode is a directory
                        if (!S_ISDIR(dir_vnode->vn_mode)) {
                                // release both vnodes if not a directory
                                vput(previous_vnode);
                                vput(dir_vnode);
                                dbg(DBG_PRINT, "(GRADING2B)\n");
                                return -ENOTDIR;
                        }

                        // skip consecutive slashes in the path
                        for (; path_index < pathname_len && pathname[path_index+1] == '/'; path_index++) {
                                dbg(DBG_PRINT, "(GRADING2B)\n");
                        }

                        if (path_index < pathname_len) {
                                if (pathname[path_index + 1] == '\0') {
                                        *res_vnode = previous_vnode;
                                        vput(dir_vnode);
                                        dbg(DBG_PRINT, "(GRADING2B)\n");
                                        return ret;
                                }
                                dbg(DBG_PRINT, "(GRADING2B)\n");
                        }

                        start_index = path_index+1;
                        vput(previous_vnode);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        // set the result vnode to the last found directory
        *res_vnode = dir_vnode;

        // if the basename is empty, set it to '.' (current directory)
        if (previous_vnode == NULL && path_index == start_index) {
            *namelen = 1;
            *name = ".";
            dbg(DBG_PRINT, "(GRADING2B)\n");
        } else if ((size_t)(path_index - start_index) > NAME_LEN) {
            vput(dir_vnode);
            dbg(DBG_PRINT, "(GRADING2B)\n");
            return -ENAMETOOLONG;
        } else {
            *namelen = (size_t)(path_index - start_index);
            *name = pathname + start_index;
            dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return ret;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
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
        // invalid pathname length
        if (strlen(pathname) == 0 || strlen(pathname) > MAXPATHLEN) {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }

        vnode_t *dir_vnode = NULL;
        size_t name_length = 0;
        const char *entry_name = NULL; 

        int result = dir_namev(pathname, &name_length, &entry_name, base, &dir_vnode);
        int operation_result;

        if (result < 0) {
                // fail to resolve parent directory
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return result;
        } else {
                // attempt to lookup the file within resolved directory
                operation_result = lookup(dir_vnode, entry_name, name_length, res_vnode);
                
                if (operation_result == -ENOENT && (flag & O_CREAT)) {
                        KASSERT(NULL != dir_vnode->vn_ops->create);
                        dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
                        dbg(DBG_PRINT, "(GRADING2B)\n");

                        // create file
                        operation_result = dir_vnode->vn_ops->create(dir_vnode, entry_name, name_length, res_vnode);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        if (name_length != strlen(entry_name)) {
                if (!S_ISDIR((*res_vnode)->vn_mode)) {
                    vput(*res_vnode);
                    vput(dir_vnode);
                    dbg(DBG_PRINT, "(GRADING2B)\n");
                    return -ENOTDIR;
                }
        }

        dbg(DBG_PRINT, "(GRADING2B)\n");
        // release the parent directory vnode
        vput(dir_vnode);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        // return the result of lookup or create operation
        return operation_result;
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
