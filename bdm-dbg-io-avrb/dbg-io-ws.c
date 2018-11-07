!!!!!!!!!!! move to the dedicated project "bdm-dbg-ws" !!!
send incoming WS messages to the WS specific pipe


#include <libwebsockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "dbg-io-ws-plugin-bdm.h"

#ifdef EXTERNAL_POLL
struct lws_pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
#endif

int debug_level = 7;
volatile int force_exit = 0;
// volatile int dynamic_vhost_enable = 0;
//struct lws_vhost *dynamic_vhost;
struct lws_context *context;
//struct lws_plat_file_ops fops_plat;
static int test_options; // TODO: bdm has nooptions - get rid of it


static int
lws_callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
		  void *in, size_t len)
{
	const unsigned char *c;
	char buf[1024];
	int n = 0, hlen;

	switch (reason) {
	case LWS_CALLBACK_HTTP:

		/* non-mount-handled accesses will turn up here */

		/* dump the headers */
		do {
			c = lws_token_to_string(n);
			if (!c) {
				n++;
				continue;
			}

			hlen = lws_hdr_total_length(wsi, n);
			if (!hlen || hlen > (int)sizeof(buf) - 1) {
				n++;
				continue;
			}

			if (lws_hdr_copy(wsi, buf, sizeof buf, n) < 0)
				fprintf(stderr, "    %s (too big)\n", (char *)c);
			else {
				buf[sizeof(buf) - 1] = '\0';

				fprintf(stderr, "    %s = %s\n", (char *)c, buf);
			}
			n++;
		} while (c);

		/* dump the individual URI Arg parameters */

		n = 0;
		while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf),
					     WSI_TOKEN_HTTP_URI_ARGS, n) > 0) {
			lwsl_notice("URI Arg %d: %s\n", ++n, buf);
		}

		if (lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL))
			return -1;

		if (lws_http_transaction_completed(wsi))
			return -1;

		return 0;
	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

void sighandler(int sig)
{
	force_exit = 1;
	lws_cancel_service(context);
}

static const struct lws_protocol_vhost_options pvo_options = {
	NULL,
	NULL,
	"options",		/* pvo name */
	(void *)&test_options	/* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
	NULL,				/* "next" pvo linked-list */
	&pvo_options,		/* "child" pvo linked-list */
	"dumb-increment-protocol",	/* protocol name we belong to on this vhost */
	""				/* ignored */
};

int io_ws_init() 
{

    struct lws_context_creation_info info;
    struct lws_vhost *vhost;

    static struct lws_protocols protocols[] = {
	    /* first protocol must always be HTTP handler */
	    { "http-only", lws_callback_http, 0, 0, },
        LWS_PLUGIN_PROTOCOL_BDM,
	    { NULL, NULL, 0, 0 } /* terminator */
    };

    /*
     * mount a filesystem directory into the URL space at /
     * point it to our /usr/share directory with our assets in
     * stuff from here is autoserved by the library
     */

    #define LOCAL_RESOURCE_PATH LWS_INSTALL_DATADIR"/libwebsockets-test-server"
    char *resource_path = LOCAL_RESOURCE_PATH;

    /* AV: TODO: FileSystem not in use - get rid of it */
    static const struct lws_http_mount mount = {
	    NULL,	/* linked-list pointer to next*/
	    "/",		/* mountpoint in URL namespace on this vhost */
	    LOCAL_RESOURCE_PATH, /* where to go on the filesystem for that */
	    "test.html",	/* default filename if none given */
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    LWSMPRO_FILE,	/* mount type is a directory in a filesystem */
	    1,		/* strlen("/"), ie length of the mountpoint */
	    NULL,

	    { NULL, NULL } // sentinel
    };

    signal(SIGINT, sighandler);

	/* tell the library what debug level to emit and to send it to stderr */
	lws_set_log_level(debug_level, NULL);

	lwsl_notice("libwebsockets test server - license LGPL2.1+SLE\n");
	lwsl_notice("(C) Copyright 2010-2018 Andy Green <andy@warmcat.com>\n");

	info.iface = NULL;                      // Listen on all interfaces
	info.protocols = protocols;             // HTTP + BDM
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.ws_ping_pong_interval = 0;         // No ping-pong

    info.gid = -1;
	info.uid = -1;
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8 | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
	info.extensions = NULL;
	info.timeout_secs = 1;
	info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-GCM-SHA384:"
			       "DHE-RSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-SHA384:"
			       "HIGH:!aNULL:!eNULL:!EXPORT:"
			       "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
			       "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
			       "!DHE-RSA-AES128-SHA256:"
			       "!AES128-GCM-SHA256:"
			       "!AES128-SHA256:"
			       "!DHE-RSA-AES256-SHA256:"
			       "!AES256-GCM-SHA384:"
			       "!AES256-SHA256";
	info.mounts = &mount;
	info.ip_limit_ah = 24; /* for testing */
	info.ip_limit_wsi = 105; /* for testing */

	context = lws_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

	info.pvo = &pvo;      // AV: TODO: can it be NULL?

	vhost = lws_create_vhost(context, &info);
	if (!vhost) {
		lwsl_err("vhost creation failed\n");
		return -1;
	}

	return 0;
}

void dbg_io_ws_close() {
    lws_context_destroy(context);
	lwsl_notice("libwebsockets-test-server exited cleanly\n");
}

int io_ws_get_handles_num() 
{
    return (context == NULL) ? 0 : lws_context_get_max_fds(context);
}
