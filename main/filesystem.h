/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

// When including this file also include the file system function header files
#include <stdio.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>


#define FILESYSTEM1 "spiffs1"
#define FILESYSTEM1_BASE "/www"
#define FILESYSTEM1_BASE_SIZE 4
// max file size is 15 (VFS) + 32 (for spiffs)
#define MAX_FILEPATH_LENGTH 47

void spiffs_start();

#endif
