/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#define _GNU_SOURCE
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#include "aes-crypt.h"
#include <fuse.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define FOPS_DATA ((struct fs_state *) fuse_get_context()->private_data)
#define MAX_PATH 256
#define XATTRNAME "user.pa5-encfs.encrypted"
#define XATTRTRUE "true"
#define XATTRFALSE "false"

struct fs_state{
	char* mirrorDir;
	char* keyPhrase;
};

/* Helper function to construct the proper path */
static void createPath(const char *path, char buffer[MAX_PATH]){
	strcpy(buffer, FOPS_DATA->mirrorDir);
	strcat(buffer, path);
}

/* Helper function to see whether or not a file is encrypted */
static int isEncrypted(char fullPath[MAX_PATH]){
	char attrBuffer[8];
	
	int val = getxattr(fullPath, XATTRNAME, attrBuffer, sizeof(attrBuffer));
	if (val == 4){
		return 1;
	}
	else{
		return 0;
	}
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);
	

	res = lstat(buffer, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = access(buffer, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = readlink(buffer, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	dp = opendir(buffer);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(buffer, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(buffer, mode);
	else
		res = mknod(buffer, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = mkdir(buffer, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = unlink(buffer);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = rmdir(buffer);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = chmod(buffer, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = lchown(buffer, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = truncate(buffer, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(buffer, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = open(buffer, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	
	/* Local Vars */
	int fileDescriptor;
	int tempFileDescriptor;
	int res;
	FILE* inFile;
	FILE* outFile;
	char realPath[MAX_PATH];
	
	/* Get the proper path to use */
	createPath(path, realPath);
	int encryptChecker = isEncrypted(realPath);
	
	/* If our file is ENCRYPTED... */
	if (encryptChecker == 1){
		
		/* Open the files */
		inFile = fopen(realPath, "r");
		outFile = tmpfile();
		tempFileDescriptor = fileno(outFile);

		(void) fi;
		fileDescriptor = open(realPath, O_RDONLY);
		if (fileDescriptor == -1)
			return -errno;
		
		/* Decrypt the encrypted file */
		do_crypt(inFile, outFile, 0, FOPS_DATA->keyPhrase);
	
		fflush(outFile);
		
		/* Read the contents of the file */
		res = pread(tempFileDescriptor, buf, size, offset);
		if (res == -1)
			res = -errno;

		/* Close opened files */
		fclose(outFile);
		fclose(inFile);
	}
	
	/* Otherwise, our file is not encrypted so we can just read the contents*/
	else {
		inFile = fopen(realPath, "r");
		fileDescriptor = open(realPath, O_RDONLY);
		if (fileDescriptor == -1)
			return -errno;
			
		do_crypt(inFile, inFile, -1, FOPS_DATA->keyPhrase);
		
		res = pread(fileDescriptor, buf, size, offset);
		if (res == -1)
			res = -errno;
		
		close(fileDescriptor);
	}
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	
	/* Local Vars */
	int fileDescriptor;
	FILE* inFile;
	FILE* outFile;
	int tempFileDescriptor;
	int res;
	char realPath[MAX_PATH];
	
	/* Get the proper path */
	createPath(path, realPath);
	inFile = fopen(realPath, "r+");

	/* Check and see if our file is encrypted */
	int encryptChecker = isEncrypted(realPath);
	
	/* If our file is ENCRYPTED... */
	if (encryptChecker == 1){
		
		/* Set up real and temp files */
		inFile = fopen(realPath, "w");
		outFile = tmpfile();
		fileDescriptor = fileno(inFile);
		tempFileDescriptor = fileno(outFile);
		
		/* Write the contents to the temp file */
		(void) fi;
		res = pwrite(tempFileDescriptor, buf, size, offset);
		if (res == -1)
			res = -errno;
		
		fflush(outFile);
		
		/* Encrypt the data to the real file */
		do_crypt(outFile, inFile, 1, FOPS_DATA->keyPhrase);
		
		fflush(inFile);
		
		fclose(inFile);
		fclose(outFile);
	}
	
	/* Otherwise, our file is not encrypted...*/
	else{
		
		/* Simple open the real file and write to it */
		inFile = fopen(realPath, "w");
		fileDescriptor = open(realPath, O_WRONLY);
		(void) fi;
		
		do_crypt(inFile, inFile, -1, FOPS_DATA->keyPhrase);
	
		res = pwrite(fileDescriptor, buf, size, offset);
		close(fileDescriptor);
	}
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char buffer[MAX_PATH];
	createPath(path, buffer);

	res = statvfs(buffer, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
    
    /* Local Vars */
    FILE* inFile;
    
    /* Update Path */
    char realPath[MAX_PATH];
	createPath(path, realPath);

	/* Create the file */
    int res;
    res = creat(realPath, mode);
    if(res == -1){
		return -errno;
	}
	inFile = fdopen(res, "w");
	
	/* Encrypt the file */
	do_crypt(inFile, inFile, 1, FOPS_DATA->keyPhrase);
	
	/* Close the files */
    close(res);
    fclose(inFile);
    
    /* Set the extended attribute to show the file is encrypted */
    setxattr(realPath, XATTRNAME, XATTRTRUE, strlen(XATTRTRUE), 0);
	
    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char buffer[MAX_PATH];
	createPath(path, buffer);
	
	int res = lsetxattr(buffer, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char buffer[MAX_PATH];
	createPath(path, buffer);
	int res = lgetxattr(buffer, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char buffer[MAX_PATH];
	createPath(path, buffer);
	int res = llistxattr(buffer, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char buffer[MAX_PATH];
	createPath(path, buffer);
	int res = lremovexattr(buffer, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);
	
	char* keyPhrase;
	char* mirrorDir;
	char* mountPoint;
	
	/* Everything provided */
	if (argc == 4){
		keyPhrase = argv[1];
		mirrorDir = argv[2];
		mountPoint = argv[3];
	}
	
	/* No Key Phrase or Mirror Directory Provided */
	if (argc == 2){
		keyPhrase = NULL;
		mirrorDir = NULL;
		mountPoint = argv[argc - 1];
	}

	struct fs_state *fs_data;
	fs_data = malloc(sizeof(struct fs_state));
	
	fs_data->keyPhrase = argv[argc - 3];
	fs_data->mirrorDir = realpath(argv[argc-2], NULL);
	argv[argc-3] = argv[argc-1];
	argv[argc-2] = NULL;
	argv[argc-1] = NULL;
	argc = argc - 2;
	
	return fuse_main(argc, argv, &xmp_oper, fs_data);
}
