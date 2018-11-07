
extern int callback_bdm(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

struct pss__bdm {
    int number;
};

#define LWS_PLUGIN_PROTOCOL_BDM \
	{ \
		"bdm-protocol", \
		callback_bdm, \
		sizeof(struct pss__bdm), \
		10, /* rx buf size must be >= permessage-deflate rx size */ \
		0, NULL, 0 \
	}
