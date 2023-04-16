/*
 This file is part of *Yarn desktp client*.
 Copyright (C) 2023 Stig Atle Steffensen - https://stigatle.no.

 Yarn desktp client is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// Rapidjson includes.
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#ifdef _WIN32
#include <io.h>
#define access _access_s
#else
#include <unistd.h>
#endif

#include <gtk-4.0/gtk/gtk.h>

#include <curl/curl.h>
#include <regex>

using namespace std;
using namespace rapidjson;

std::string token = "";
GtkWidget *input_status;
GtkWidget *button_post_status;
GtkWidget *button_refresh_timeline;
GtkWidget *window;
GtkWidget *button_login;
GtkWidget *input_username;
GtkWidget *input_password;
GtkWidget *input_server;
GtkWidget *checkbox_SSLVerify;
GtkWidget *timelineDropDown;
GtkWidget *timelineGrid = NULL;
GtkWidget *statusEntryGrid = NULL;
const char *currentTimelineName = NULL;
bool verifySSL = true;
const char *timelineNames[] = {"discover", "timeline", "mentions", NULL};

class UserInfo {
public:
  string username = "";
  string serverUrl = "";
  string password = "";
};

UserInfo *userinfo = NULL;

class statusPost {
public:
  string status = "";
  string subject = "";
  string nick = "";
  string avatarUrl = "";
  string created = "";
  string hash = "";
  string nickUrl = "";
  vector<string> mentions;
  string authorUri = "";
};

size_t writefunc(void *ptr, size_t size, size_t nmemb, std::string *s) {
  s->append(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

void postStatus(std::string token, std::string status, std::string serverurl) {
  std::string prefix = "{\"text\": \"";
  std::string suffix = "\"}";

  std::string finalStatusJson = prefix + status + suffix;

  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();

  if (curl) {
    std::string s;

    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "application/x-www-form-urlencoded");
    std::string tokenheader = ("token:" + token);
    hs = curl_slist_append(hs, tokenheader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

    std::string finalServerUrl = serverurl + "/api/v1/post";

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);

    curl_easy_setopt(curl, CURLOPT_URL, finalServerUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, finalStatusJson.c_str());

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      std::cout << res << std::endl;
    } else {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
    }
    curl_easy_cleanup(curl);
  }
}

// Fetch the username for your profile.
std::string whoAmI(std::string serverurl, std::string tokenTemp) {
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();

  if (curl) {
    std::string jsonReplyString;
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    std::string tokenheader = ("token: " + tokenTemp);
    hs = curl_slist_append(hs, tokenheader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

    std::string finalServerUrl = serverurl + "/api/v1/whoami";

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);

    curl_easy_setopt(curl, CURLOPT_URL, finalServerUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonReplyString);

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      rapidjson::Document jsonReply;
      jsonReply.Parse(jsonReplyString.c_str());
      //std::cout << " json reply whoami: " << jsonReplyString << std::endl;
      if (jsonReply["username"] != NULL) {
        std::string username = jsonReply["username"].GetString();
        std::cout << username << std::endl;
        curl_easy_cleanup(curl);

        return username;
      }
      return "";
    } else {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      return "";
    }
    curl_easy_cleanup(curl);
    return "";
  }
  return "";
}

std::string getToken(std::string username, std::string password,
                     std::string serverurl) {
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();

  if (curl) {
    std::string jsonReplyString;

    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

    std::string finalServerUrl = serverurl + "/api/v1/auth";

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);

    curl_easy_setopt(curl, CURLOPT_URL, finalServerUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonReplyString);

    std::string finalRequest =
        "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, finalRequest.c_str());

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      rapidjson::Document jsonReply;
     // std::cout << " json reply: " << jsonReplyString << std::endl;

      if (jsonReplyString == "Invalid Credentials\n") {
        std::cout << "Invalid credentials!" << std::endl;
        return "";
      }
      if (jsonReplyString != "Invalid Credentials\n") {

        jsonReply.Parse(jsonReplyString.c_str());

        // Fetch token
        if (jsonReply["token"] != NULL) {
          std::cout << jsonReply["token"].GetString() << std::endl;
          curl_easy_cleanup(curl);
          std::string token_temp = jsonReply["token"].GetString();

          // Since we got the token - we also update the profile's username.
          userinfo->username = whoAmI(serverurl, token_temp).c_str();

          return token_temp;
        }
      }
      return "";
    } else {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      return "";
    }
    curl_easy_cleanup(curl);
    return "";
  }
  return "";
}

std::string ReplaceAll(std::string str, const std::string &from,
                       const std::string &to) {
  size_t start_pos = 0;

  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return str;
}

bool FileExists(const std::string &Filename) {
  return access(Filename.c_str(), 0) == 0;
}

// Download the avatars that does not exist already.
void downloadAvatar(std::string avatarUrl, std::string filename) {
  CURL *curl;
  CURLcode imgresult;
  const char *url = avatarUrl.c_str();
  curl = curl_easy_init();
  FILE *avatarFile;
  avatarFile = fopen(filename.c_str(), "wb");

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, avatarFile);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                     "(KHTML, like Gecko) Chrome/62.0.3202.94 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    // Grab image
    imgresult = curl_easy_perform(curl);

    if (imgresult) {
      cout << "Cannot grab the image!\n" << imgresult;
    } else {
      cout << "Successfully grabbed avatar! stored at: " << filename
           << imgresult;
    }

   // cout << "Fetch avatar result: \n" << imgresult;
  }
  curl_easy_cleanup(curl);
  fclose(avatarFile);
}

void button_reply_clicked(GtkLabel *lbl, std::string subject,
                          vector<string> mentions) {
  std::string replyString = subject;

  gtk_editable_set_text(GTK_EDITABLE(input_status), replyString.c_str());
}

void parseJsonStatuses(std::string jsonstring) {
  Document document;
  document.Parse(jsonstring.c_str());
  const Value &twtsArray = document["twts"];
  std::string finalTwtString = "";
  int guiLineOffset = 0;

  if (twtsArray.IsArray()) {
    for (SizeType index = 0; index < twtsArray.Size(); index++) {
      if (twtsArray[index]["markdownText"] != NULL) {
        statusPost *post = new statusPost();
        finalTwtString = "";

        post->nick = twtsArray[index]["twter"]["nick"].GetString();

        post->created = twtsArray[index]["created"].GetString();
        post->subject = twtsArray[index]["subject"].GetString();
        post->nickUrl = twtsArray[index]["twter"]["uri"].GetString();
        post->avatarUrl = twtsArray[index]["twter"]["avatar"].GetString();
        post->hash = twtsArray[index]["hash"].GetString();
        std::string finalReplyString = post->subject + " ";

        post->authorUri.append("@<");
        post->authorUri.append(post->nick.c_str());
        post->authorUri.append(" ");
        post->authorUri.append(post->nickUrl.c_str());
        post->authorUri.append("> ");

        // Do not include yourself in the reply if you hit reply on your own
        // post.
        if ((post->authorUri.find(userinfo->username) != string::npos) ==
            false) {
          finalReplyString.append(post->authorUri.c_str());
        }

        if (twtsArray[index]["mentions"] != NULL) {
          const Value &mentionsArray = twtsArray[index]["mentions"];
          if (mentionsArray != NULL) {
            // TODO: remove 'yourself' from the mentions, so that it only
            // appends others - not yourself.
            for (SizeType mentionIndex = 0; mentionIndex < mentionsArray.Size();
                 mentionIndex++) {
              // Check if the 'mentions' contains userinfo.username - skip if it
              // has the 'owners' name in it.
              std::string tmpString =
                  document["twts"][index]["mentions"][mentionIndex].GetString();

              if ((tmpString.find(userinfo->username) != string::npos) ==
                  false) {
                finalReplyString.append(tmpString);
                finalReplyString.append(" ");
              } else {
                // We do nothing if the mention is yourself.
              }

             // std::cout << std::endl
             //           << "replystring:" << finalReplyString << std::endl;
            }
          }
        }

        post->subject = finalReplyString;

        // We do not check file format for images, so we just store it with png
        // as extension - regardless of format right now.
        std::string filename = "./" + post->nick + ".png";

        if (post->avatarUrl != "") {
          if (!FileExists(filename)) {
            downloadAvatar(post->avatarUrl, filename);
          }
        }

        post->created = ReplaceAll(post->created, "T", ":");
        post->created = ReplaceAll(post->created, "Z", ".");

        finalTwtString = "\n" + post->created + "\n" + post->nick + ": \n\n" +
                         twtsArray[index]["markdownText"].GetString();

        while (finalTwtString.find("<") != std::string::npos) {
          auto startpos = finalTwtString.find("<");
          auto endpos = finalTwtString.find(">") + 1;

          if (endpos != std::string::npos) {
            finalTwtString.erase(startpos, endpos - startpos);
          }
        }

        while (finalTwtString.find("(http") != std::string::npos) {
          auto startpos = finalTwtString.find("(");
          auto endpos = finalTwtString.find(")") + 1;

          if (endpos != std::string::npos) {
            finalTwtString.erase(startpos, endpos - startpos);
          }
        }

        finalTwtString = ReplaceAll(finalTwtString, "\\u003c", "");
        finalTwtString = ReplaceAll(finalTwtString, "\\u003e", "");
        finalTwtString = ReplaceAll(finalTwtString, "&#39;", "'");
        finalTwtString = ReplaceAll(finalTwtString, "<p>", " ");
        finalTwtString = ReplaceAll(finalTwtString, "</p>", " ");
        finalTwtString = ReplaceAll(finalTwtString, "[", "");
        finalTwtString = ReplaceAll(finalTwtString, "]", "");
        finalTwtString = ReplaceAll(finalTwtString, post->subject, "");
        finalTwtString = ReplaceAll(finalTwtString, "Read more", "");

        post->status = finalTwtString.c_str();
       // std::cout << std::endl
       //           << "finalTwtString:" << finalTwtString << std::endl;
        GtkWidget *statusLabel = gtk_label_new(post->status.c_str());

        gtk_label_set_use_markup(GTK_LABEL(statusLabel), true);

        gtk_label_set_wrap(GTK_LABEL(statusLabel), true);
        gtk_label_set_xalign(GTK_LABEL(statusLabel), 0);
        gtk_label_set_max_width_chars(GTK_LABEL(statusLabel), 150);
        gtk_grid_attach(GTK_GRID(timelineGrid), statusLabel, 1,
                        index + guiLineOffset, 1, 1);

        GtkWidget *avatar = gtk_image_new_from_file(filename.c_str());
        gtk_image_set_pixel_size(GTK_IMAGE(avatar), 50);
        gtk_grid_attach(GTK_GRID(timelineGrid), avatar, 0,
                        index + guiLineOffset, 1, 1);

        // Create a reply button and send the 'subject' with it, so that the new
        // post replies to that status.
        GtkWidget *replyButton = gtk_button_new_with_label("Reply");

        g_signal_connect(replyButton, "clicked",
                         G_CALLBACK(button_reply_clicked), &post->subject);

        gtk_grid_attach(GTK_GRID(timelineGrid), replyButton, 0,
                        index + guiLineOffset + 1, 1, 1);

        GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_attach(GTK_GRID(timelineGrid), separator1, 0,
                        index + guiLineOffset + 2, 1, 1);
        gtk_grid_attach(GTK_GRID(timelineGrid), separator2, 1,
                        index + guiLineOffset + 2, 1, 1);

        // Bump the index based on extra widgets that has been added.
        guiLineOffset += 3;
      }
    }
  }
}

std::string getTimeline(std::string serverurl) {
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();

  if (currentTimelineName == NULL) {
    currentTimelineName = "discover";
  }

  if (curl) {
    std::string jsonReplyString;
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs,
                           "Content-Type: application/x-www-form-urlencoded");
    std::string tokenheader = ("token:" + token);
    hs = curl_slist_append(hs, tokenheader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{}");
    std::string finalServerUrl = serverurl + "/api/v1/" + currentTimelineName;

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);

    curl_easy_setopt(curl, CURLOPT_URL, finalServerUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonReplyString);

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      rapidjson::Document jsonReply;
      jsonReply.Parse(jsonReplyString.c_str());
      parseJsonStatuses(jsonReplyString);
      return "";
    } else {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      return "";
    }
    curl_easy_cleanup(curl);
    return "";
  }
  return "";
}

void refreshTimeline() {
  GtkWidget *iter = gtk_widget_get_first_child(timelineGrid);
  while (iter != NULL) {
    GtkWidget *next = gtk_widget_get_next_sibling(iter);
    gtk_grid_remove(GTK_GRID(timelineGrid), iter);
    iter = next;
  }
  getTimeline(userinfo->serverUrl);
}

void selectedTimeline() {
  auto selectedTimeLineIndex =
      gtk_drop_down_get_selected(GTK_DROP_DOWN(timelineDropDown));
  currentTimelineName = timelineNames[selectedTimeLineIndex];
  std::cout << std::endl
            << "selected timeline: " << currentTimelineName << std::endl;
  refreshTimeline();
}

void button_login_clicked(GtkLabel *lbl) {
  userinfo = new UserInfo();
  userinfo->username = gtk_editable_get_text(GTK_EDITABLE(input_username));
  userinfo->serverUrl = gtk_editable_get_text(GTK_EDITABLE(input_server));
  userinfo->password = gtk_editable_get_text(GTK_EDITABLE(input_password));
  token = getToken(userinfo->username, userinfo->password, userinfo->serverUrl);
  verifySSL = gtk_check_button_get_active(GTK_CHECK_BUTTON(checkbox_SSLVerify));

  if (token != "") {
    GtkWidget *statusEntrySeparator =
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(statusEntrySeparator, true);

    GtkWidget *statusEntrySeparator2 =
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(statusEntrySeparator2, true);

    GtkWidget *statusEntrySeparator3 =
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(statusEntrySeparator3, true);

    std::cout << std::endl << "Token: " << token << std::endl;
    getTimeline(userinfo->serverUrl);

    timelineDropDown = gtk_drop_down_new_from_strings(timelineNames);
    g_signal_connect(timelineDropDown, "notify::selected",
                     G_CALLBACK(selectedTimeline), NULL);

    gtk_grid_attach(GTK_GRID(statusEntryGrid), input_status, 1, 0, 3, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), button_post_status, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), button_refresh_timeline, 2, 1, 1,
                    1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), timelineDropDown, 3, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(statusEntryGrid), statusEntrySeparator, 1, 2, 1,
                    1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), statusEntrySeparator2, 2, 2, 1,
                    1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), statusEntrySeparator3, 3, 2, 1,
                    1);

    gtk_widget_hide(input_password);
    gtk_widget_hide(input_server);
    gtk_widget_hide(button_login);
    gtk_widget_hide(input_server);
    gtk_widget_hide(input_username);
    gtk_widget_hide(checkbox_SSLVerify);
  }
}
void button_refresh_timeline_clicked(GtkLabel *lbl) { refreshTimeline(); }

void button_post_status_clicked(GtkLabel *lbl) {
  std::string status = gtk_editable_get_text(GTK_EDITABLE(input_status));
  postStatus(token, status, userinfo->serverUrl);
  gtk_editable_set_text(GTK_EDITABLE(input_status), "");
  refreshTimeline();
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *gridParent;
  gridParent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *scrolled_window = gtk_scrolled_window_new();
  timelineGrid = gtk_grid_new();
  statusEntryGrid = gtk_grid_new();

  gtk_grid_set_column_spacing(GTK_GRID(statusEntryGrid), 1);
  gtk_grid_set_row_spacing(GTK_GRID(statusEntryGrid), 1);

  gtk_widget_set_hexpand(statusEntryGrid, true);
  gtk_widget_set_vexpand(statusEntryGrid, false);

  button_login = gtk_button_new_with_label("Log in");

  gtk_box_append(GTK_BOX(gridParent), statusEntryGrid);

  g_signal_connect(button_login, "clicked", G_CALLBACK(button_login_clicked),
                   NULL);

  window = gtk_application_window_new(app);

  gtk_window_set_title(GTK_WINDOW(window), "Yarn Desktop Client");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

  gtk_grid_set_column_spacing(GTK_GRID(timelineGrid), 1);
  gtk_grid_set_row_spacing(GTK_GRID(timelineGrid), 1);

  input_username = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_username), "Username");

  input_password = gtk_password_entry_new();
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(input_password),
                                        true);

  input_server = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_server), "Server URL");

  checkbox_SSLVerify = gtk_check_button_new_with_label("SSL verify.");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(checkbox_SSLVerify), true);

  input_status = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_status), "Enter status");

  button_post_status = gtk_button_new_with_label("Post status");

  button_refresh_timeline = gtk_button_new_with_label("Refresh timeline");

  g_signal_connect(button_post_status, "clicked",
                   G_CALLBACK(button_post_status_clicked), NULL);

  g_signal_connect(button_refresh_timeline, "clicked",
                   G_CALLBACK(button_refresh_timeline_clicked), NULL);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
                                timelineGrid);

  gtk_grid_set_column_homogeneous(GTK_GRID(statusEntryGrid), true);
  gtk_grid_set_column_homogeneous(GTK_GRID(timelineGrid), false);

  gtk_widget_set_hexpand(timelineGrid, false);
  gtk_widget_set_vexpand(timelineGrid, true);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  gtk_grid_attach(GTK_GRID(timelineGrid), input_username, 0, 0, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), input_password, 0, 1, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), input_server, 0, 2, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), button_login, 0, 3, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), checkbox_SSLVerify, 0,4,3,1);

  gtk_box_append(GTK_BOX(gridParent), scrolled_window);

  gtk_window_set_child(GTK_WINDOW(window), gridParent);

  gtk_widget_show(window);
}

int main(int argc, char **argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return (status);
}
