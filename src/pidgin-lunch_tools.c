#define PURPLE_PLUGINS

#define PLUGIN_ID "gtk-jcavanagh-pidgin_lunch_tools"

//Configuration file stuff
#define PREF_PREFIX     "/plugins/gtk/" PLUGIN_ID

#define PREF_BOT                         PREF_PREFIX "/bot"
#define PREF_BOT_CMD_STR                 PREF_BOT "/cmd_prefix"

#define PREF_LUNCH_COUP                  PREF_PREFIX "/lunch_coup"
#define PREF_LUNCH_COUP_START_CMD        PREF_LUNCH_COUP "/start_cmd"
#define PREF_LUNCH_COUP_VOTES_LEFT_REGEX PREF_LUNCH_COUP "/votes_left_regex"
#define PREF_LUNCH_COUP_COMPLETE_REGEX   PREF_LUNCH_COUP "/complete_regex"

#define PREF_CHANNEL                     PREF_PREFIX "/channel"
#define PREF_CHANNEL_NICK_CHANGE_DELAY   PREF_CHANNEL "/nick_change_delay"
#define PREF_CHANNEL_MSG_DELAY           PREF_CHANNEL "/msg_delay"

#include <glib.h>

#include "cmds.h"
#include "conversation.h"
#include "debug.h"
#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"
#include "version.h"

//Command IDs
static PurpleCmdId lunch_coup_command_id;

//Lunch coup things
static PurpleCmdRet lunch_coup_cb(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data) {
    purple_debug_misc(PLUGIN_ID, "lunch_coup_cb called\n");
    return PURPLE_CMD_RET_OK;
}

static void init_lunch_coup(PurplePlugin *plugin) {
    // /lunchcoup
    lunch_coup_command_id = purple_cmd_register( 
        "lunchcoup",
        "",
        PURPLE_CMD_P_DEFAULT,
        PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
        PLUGIN_ID,
        lunch_coup_cb,
        "Initiates and executes a lunch coup",
        NULL
    );
}

//Configuration things
static PurplePluginPrefFrame *get_plugin_pref_frame(PurplePlugin *plugin) {
    PurplePluginPrefFrame *frame;
    PurplePluginPref *pref;

    frame = purple_plugin_pref_frame_new();

    //Bot config
    pref = purple_plugin_pref_new_with_label("Bot");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_BOT_CMD_STR,
        "Command string"
    );
    purple_plugin_pref_frame_add(frame, pref);

    //Lunch coup config
    pref = purple_plugin_pref_new_with_label("Lunch Coup");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_START_CMD,
        "Start command"
    );
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_VOTES_LEFT_REGEX,
        "Votes left regex"
    );
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_COMPLETE_REGEX,
        "Coup complete regex"
    );
    purple_plugin_pref_frame_add(frame, pref);

    //Channel
    pref = purple_plugin_pref_new_with_label("Channel");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_CHANNEL_NICK_CHANGE_DELAY,
        "Nick change delay (ms)"
    );
    purple_plugin_pref_set_bounds(pref, 100, 5000);
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_CHANNEL_MSG_DELAY,
        "Message send delay (ms)"
    );
    purple_plugin_pref_set_bounds(pref, 100, 5000);
    purple_plugin_pref_frame_add(frame, pref);

    return frame;
}

static void create_prefs() {
    //Prefixes and such
    purple_prefs_add_none(PREF_PREFIX);

    purple_prefs_add_none(PREF_BOT);
    purple_prefs_add_none(PREF_LUNCH_COUP);
    purple_prefs_add_none(PREF_CHANNEL);

    //Bot prefs
    purple_prefs_add_string(PREF_BOT_CMD_STR, "!dbldown");

    //Lunch coup prefs
    purple_prefs_add_string(PREF_LUNCH_COUP_START_CMD, "lunch coup");
    purple_prefs_add_string(PREF_LUNCH_COUP_VOTES_LEFT_REGEX, "^(\\d+).+votes.+$");
    purple_prefs_add_string(PREF_LUNCH_COUP_COMPLETE_REGEX, "^Down with.+$");

    //Channel prefs
    purple_prefs_add_int(PREF_CHANNEL_NICK_CHANGE_DELAY, 500);
    purple_prefs_add_int(PREF_CHANNEL_MSG_DELAY, 500);
}

//Plugin things
static gboolean plugin_load(PurplePlugin *plugin) {
    //Load/create prefs
    create_prefs();

    //Load the rest of the plugin
    init_lunch_coup(plugin);

    return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
  purple_cmd_unregister(lunch_coup_command_id);

  return TRUE;
}

static PurplePluginUiInfo prefs_info = {
    get_plugin_pref_frame,
    0,

    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
};

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
    plugin_unload,
    NULL,

    NULL,
    NULL,
    &prefs_info,
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

PURPLE_INIT_PLUGIN(pidgin_lunch_tools, init_plugin, info)