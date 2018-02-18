/*
 * ae_io.h -- ae net library wrap for mqtt.
 *
 * Copyright (c) zhoukk <izhoukk@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _AE_IO_H_
#define _AE_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AE_IO_API
#define AE_IO_API extern
#endif // AE_IO_API

#include "libmqtt.h"

typedef void (* io_disconnect)(void *io);

AE_IO_API void *io_create(struct libmqtt *mqtt);

AE_IO_API void io_destroy(void *io);

AE_IO_API int io_connect(void *io, const char *host, int port, io_disconnect disconnect);

AE_IO_API int io_write(void *io, const char *data, int size);

AE_IO_API void io_close(void *io);

AE_IO_API void io_loop(void *io);

AE_IO_API void io_stop(void *io);

#ifdef __cplusplus
}
#endif

#endif // _AE_IO_H_


#ifdef AE_IO_IMPLEMENTATION


#include "lib/ae.h"
#include "lib/anet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

struct ae_io {
    aeEventLoop *el;
    struct libmqtt *mqtt;
    int fd;
    long long timer_id;
    io_disconnect disconnect;
};


static void
__read(aeEventLoop *el, int fd, void *privdata, int mask) {
    struct ae_io *io;
    int nread;
    char buff[4096];

    io = (struct ae_io *)privdata;
    nread = read(fd, buff, sizeof(buff));
    if (nread == -1 && errno == EAGAIN) {
        return;
    }
    if (nread <= 0 || LIBMQTT_SUCCESS != libmqtt__read(io->mqtt, buff, nread)) {
        io_close(io);
        if (io->disconnect) {
            io->disconnect(io);
        } else {
            io_stop(io);
        }
    }
}

static int
__update(aeEventLoop *el, long long id, void *privdata) {
    struct ae_io *io;

    io = (struct ae_io *)privdata;

    if (LIBMQTT_SUCCESS != libmqtt__update(io->mqtt)) {
        return 0;
    }
    return 1000;
}

AE_IO_API void *
io_create(struct libmqtt *mqtt) {
    struct ae_io *io;

    io = (struct ae_io *)malloc(sizeof *io);
    memset(io, 0, sizeof *io);
    io->el = aeCreateEventLoop(128);
    io->mqtt = mqtt;
    return (void *)io;
}

AE_IO_API void
io_destroy(void *io) {
    struct ae_io *aeio;

    aeio = (struct ae_io *)io;
    aeDeleteEventLoop(aeio->el);
    free(aeio);
}

AE_IO_API int
io_connect(void *io, const char *host, int port, io_disconnect disconnect) {
    struct ae_io *aeio;
    int fd;
    int timer_id;
    char err[ANET_ERR_LEN];

    aeio = (struct ae_io *)io;
    fd = anetTcpConnect(err, (char *)host, port);
    if (ANET_ERR == fd) {
        fprintf(stderr, "anetTcpConnect: %s\n", err);
        return -1;
    }
    anetNonBlock(0, fd);
    anetEnableTcpNoDelay(0, fd);
    anetTcpKeepAlive(0, fd);
    if (AE_ERR == aeCreateFileEvent(aeio->el, fd, AE_READABLE, __read, io)) {
        fprintf(stderr, "aeCreateFileEvent: error\n");
        return -1;
    }
    timer_id = aeCreateTimeEvent(aeio->el, 1000, __update, io, 0);
    if (AE_ERR == timer_id) {
        fprintf(stderr, "aeCreateTimeEvent: error\n");
        return -1;
    }

    aeio->fd = fd;
    aeio->timer_id = timer_id;
    aeio->disconnect = disconnect;
    return 0;
}

AE_IO_API int
io_write(void *io, const char *data, int size) {
    struct ae_io *aeio;

    aeio = (struct ae_io *)io;
    return write(aeio->fd, data, size);
}

AE_IO_API void
io_close(void *io) {
    struct ae_io *aeio;

    aeio = (struct ae_io *)io;
    aeDeleteFileEvent(aeio->el, aeio->fd, AE_READABLE);
    aeDeleteTimeEvent(aeio->el, aeio->timer_id);
    close(aeio->fd);
    aeio->fd = 0;
}

AE_IO_API void
io_loop(void *io) {
    struct ae_io *aeio;

    aeio = (struct ae_io *)io;
    aeMain(aeio->el);
}

AE_IO_API void
io_stop(void *io) {
    struct ae_io *aeio;

    aeio = (struct ae_io *)io;
    aeStop(aeio->el);
}

#endif // AE_IO_IMPLEMENTATION