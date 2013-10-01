#define PURPLE_PLUGINS

#define PLUGIN_ID "gtk-jcavanagh-pidgin_lunch_tools"

//Configuration file stuff
#define PREF_PREFIX     "/plugins/gtk/" PLUGIN_ID

#define PREF_BOT                         PREF_PREFIX "/bot"
#define PREF_BOT_NAME                    PREF_BOT "/name"
#define PREF_BOT_CMD_PREFIX              PREF_BOT "/cmd_prefix"

#define PREF_LUNCH_COUP                  PREF_PREFIX "/lunch_coup"
#define PREF_LUNCH_COUP_START_CMD        PREF_LUNCH_COUP "/start_cmd"
#define PREF_LUNCH_COUP_VOTES_LEFT_REGEX PREF_LUNCH_COUP "/votes_left_regex"
#define PREF_LUNCH_COUP_COMPLETE_REGEX   PREF_LUNCH_COUP "/complete_regex"

#define PREF_CHANNEL                     PREF_PREFIX "/channel"
#define PREF_CHANNEL_NICK_CMD            PREF_CHANNEL "/nick_cmd"

#include <glib.h>
#include <string.h>

#include "cmds.h"
#include "conversation.h"
#include "debug.h"
#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"
#include "version.h"

//Command IDs
static PurpleCmdId lunch_coup_command_id;

//Utils
void gen_random_str(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

//Lunch coup things
static char *lunch_coup_original_nick = NULL;
static gboolean lunch_coup_active = FALSE;

static void change_nick(PurpleConvChat *conv_chat, char *new_nick) {
    const char *nick_cmd = purple_prefs_get_string(PREF_CHANNEL_NICK_CMD);
    char *final_nick_cmd = (char*) malloc(32 * sizeof(char));

    strcat(final_nick_cmd, nick_cmd);
    strcat(final_nick_cmd, " ");
    strcat(final_nick_cmd, new_nick);

    purple_conv_chat_send(conv_chat, nick_cmd);

    free(final_nick_cmd);
}

static gboolean lunch_coup_receiving_msg_cb(PurpleAccount *account, char **sender, char **message, PurpleConversation *conv, PurpleMessageFlags *flags) {
    //Pull all the prefs we need
    const char *bot_name = purple_prefs_get_string(PREF_BOT_NAME);
    const char *votes_left_regex = purple_prefs_get_string(PREF_LUNCH_COUP_VOTES_LEFT_REGEX);
    const char *coup_complete_regex = purple_prefs_get_string(PREF_LUNCH_COUP_COMPLETE_REGEX);

    //Get conversation type and chat data
    //TODO: Support for IM?  Probably don't need it.
    PurpleConversationType conv_type = purple_conversation_get_type(conv);
    PurpleConvChat *conv_chat = purple_conversation_get_chat_data(conv);
    
    if(conv_type == PURPLE_CONV_TYPE_CHAT) {
        if(lunch_coup_active) {
            if(*sender == bot_name) {
                //Check to see what it sent
                if(g_regex_match_simple(coup_complete_regex, *message, G_REGEX_CASELESS, 0)) {
                    //Coup complete!  Return to original nick and end coup
                    change_nick(conv_chat, lunch_coup_original_nick);
                    lunch_coup_active = FALSE;
                } else if(g_regex_match_simple(votes_left_regex, *message, G_REGEX_CASELESS, 0)) {
                    //More votes are required, change nick and go to town
                    char *new_nick = (char*) malloc(16 * sizeof(char));
                    gen_random_str(new_nick, 8);
        
                    change_nick(conv_chat, new_nick);

                    free(new_nick);
                } else {
                    purple_debug_misc(PLUGIN_ID, "Ignoring bot message - no match: ");
                    purple_debug_misc(PLUGIN_ID, *sender);
                    purple_debug_misc(PLUGIN_ID, "\n");
                }
            } else {
                purple_debug_misc(PLUGIN_ID, "Ignoring message - not from configured bot\n");
            }
        } else {
            purple_debug_misc(PLUGIN_ID, "Ignoring message - lunch coup not active\n");
        }
    } else {
        purple_debug_misc(PLUGIN_ID, "Ignoring message - not in chat\n");
    }

    return TRUE;
}

static PurpleCmdRet lunch_coup_cb(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data) {
    //Pull all the prefs we need
    const char *start_cmd = purple_prefs_get_string(PREF_LUNCH_COUP_START_CMD);

    //Get conversation type and chat data
    //TODO: Support for IM?  Probably don't need it.
    PurpleConversationType conv_type = purple_conversation_get_type(conv);
    PurpleConvChat *conv_chat = purple_conversation_get_chat_data(conv);
    
    //DOWN WITH THE KING
    if(conv_type == PURPLE_CONV_TYPE_CHAT) {
        //Start the coup
        lunch_coup_original_nick = (char *)purple_conv_chat_get_nick(conv_chat);
        lunch_coup_active = TRUE;
        purple_conv_chat_send(conv_chat, start_cmd);
    } else {
        purple_conversation_write(
            conv,
            PLUGIN_ID,
            "Cannot start lunch coup - not in a chat.",
            PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG,
            time(NULL)
        );
    }

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
        PREF_BOT_CMD_PREFIX,
        "Bot Command Prefix"
    );
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_BOT_NAME,
        "Bot Name"
    );
    purple_plugin_pref_frame_add(frame, pref);

    //Lunch coup config
    pref = purple_plugin_pref_new_with_label("Lunch Coup");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_START_CMD,
        "Start Command"
    );
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_VOTES_LEFT_REGEX,
        "Votes Left Regex"
    );
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_COMPLETE_REGEX,
        "Coup Complete Regex"
    );
    purple_plugin_pref_frame_add(frame, pref);

    //Channel
    pref = purple_plugin_pref_new_with_label("Channel");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_CHANNEL_NICK_CMD,
        "Nick Change Command"
    );
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
    purple_prefs_add_string(PREF_BOT_NAME, "dbldown");
    purple_prefs_add_string(PREF_BOT_CMD_PREFIX, "!");

    //Lunch coup prefs
    purple_prefs_add_string(PREF_LUNCH_COUP_START_CMD, "lunch coup");
    purple_prefs_add_string(PREF_LUNCH_COUP_VOTES_LEFT_REGEX, "^(\\d+).+votes.+$");
    purple_prefs_add_string(PREF_LUNCH_COUP_COMPLETE_REGEX, "^Down with.+$");

    //Channel prefs
    purple_prefs_add_string(PREF_CHANNEL_NICK_CMD, "/nick");
}

//Chat handlers
static gboolean receiving_msg_cb(PurpleAccount *account, char **sender, char **message, PurpleConversation *conv, PurpleMessageFlags *flags) {
    //Delegate to lunch coup handler
    lunch_coup_receiving_msg_cb(account, sender, message, conv, flags);

    return TRUE;
}

//Plugin things
static gboolean plugin_load(PurplePlugin *plugin) {
    //Load/create prefs
    create_prefs();

    //Load the rest of the plugin
    init_lunch_coup(plugin);

    //Attach to chat events
    purple_signal_connect(
        purple_conversations_get_handle(), 
        "receiving-chat-msg",
        plugin,
        PURPLE_CALLBACK(receiving_msg_cb),
        NULL
    );

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