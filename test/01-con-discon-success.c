#define AE_IO_IMPLEMENTATION
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
__disconnect(void *io) {
    io_stop(io);
}

static void
__log(void *ud, const char *str) {
    fprintf(stdout, "%s\n", str);
}

int main(int argc, char *argv[]) {
	int rc;
	struct libmqtt *mqtt;
    void *io;
    struct libmqtt_cb cb = {
        .connack = __connack,
    };

    rc = libmqtt__create(&mqtt, "01-con-discon-success", 0, &cb);
    if (!rc) libmqtt__debug(mqtt, __log);

    io = io_create(mqtt);
    rc = io_connect(io, "218.244.150.89", 1883, __disconnect);

    if (!rc) rc = libmqtt__connect(mqtt, io, io_write);
    if (rc != LIBMQTT_SUCCESS) {
        fprintf(stderr, "%s\n", libmqtt__strerror(rc));
        return 0;
    }

    io_loop(io);

    libmqtt__destroy(mqtt);
    io_close(io);
    io_destroy(io);
	return 0;
}
