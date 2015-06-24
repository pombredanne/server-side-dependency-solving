#ifndef _CLIENT_H
#define _CLIENT_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//SSDS
#include "../common/log_handler.h"
#include "../common/network_util.h"
#include "../common/params.h"
#include "../common/json_handler.h"
#include "../common/repo_handler.h"

/** path to package download directory */
static const char *DOWNLOAD_TARGET = "/var/cache/ssds/packages/";

#ifdef __cplusplus
}
#endif

#endif