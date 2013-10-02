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
#define PREF_LUNCH_COUP_NO_KING_REGEX    PREF_LUNCH_COUP "/no_king_regex"

#define PREF_LUNCH_KING                  PREF_PREFIX "/lunch_king"
#define PREF_LUNCH_KING_CMD              PREF_LUNCH_KING "/cmd"

#define PREF_CHANNEL                     PREF_PREFIX "/channel"

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
static void gen_random_nick(char *dest, size_t length) {
    char first_charset[] = "abcdefghijklmnopqrstuvwxyz";
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    //Make sure the first is a letter
    size_t first_index = (double) rand() / RAND_MAX * (sizeof first_charset - 1);
    *dest++ = first_charset[first_index];
    length--;

    //Fill rest of string
    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

static void send_msg(PurpleConversation *conv, char *msg) {
    //TODO: IM support?
    PurpleConvChat *conv_chat = purple_conversation_get_chat_data(conv);

    purple_conv_chat_send_with_flags(conv_chat, msg, PURPLE_MESSAGE_SEND);
}

static void send_bot_cmd(PurpleConversation *conv, char *cmd) {
    //Get prefix and bot name
    const char *bot_name = purple_prefs_get_string(PREF_BOT_NAME);
    const char *bot_cmd_prefix = purple_prefs_get_string(PREF_BOT_CMD_PREFIX);

    //Build command
    char *bot_cmd = (char *) malloc(sizeof(bot_cmd_prefix) + sizeof(bot_name) + sizeof(cmd) + 1);
    strcpy(bot_cmd, bot_cmd_prefix);
    strcat(bot_cmd, bot_name);
    strcat(bot_cmd, " ");
    strcat(bot_cmd, cmd);

    send_msg(conv, bot_cmd);

    free(bot_cmd);
}

static void do_command(PurpleConversation *conv, char *cmd) {
    char *error = (char *) malloc(1024 * sizeof(char));

    purple_cmd_do_command(
        conv,
        cmd,
        cmd,
        &error
    );

    free(error);
}

static void change_nick(PurpleConversation *conv, char *new_nick) {
    char *final_nick_cmd = (char *) malloc(64 * sizeof(char));
    strcpy(final_nick_cmd, "nick ");
    strcat(final_nick_cmd, new_nick);

    purple_debug_misc(PLUGIN_ID, "Changing nick to: %s\n", new_nick);

    do_command(conv, final_nick_cmd);

    free(final_nick_cmd);
}

//Lunch coup things
static char lunch_coup_original_nick[64];
static gboolean lunch_coup_active = FALSE;

static void lunch_coup_send_start_cmd(PurpleConversation *conv) {
    //Pull prefs, create command
    const char *start_cmd = purple_prefs_get_string(PREF_LUNCH_COUP_START_CMD);

    send_bot_cmd(conv, (char *)start_cmd);
}

static void lunch_coup_activate(PurpleConversation *conv) {
    //TODO: IM support?
    PurpleConvChat *conv_chat = purple_conversation_get_chat_data(conv);

    //Activate lunch coup
    strcpy(lunch_coup_original_nick, purple_conv_chat_get_nick(conv_chat));
    lunch_coup_active = TRUE;

    //Send message
    lunch_coup_send_start_cmd(conv);

    purple_conversation_write(
        conv,
        PLUGIN_ID,
        "Lunch coup initiated!",
        PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG,
        time(NULL)
    );
}

static void lunch_coup_deactivate(PurpleConversation *conv) {
    //Deactivate lunch coup
    lunch_coup_active = FALSE;

    purple_conversation_write(
        conv,
        PLUGIN_ID,
        "Lunch coup complete!",
        PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG,
        time(NULL)
    );
}

static gboolean lunch_coup_receiving_msg_cb(PurpleAccount *account, char **sender, char **message, PurpleConversation *conv, PurpleMessageFlags *flags) {
    //Pull all the prefs we need
    const char *bot_name = purple_prefs_get_string(PREF_BOT_NAME);
    const char *votes_left_regex = purple_prefs_get_string(PREF_LUNCH_COUP_VOTES_LEFT_REGEX);
    const char *coup_complete_regex = purple_prefs_get_string(PREF_LUNCH_COUP_COMPLETE_REGEX);
    const char *no_king_regex = purple_prefs_get_string(PREF_LUNCH_COUP_NO_KING_REGEX);

    if(lunch_coup_active) {
        if(strcmp(*sender, bot_name) == 0) {
            //Check to see what it sent
            if(g_regex_match_simple(coup_complete_regex, *message, G_REGEX_CASELESS, 0)) {
                //Coup complete!  Return to original nick and end coup
                change_nick(conv, lunch_coup_original_nick);
                lunch_coup_deactivate(conv);
            } else if(g_regex_match_simple(no_king_regex, *message, G_REGEX_CASELESS, 0)) {
                //No king - no lunch coup necessary
                lunch_coup_deactivate(conv);
            } else if(g_regex_match_simple(votes_left_regex, *message, G_REGEX_CASELESS, 0)) {
                //More votes are required, change nick and go to town
                char new_nick[9];
                gen_random_nick(new_nick, 8);

                change_nick(conv, new_nick);
                lunch_coup_send_start_cmd(conv);
            } else {
                purple_debug_misc(PLUGIN_ID, "Ignoring bot message - no match: %s\n", *message);
            }
        } else {
            purple_debug_misc(PLUGIN_ID, "Ignoring message - not from configured bot: %s\n", *sender);
        }
    } else {
        purple_debug_misc(PLUGIN_ID, "Ignoring message - lunch coup not active\n");
    }

    return FALSE;
}

static PurpleCmdRet lunch_coup_cb(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data) {
    //Get conversation type and chat data
    //TODO: Support for IM?  Probably don't need it.
    PurpleConversationType conv_type = purple_conversation_get_type(conv);
    
    //DOWN WITH THE KING
    if(conv_type == PURPLE_CONV_TYPE_CHAT) {
        //Start the coup
        lunch_coup_activate(conv);
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

    // /lc
    lunch_coup_command_id = purple_cmd_register( 
        "lc",
        "",
        PURPLE_CMD_P_DEFAULT,
        PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
        PLUGIN_ID,
        lunch_coup_cb,
        "Initiates and executes a lunch coup",
        NULL
    );
}

//Lunch king things
static PurpleCmdRet lunch_king_cb(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data) {
    //Get prefs
    const char *king_cmd = purple_prefs_get_string(PREF_LUNCH_KING_CMD);

    //ALL HAIL THE KING
    send_bot_cmd(conv, (char *)king_cmd);

    return PURPLE_CMD_RET_OK;
}

static void init_lunch_king(PurplePlugin *plugin) {
    // /lunchking
    lunch_coup_command_id = purple_cmd_register(
        "lunchking",
        "",
        PURPLE_CMD_P_DEFAULT,
        PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
        PLUGIN_ID,
        lunch_king_cb,
        "",
        NULL
    );

    // /lk
    lunch_coup_command_id = purple_cmd_register(
        "lk",
        "",
        PURPLE_CMD_P_DEFAULT,
        PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
        PLUGIN_ID,
        lunch_king_cb,
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

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_COUP_NO_KING_REGEX,
        "No King Regex"
    );
    purple_plugin_pref_frame_add(frame, pref);

    //Lunch king config
    pref = purple_plugin_pref_new_with_label("Lunch King");
    purple_plugin_pref_frame_add(frame, pref);

    pref = purple_plugin_pref_new_with_name_and_label(
        PREF_LUNCH_KING_CMD,
        "Command"
    );
    purple_plugin_pref_frame_add(frame, pref);

    //Channel
    pref = purple_plugin_pref_new_with_label("Channel");
    purple_plugin_pref_frame_add(frame, pref);

    return frame;
}

static void create_prefs() {
    //Prefixes and such
    purple_prefs_add_none(PREF_PREFIX);

    purple_prefs_add_none(PREF_BOT);
    purple_prefs_add_none(PREF_LUNCH_COUP);
    purple_prefs_add_none(PREF_LUNCH_KING);
    purple_prefs_add_none(PREF_CHANNEL);

    //Bot prefs
    purple_prefs_add_string(PREF_BOT_NAME, "dbldown");
    purple_prefs_add_string(PREF_BOT_CMD_PREFIX, "!");

    //Lunch coup prefs
    purple_prefs_add_string(PREF_LUNCH_COUP_START_CMD, "lunch coup");
    purple_prefs_add_string(PREF_LUNCH_COUP_VOTES_LEFT_REGEX, "^.*(\\d+).*more.*vote.*$");
    purple_prefs_add_string(PREF_LUNCH_COUP_COMPLETE_REGEX, "^Down with.+$");
    purple_prefs_add_string(PREF_LUNCH_COUP_NO_KING_REGEX, "^.+no.+king.*$");

    //Lunch king prefs
    purple_prefs_add_string(PREF_LUNCH_KING_CMD, "lunch king");

    //Channel prefs
}

//Chat handlers
static gboolean receiving_msg_cb(PurpleAccount *account, char **sender, char **message, PurpleConversation *conv, PurpleMessageFlags *flags) {
    //Delegate to lunch coup handler
    lunch_coup_receiving_msg_cb(account, sender, message, conv, flags);

    return FALSE;
}

//Plugin things
static gboolean plugin_load(PurplePlugin *plugin) {
    //Load/create prefs
    create_prefs();

    //Load the rest of the plugin
    init_lunch_coup(plugin);
    init_lunch_king(plugin);

    //Attach to chat events
    //TODO: IM support?
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