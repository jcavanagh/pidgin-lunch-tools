#define PURPLE_PLUGINS

#define PLUGIN_ID "gtk-jcavanagh-pidgin_lunch_tools"

#include <glib.h>

#include "cmds.h"
#include "conversation.h"
#include "debug.h"
#include "plugin.h"
#include "version.h"

//Command IDs
static PurpleCmdId lunch_coup_command_id;

static PurpleCmdRet lunch_coup_cb(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data) {
    purple_debug_misc(PLUGIN_ID, "lunch_coup_cb called\n");
    return PURPLE_CMD_RET_OK;
}

static void init_lunch_coup(PurplePlugin *plugin) {
    // /lunchcoup
    lunch_coup_command_id = purple_cmd_register( 
        "lunchcoup",
        "s",
        PURPLE_CMD_P_DEFAULT,
        PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
        PLUGIN_ID,
        lunch_coup_cb,
        "Initiates and executes a lunch coup",
        NULL
    );
}

static gboolean plugin_load(PurplePlugin *plugin) {
    init_lunch_coup(plugin);

    return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
  purple_cmd_unregister(lunch_coup_command_id);

  return TRUE;
}

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,
    PLUGIN_ID,
    "Lunch Tools", 
    "1.0",
    "Tools for #lunchclub",
    "Need a lunch coup?  We get it done.",
    "Joe Cavanagh <joe@jlcavanagh.com>",
    "https://github.com/jcavanagh/pidgin-lunch-tools",
    plugin_load,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};                            
    
static void                        
init_plugin(PurplePlugin *plugin)
{
    //Nothing to do
}

PURPLE_INIT_PLUGIN("pidgin-lunch-tools", init_plugin, info)