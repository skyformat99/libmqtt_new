/*
 * libmqtt.h -- mqtt client library.
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

#ifndef _LIBMQTT_H
#define _LIBMQTT_H

#ifdef __cplusplus
extern "C" {
#endif

/* generic includes. */
#include <stdint.h>
#include <sys/types.h>

#include "mqtt.h"

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define LIBMQTT_API __attribute__((visibility("default")))
#else
# define LIBMQTT_API
#endif


#define LIBMQTT_SUCCESS             0       /* success. */

/* define errors. */
#define LIBMQTT_ERROR_NULL          -1      /* mqtt null pointer access. */
#define LIBMQTT_ERROR_MALLOC        -2		/* mqtt emory allocation error. */
#define LIBMQTT_ERROR_QOS           -3      /* mqtt qos error. */
#define LIBMQTT_ERROR_VSN           -4      /* mqtt protocol version error. */
#define LIBMQTT_ERROR_WRITE         -5      /* mqtt io write error. */
#define LIBMQTT_ERROR_PARSE			-6		/* mqtt packet parse error. */
#define LIBMQTT_ERROR_TIMEOUT		-7		/* mqtt timeout error. */
#define LIBMQTT_ERROR_MAXSUB        -8      /* mqtt max topic/qos per subscribe or unsubscribe. */

/* default mqtt keep alive. */
#define LIBMQTT_DEF_KEEPALIVE       30

/* default mqtt packet retry time. */
#define LIBMQTT_DEF_TIMERETRY       3

/* libmqtt data structure. */
struct libmqtt;

/* libmqtt io write. */
typedef int (* libmqtt__io_write)(void *io, const char *data, int size);

/* libmqtt callbacks. */
typedef void (* libmqtt__on_connack)(struct libmqtt *, void *ud, int ack_flags, enum mqtt_connack return_code);
typedef void (* libmqtt__on_suback)(struct libmqtt *, void *ud, uint16_t id, int count, enum mqtt_qos *qos);
typedef void (* libmqtt__on_unsuback)(struct libmqtt *, void *ud, uint16_t id);
typedef void (* libmqtt__on_puback)(struct libmqtt *, void *ud, uint16_t id);
typedef void (* libmqtt__on_publish)(struct libmqtt *, void *ud, uint16_t id, const char *topic, enum mqtt_qos qos, int retain, const char *payload, int length);

/* libmqtt callback structure. */
struct libmqtt_cb {
    libmqtt__on_connack connack;
    libmqtt__on_suback suback;
    libmqtt__on_unsuback unsuback;
    libmqtt__on_puback puback;
    libmqtt__on_publish publish;
};

/* string error message for a libmqtt return code. */
extern LIBMQTT_API const char *libmqtt__strerror(int rc);

/* set a log callback for debug libmqtt. */
extern LIBMQTT_API void libmqtt__debug(struct libmqtt *mqtt, void (* log)(void *ud, const char *str));

/* generic libmqtt functions. */
extern LIBMQTT_API int libmqtt__create(struct libmqtt **mqtt, const char *client_id, void *ud, struct libmqtt_cb *cb);
extern LIBMQTT_API int libmqtt__destroy(struct libmqtt *mqtt);

extern LIBMQTT_API int libmqtt__time_retry(struct libmqtt *mqtt, int time_retry);
extern LIBMQTT_API int libmqtt__keep_alive(struct libmqtt *mqtt, uint16_t keep_alive);
extern LIBMQTT_API int libmqtt__clean_sess(struct libmqtt *mqtt, int clean_sess);
extern LIBMQTT_API int libmqtt__version(struct libmqtt *mqtt, enum mqtt_vsn vsn);
extern LIBMQTT_API int libmqtt__auth(struct libmqtt *mqtt, const char *username, const char *password);
extern LIBMQTT_API int libmqtt__will(struct libmqtt *mqtt, int retain, enum mqtt_qos qos, const char *topic, const char *payload, int payload_len);

extern LIBMQTT_API int libmqtt__connect(struct libmqtt *mqtt, void *io, libmqtt__io_write write);
extern LIBMQTT_API int libmqtt__disconnect(struct libmqtt *mqtt);

extern LIBMQTT_API int libmqtt__subscribe(struct libmqtt *mqtt, uint16_t *id, int count, const char *topic[], enum mqtt_qos qos[]);
extern LIBMQTT_API int libmqtt__unsubscribe(struct libmqtt *mqtt, uint16_t *id, int count, const char *topic[]);
extern LIBMQTT_API int libmqtt__publish(struct libmqtt *mqtt, uint16_t *id, const char *topic, enum mqtt_qos qos, int retain, const char *payload, int length);

extern LIBMQTT_API int libmqtt__read(struct libmqtt *mqtt, const char *data, int size);
extern LIBMQTT_API int libmqtt__update(struct libmqtt *mqtt);

#ifdef __cplusplus
}
#endif

#endif /* _LIBMQTT_H */
