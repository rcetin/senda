#include <string.h>
#include <sys/queue.h>
#include <limits.h>

#include "config.h"
#include "main.h"
#include "debug/debug.h"
#include "json/json_parser.h"

struct node {
    TAILQ_ENTRY(node) nodes;
    config_t *cfg;
};

TAILQ_HEAD(nodeq, node);

struct {
    struct nodeq configq;
    char filename[PATH_MAX];
    configtype_e configtype;
    config_worker_t *worker;
} g_config;

int config_init(const char *filename, const char *configtype)
{
    debugf("[CONFIG] Enter");

    memset(&g_config, 0, sizeof(g_config));
    strcpy(g_config.filename, filename);
    if (!strcmp(configtype, "json")) {
        g_config.configtype = JSON;
        g_config.worker = &json_worker;

    } else {
        errorf("[CONFIG] wrong config type!");
        return -1;
    }
    TAILQ_INIT(&g_config.configq);

    infof("[CONFIG] New config is created.");
    debugf("[CONFIG] returning");
    return 0;
}

static void config_stream_destroy(config_t *cfg)
{
    if (!cfg) {
        errorf("[cfg] cfg NULL");
        return;
    }

    for (int i = 0; i < cfg->cfg_size; ++i) {
        errorf("[cfg] removing idx=%d", i);
        // SFREE(cfg->streams[i].stream_ctx);
        if (!cfg->streams[i].mapped) {
            SFREE(cfg->streams[i].data);
        }
    }
        errorf("[cfg] removing streams");
    SFREE(cfg->streams);
}

int config_get_stream(config_t *cfg_out, streamtype_e stream_type, int *stream_size)
{
    g_config.worker->get_stream(g_config.filename, stream_type, cfg_out, stream_size);

    struct node *cfg_node = calloc(1, sizeof(*cfg_node));
    if (!cfg_node) {
        errorf("[CONFIG] mem allocation failed");
        config_stream_destroy(cfg_out);
        cfg_out = NULL;
        return -1;
    }

    cfg_node->cfg = cfg_out;
    TAILQ_INSERT_TAIL(&g_config.configq, cfg_node, nodes);
    return 0;
}

void config_destroy(void)
{
    struct node *p, *tp;
    TAILQ_FOREACH_SAFE(p, &g_config.configq, nodes, tp) {
        TAILQ_REMOVE(&g_config.configq, p, nodes);
        config_stream_destroy(p->cfg);
        
        SFREE(p);
    }
}