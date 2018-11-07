#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#include "dbg-io-ws-plugin-bdm.h"

/**
 *  Protocol user data
 */
struct pss__bdm {
    int number;
};

struct vhd__bdm {
	const unsigned int *options;
};

int
callback_bdm(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct pss__bdm *pss = (struct pss__bdm *)user;
	struct vhd__bdm *vhd =(struct vhd__bdm *)
				lws_protocol_vh_priv_get(lws_get_vhost(wsi),
						lws_get_protocol(wsi));
	uint8_t buf[LWS_PRE + 20], *p = &buf[LWS_PRE];
	const struct lws_protocol_vhost_options *opt;
	int n, m;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
			lws_get_protocol(wsi),
			sizeof(struct vhd__bdm));
		if (!vhd)
			return -1;
		//if ((opt = lws_pvo_search(
		//		(const struct lws_protocol_vhost_options *)in,
		//		"options")))
		//	vhd->options = (unsigned int *)opt->value;
		break;

	case LWS_CALLBACK_ESTABLISHED:  
        // ^^^ for ssl connection only
		//if (!vhd->options || !((*vhd->options) & 1))
		//	lws_set_timer_usecs(wsi, DUMB_PERIOD_US);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		// Prepare answer in n, p 
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		// process commands in - "in", "len"

		if (/*bue-bue*/ 0) {
			lwsl_notice("dumb_inc: closing as requested\n");
			lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, NULL, 0);
			return -1;
		}
		break;

	case LWS_CALLBACK_TIMER:
		if (!vhd->options || !((*vhd->options) & 1)) {
			lws_callback_on_writable_all_protocol_vhost(
				lws_get_vhost(wsi), lws_get_protocol(wsi));
			lws_set_timer_usecs(wsi, -1);
		}
		break;

	default:
		break;
	}

	return 0;
}
