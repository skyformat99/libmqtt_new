#include "ae_io.h"

static void
__connack(struct libmqtt *mqtt, void *ud, int ack_flags, enum mqtt_connack return_code) {
    (void)ud;
    (void)ack_flags;

    if (return_code != CONNACK_ACCEPTED) {
        fprintf(stderr, "%s\n", MQTT_CONNACK_NAMES[return_code]);
        return;
    }
    libmqtt__disconnect(mqtt);
}

static void
__disconnect(aeEventLoop *el, struct ae_io *io) {
    aeStop(el);
}

static void
__log(void *ud, const char *str) {
    fprintf(stdout, "%s\n", str);
}

int main(int argc, char *argv[]) {
	int rc;
	struct libmqtt *mqtt;
    struct libmqtt_cb cb = {
        .connack = __connack,
    };
    aeEventLoop *el;
    struct ae_io *io;

    el = aeCreateEventLoop(128);

    rc = libmqtt__create(&mqtt, "01-con-discon-success", 0, &cb);
    if (!rc) libmqtt__debug(mqtt, __log);

    io = ae_io__connect(el, mqtt, "218.244.150.89", 1883, __disconnect);
    if (!io) {
        return 0;
    }

    if (!rc) rc = libmqtt__connect(mqtt, io, ae_io__write);
    if (rc != LIBMQTT_SUCCESS) {
        fprintf(stderr, "%s\n", libmqtt__strerror(rc));
        return 0;
    }

    aeMain(el);

    libmqtt__destroy(mqtt);

    ae_io__close(el, io);
    aeDeleteEventLoop(el);

	return 0;
}
