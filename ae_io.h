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

#include "libmqtt.h"

#include "lib/ae.h"
#include "lib/anet.h"

#include <unistd.h>
#include <errno.h>

struct ae_io {
    int fd;
    long long timer_id;
    struct libmqtt *mqtt;
    void (* on_close)(aeEventLoop *el, struct ae_io *);
};


static void
ae_io__close(aeEventLoop *el, struct ae_io *io) {
    if (io->fd) {
        aeDeleteFileEvent(el, io->fd, AE_READABLE);
        close(io->fd);
    }
    if (io->timer_id)
        aeDeleteTimeEvent(el, io->timer_id);
    free(io);
}

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
        if (io->on_close)
            io->on_close(el, io);
        ae_io__close(el, io);
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


static struct ae_io *
ae_io__connect(aeEventLoop *el, struct libmqtt *mqtt, char *host, int port, void (* on_close)(aeEventLoop *el, struct ae_io *)) {
    struct ae_io *io;
    int fd;
    long long timer_id;
    char err[ANET_ERR_LEN];

    fd = anetTcpConnect(err, host, port);
    if (ANET_ERR == fd) {
        fprintf(stderr, "anetTcpConnect: %s\n", err);
        goto e1;
    }
    anetNonBlock(0, fd);
    anetEnableTcpNoDelay(0, fd);
    anetTcpKeepAlive(0, fd);

    io = (struct ae_io *)malloc(sizeof *io);
    memset(io, 0, sizeof *io);

    if (AE_ERR == aeCreateFileEvent(el, fd, AE_READABLE, __read, io)) {
        fprintf(stderr, "aeCreateFileEvent: error\n");
        goto e2;
    }

    timer_id = aeCreateTimeEvent(el, 1000, __update, io, 0);
    if (AE_ERR == timer_id) {
        fprintf(stderr, "aeCreateTimeEvent: error\n");
        goto e3;
    }
    
    io->fd = fd;
    io->timer_id = timer_id;
    io->mqtt = mqtt;
    io->on_close = on_close;
    return io;

e3:
    aeDeleteFileEvent(el, fd, AE_READABLE);
e2:
    close(fd);
    free(io);
e1:
    return 0;
}

static int
ae_io__write(void *p, const char *data, int size) {
    struct ae_io *io;

    io = (struct ae_io *)p;

    return write(io->fd, data, size);
}

#endif // _AE_IO_H_