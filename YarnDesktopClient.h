#pragma once
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <libsecret/secret.h>
#include <gtk-4.0/gtk/gtk.h>

using namespace std;

std::string token = "";
GtkApplication *app;
GtkWidget *input_status;
GtkWidget *button_post_status;
GtkWidget *button_refresh_timeline;
GtkWidget *button_upload_media;
GtkWidget *window;
GtkWidget *button_login;
GtkWidget *input_username;
GtkWidget *input_password;
GtkWidget *input_server;
GtkWidget *checkbox_SSLVerify;
GtkWidget *checkbox_StoreUsernameServerUrl;
GtkWidget *timelineDropDown;
GtkWidget *timelineGrid = NULL;
GtkWidget *statusEntryGrid = NULL;
const char *currentTimelineName = NULL;
bool verifySSL = true;
const char *timelineNames[] = {"discover", "timeline", "mentions", NULL};
const char *userSettingsFile = "UserSettings.txt";
fstream settingsFile;

class UserInfo
{
public:
  string username = "";
  string serverUrl = "";
  string password = "";
};

const SecretSchema *get_schema(void) G_GNUC_CONST;
#define PASSWORD_SCHEMA get_schema()

int main(int argc, char **argv);
void replaceString(std::string &str, const std::string &from,
                   const std::string &to);
size_t writefunc(void *ptr, size_t size, size_t nmemb, std::string *s);
int str_ends_with(const char *str, const char *suffix);
void postStatus(std::string token, std::string status, std::string serverurl);
std::string whoAmI(std::string serverurl, std::string tokenTemp);
std::string getToken(std::string username, std::string password,
                     std::string serverurl);
std::string ReplaceAll(std::string str, const std::string &from,
                       const std::string &to);
bool FileExists(const std::string &Filename);
void downloadFile(std::string fileUrl, std::string filename);
void button_reply_clicked(__attribute__((unused)) GtkLabel *lbl,
                          std::string subject);
void parseJsonStatuses(std::string jsonstring);
std::string getTimeline(std::string serverurl);
void refreshTimeline();
void selectedTimeline();
void button_login_clicked(__attribute__((unused)) GtkLabel *lbl);
void button_refresh_timeline_clicked(__attribute__((unused)) GtkLabel *lbl);
void button_upload_media_clicked(__attribute__((unused)) GtkLabel *lbl);
static void activate(GtkApplication *app,
                     __attribute__((unused)) gpointer user_data);
std::string getSSLUrl(std::string url, bool verifySSL);
std::string getCleanLinkUrl(std::string link);
bool hasEnding(std::string const &fullString, std::string const &ending);
void readAndApplyUserSettings();
void storeUserSettings();
void storePassword();
