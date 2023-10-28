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

#include "YarnDesktopClient.h"
#include "statuspost.h"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <experimental/filesystem>

#include <libsecret/secret.h>

// Rapidjson includes.
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define access _access_s
#else
#include <unistd.h>
#endif

#include <curl/curl.h>
#include <regex>

using namespace std;
using namespace rapidjson;

UserInfo *userinfo = NULL;

void replaceString(std::string &str, const std::string &from,
                   const std::string &to)
{
  size_t start_pos = str.find(from);
  str.replace(start_pos, from.length(), to);
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, std::string *s)
{
  s->append(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

/*  returns 1 iff str ends with suffix  */
int str_ends_with(const char *str, const char *suffix)
{
  if (str == NULL || suffix == NULL)
    return 0;

  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if (suffix_len > str_len)
    return 0;

  return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}

void postStatus(std::string token, std::string status, std::string serverurl)
{
  std::string prefix = "{\"text\": \"";
  std::string suffix = "\"}";

  std::string finalStatusJson = prefix + status + suffix;

  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();

  if (curl)
  {
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

    if (res == CURLE_OK)
    {
      std::cout << res << std::endl;
    }
    else
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    curl_slist_free_all(hs);
    curl_easy_cleanup(curl);
  }
}

// Fetch the username for your profile.
std::string whoAmI(std::string serverurl, std::string tokenTemp)
{
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  std::string username = "";

  if (curl)
  {
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

    if (res == CURLE_OK)
    {
      rapidjson::Document jsonReply;
      jsonReply.Parse(jsonReplyString.c_str());
      // std::cout << " json reply whoami: " << jsonReplyString << std::endl;
      if (jsonReply["username"] != NULL)
      {
        username = jsonReply["username"].GetString();
        std::cout << username << std::endl;
      }
    }
    else
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }

    curl_slist_free_all(hs);
  }
  curl_easy_cleanup(curl);
  return username;
}

std::string getToken(std::string username, std::string password,
                     std::string serverurl)
{
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  std::string token_temp = "";

  if (curl)
  {
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

    if (res == CURLE_OK)
    {
      rapidjson::Document jsonReply;
      // std::cout << " json reply: " << jsonReplyString << std::endl;

      if (jsonReplyString == "Invalid Credentials\n")
      {
        std::cout << "Invalid credentials!" << std::endl;
      }
      if (jsonReplyString != "Invalid Credentials\n")
      {

        jsonReply.Parse(jsonReplyString.c_str());

        // Fetch token
        if (jsonReply["token"] != NULL)
        {
          std::cout << jsonReply["token"].GetString() << std::endl;
          token_temp = jsonReply["token"].GetString();

          // Since we got the token - we also update the profile's username.
          userinfo->username = whoAmI(serverurl, token_temp).c_str();
        }
      }
    }
    else
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
  }

  curl_easy_cleanup(curl);
  return token_temp;
}

std::string ReplaceAll(std::string str, const std::string &from,
                       const std::string &to)
{
  size_t start_pos = 0;

  while ((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return str;
}

std::string injectClickableLink(std::string statusText, std::string linkText)
{
  linkText = ReplaceAll(statusText, linkText, "<a href=\"" + linkText + "\"" + ">" + linkText + "</a>");
  return linkText;
}

bool FileExists(const std::string &Filename)
{
  return access(Filename.c_str(), 0) == 0;
}

// Download file from url..
void downloadFile(std::string fileUrl, std::string filename)
{
  CURL *curl;
  CURLcode fileResult;
  const char *url = fileUrl.c_str();
  curl = curl_easy_init();
  FILE *file;
  file = fopen(filename.c_str(), "wb");

  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                     "(KHTML, like Gecko) Chrome/62.0.3202.94 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    // Grab image
    fileResult = curl_easy_perform(curl);

    if (fileResult)
    {
      cout << "Cannot grab the file!\n"
           << fileResult << std::endl;
    }
    else
    {
      cout << "Successfully grabbed file! stored at: " << filename
           << fileResult << std::endl;
    }
  }
  curl_easy_cleanup(curl);
  fclose(file);
}

void button_reply_clicked(__attribute__((unused)) GtkLabel *lbl,
                          std::string subject)
{
  gtk_editable_set_text(GTK_EDITABLE(input_status), subject.c_str());
}

std::string getCleanLinkUrl(std::string link)
{
  link = ReplaceAll(link, "![](", "");
  link = ReplaceAll(link, ")", "");
  return link;
}

bool hasEnding(std::string const &fullString, std::string const &ending)
{
  if (fullString.length() >= ending.length())
  {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  }
  else
  {
    return false;
  }
}

void parseJsonStatuses(std::string jsonstring)
{
  Document document;
  document.Parse(jsonstring.c_str());
  const Value &twtsArray = document["twts"];
  std::string finalTwtString = "";
  int guiLineOffset = 0;

  if (twtsArray.IsArray())
  {
    for (SizeType index = 0; index < twtsArray.Size(); index++)
    {
      if (twtsArray[index]["markdownText"] != NULL)
      {
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
            false)
        {
          finalReplyString.append(post->authorUri.c_str());
        }

        if (twtsArray[index]["mentions"] != NULL)
        {
          const Value &mentionsArray = twtsArray[index]["mentions"];
          if (mentionsArray != NULL)
          {
            // appends others - not yourself.
            for (SizeType mentionIndex = 0; mentionIndex < mentionsArray.Size();
                 mentionIndex++)
            {
              // Check if the 'mentions' contains userinfo.username - skip if it
              // has the 'owners' name in it.
              std::string tmpString =
                  document["twts"][index]["mentions"][mentionIndex].GetString();

              if ((tmpString.find(userinfo->username) != string::npos) ==
                  false)
              {
                finalReplyString.append(tmpString);
                finalReplyString.append(" ");
              }
              else
              {
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

        if (post->avatarUrl != "")
        {
          if (!FileExists(filename))
          {
            downloadFile(post->avatarUrl, filename);
          }
        }

        post->created = ReplaceAll(post->created, "T", ":");
        post->created = ReplaceAll(post->created, "Z", ".");

        std::string postInfo = "\n" + post->created + "\n" + 
        "<a href=\"" + post->nickUrl + "\"" + ">" + 
        post->nick + "</a>" + " : \n\n";

        finalTwtString = twtsArray[index]["markdownText"].GetString();

        while (finalTwtString.find("<") != std::string::npos)
        {
          auto startpos = finalTwtString.find("<");
          auto endpos = finalTwtString.find(">") + 1;

          if (endpos != std::string::npos)
          {
            finalTwtString.erase(startpos, endpos - startpos);
          }
        }

        while (finalTwtString.find("(http") != std::string::npos)
        {
          auto startpos = finalTwtString.find("(");
          auto endpos = finalTwtString.find(")") + 1;

          if (endpos != std::string::npos)
          {
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

        post->status = postInfo + finalTwtString.c_str();

        if (twtsArray[index]["links"] != NULL)
        {
          const Value &linksArray = twtsArray[index]["links"];

          if (linksArray != NULL)
          {
            for (SizeType linkIndex = 0; linkIndex < linksArray.Size();
                 linkIndex++)
            {
              std::string linkString =
                  document["twts"][index]["links"][linkIndex].GetString();
              post->status = injectClickableLink(post->status, linkString);
            }
          }
        }

        GtkWidget *statusLabel = gtk_label_new(post->status.c_str());
        gtk_label_set_use_markup(GTK_LABEL(statusLabel), TRUE);
        gtk_label_set_wrap(GTK_LABEL(statusLabel), true);
        gtk_label_set_xalign(GTK_LABEL(statusLabel), 0);
        gtk_label_set_max_width_chars(GTK_LABEL(statusLabel), 150);
        gtk_grid_attach(GTK_GRID(timelineGrid), statusLabel, 1,
                        index + guiLineOffset, 1, 1);

        GtkWidget *avatar = gtk_image_new_from_file(filename.c_str());
        gtk_image_set_pixel_size(GTK_IMAGE(avatar), 50);
        gtk_grid_attach(GTK_GRID(timelineGrid), avatar, 0,
                        index + guiLineOffset, 1, 1);

        if (twtsArray[index]["links"] != NULL)
        {
          const Value &linksArray = twtsArray[index]["links"];
          if (linksArray != NULL)
          {
            for (SizeType linkIndex = 0; linkIndex < linksArray.Size();
                 linkIndex++)
            {
              std::string linkString =
                  document["twts"][index]["links"][linkIndex].GetString();
              linkString = getCleanLinkUrl(linkString);

              if (hasEnding(linkString, ".png") ||
                  hasEnding(linkString, ".jpg"))
              {
                std::string base_filename =
                    linkString.substr(linkString.find_last_of("/\\") + 1);
                if (!FileExists(base_filename))
                {
                  std::cout << std::endl
                            << "filename: " << base_filename << std::endl;
                  downloadFile(linkString, base_filename);
                }

                GtkWidget *attachedImage =
                    gtk_image_new_from_file(base_filename.c_str());
                gtk_image_set_pixel_size(GTK_IMAGE(attachedImage), 500);

                gtk_grid_attach(GTK_GRID(timelineGrid), attachedImage, 1,
                                index + guiLineOffset + 1, 1, 1);
                guiLineOffset += 1;
              }
            }
          }
        }

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

std::string getTimeline(std::string serverurl)
{
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  std::string jsonReplyString = "";

  if (currentTimelineName == NULL)
  {
    currentTimelineName = "discover";
  }

  if (curl)
  {
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

    if (res == CURLE_OK)
    {
      rapidjson::Document jsonReply;
      jsonReply.Parse(jsonReplyString.c_str());
      parseJsonStatuses(jsonReplyString);
      return "";
    }
    else
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    curl_slist_free_all(hs);
  }
  curl_easy_cleanup(curl);

  return jsonReplyString;
}

void refreshTimeline()
{
  GtkWidget *iter = gtk_widget_get_first_child(timelineGrid);
  while (iter != NULL)
  {
    GtkWidget *next = gtk_widget_get_next_sibling(iter);
    gtk_grid_remove(GTK_GRID(timelineGrid), iter);
    iter = next;
  }
  getTimeline(userinfo->serverUrl);
}

void selectedTimeline()
{
  auto selectedTimeLineIndex =
      gtk_drop_down_get_selected(GTK_DROP_DOWN(timelineDropDown));
  currentTimelineName = timelineNames[selectedTimeLineIndex];
  std::cout << std::endl
            << "selected timeline: " << currentTimelineName << std::endl;
  refreshTimeline();
}

void button_login_clicked(__attribute__((unused)) GtkLabel *lbl)
{
  userinfo = new UserInfo();
  userinfo->username = gtk_editable_get_text(GTK_EDITABLE(input_username));
  verifySSL = gtk_check_button_get_active(GTK_CHECK_BUTTON(checkbox_SSLVerify));

  std::string serverUrl = gtk_editable_get_text(GTK_EDITABLE(input_server));

  if (str_ends_with(serverUrl.c_str(), "/") == true)
  {
    std::cout << "Found '/' at end of url, will remove it." << std::endl;
    serverUrl = serverUrl.substr(0, serverUrl.size() - 1);
  }
  else
  {
    std::cout << "No '/' found at end of url, will use as is." << std::endl;
  }

  serverUrl = getSSLUrl(serverUrl, verifySSL);

  userinfo->serverUrl = serverUrl;
  userinfo->password = gtk_editable_get_text(GTK_EDITABLE(input_password));
  token = getToken(userinfo->username, userinfo->password, userinfo->serverUrl);

  if (token != "")
  {

    storeUserSettings();

    storePassword();

    GtkWidget *statusEntrySeparator =
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(statusEntrySeparator, true);

    GtkWidget *statusEntrySeparator2 =
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(statusEntrySeparator2, true);

    GtkWidget *statusEntrySeparator3 =
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(statusEntrySeparator3, true);

    std::cout << std::endl
              << "Token: " << token << std::endl;
    getTimeline(userinfo->serverUrl);

    timelineDropDown = gtk_drop_down_new_from_strings(timelineNames);
    g_signal_connect(timelineDropDown, "notify::selected",
                     G_CALLBACK(selectedTimeline), NULL);

    gtk_grid_attach(GTK_GRID(statusEntryGrid), input_status, 1, 0, 3, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), button_post_status, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), button_refresh_timeline, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), button_upload_media, 3, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), timelineDropDown, 4, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(statusEntryGrid), statusEntrySeparator, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), statusEntrySeparator2, 2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(statusEntryGrid), statusEntrySeparator3, 3, 2, 1, 1);

    gtk_widget_hide(input_password);
    gtk_widget_hide(input_server);
    gtk_widget_hide(button_login);
    gtk_widget_hide(input_server);
    gtk_widget_hide(input_username);
    gtk_widget_hide(checkbox_SSLVerify);
    gtk_widget_hide(checkbox_StoreUsernameServerUrl);
  }
}

std::string getSSLUrl(std::string url, bool verifySSL)
{
  if (verifySSL)
  {
    // Se if we have entered a http only url if ssl was enabled.
    if (url.find("http://") != string::npos)
    {
      replaceString(url, "http://", "https://");
      std::cout
          << "SSl is enabled, but http only url was used, forcing ssl for url."
          << std::endl;
      std::cout << url << std::endl;
    }
  }
  else
  {
    // Do the reverse if we have http only and have entered a https url
    if (url.find("https://") != string::npos)
    {
      replaceString(url, "https://", "http://");
      std::cout << "SSl is disabled, but https only url was used, forcing "
                   "non-ssl for url."
                << std::endl;
      std::cout << url << std::endl;
    }
  }
  return url;
}

void button_refresh_timeline_clicked(__attribute__((unused)) GtkLabel *lbl)
{
  refreshTimeline();
}

void checkTask(std::string taskURL)
{
  CURLcode ret;
  CURL *curl;
  curl = curl_easy_init();
  std::string replystring = "";

  cout << "checking task at url: " << taskURL << "\n";

  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
  curl_easy_setopt(curl, CURLOPT_URL, taskURL.c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.3.0");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &replystring);

  ret = curl_easy_perform(curl);
  cout << "replystring :" << replystring << "\n";

  Document document;
  document.Parse(replystring.c_str());

  if (document["state"] != NULL)
  {
    std::string taskStateString = document["state"].GetString();
    std::string errorString = document["error"].GetString();

    if (errorString != "")
    {
      cout << "Error from task is: " << errorString << "\n";
      curl_easy_cleanup(curl);
      curl = NULL;
    }
    else
    {
      cout << "state from task is: " << taskStateString << "\n";

      if (taskStateString == "complete")
      {
        cout << document["data"]["mediaURI"].GetString() << "\n";

        std::string imageurl = "";

        imageurl.append("![](");
        imageurl.append(document["data"]["mediaURI"].GetString());
        imageurl.append(") ");

        std::string statusTextAndImageUrl = gtk_editable_get_text(GTK_EDITABLE(input_status));
        statusTextAndImageUrl += " " + imageurl;

        gtk_editable_set_text(GTK_EDITABLE(input_status), statusTextAndImageUrl.c_str());

        curl_easy_cleanup(curl);
        curl = NULL;
      }
      else
      {
        cout << "Task was not complete (on server), waiting 2 seconds and check again.\n";
        curl_easy_cleanup(curl);
        curl = NULL;
        sleep(2);
        checkTask(taskURL);
      }
    }
  }
  curl_easy_cleanup(curl);
  curl = NULL;
}

/*
  Example reply for image that's been uploaded:
  replystring :{"state":"complete","error":"","data":{"mediaURI":"https://yarn.stigatle.no/media/image.png"}}
  meaning that if the task is not reporting 'complete' we then need to check the task again.
*/
std::string uploadMedia(std::string filepath, std::string token)
{
  cout << "uploading file from path: " << filepath << "\n";
  cout << "to url: " + userinfo->serverUrl + "/api/v1/upload"
       << "\n";

  CURL *curl;
  curl_mime *mime1;
  curl_mimepart *part1;
  struct curl_slist *slist1;
  CURLcode res;
  mime1 = NULL;
  slist1 = NULL;
  std::string tokenJson = ("Token: " + token);
  std::string jsonReplyString = "";
  std::string urlTemp = userinfo->serverUrl + "/api/v1/upload";

  slist1 = curl_slist_append(slist1, tokenJson.c_str());
  curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);
  curl_easy_setopt(curl, CURLOPT_URL, urlTemp.c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL);

  mime1 = curl_mime_init(curl);
  part1 = curl_mime_addpart(mime1);

  curl_mime_filedata(part1, filepath.c_str());
  curl_mime_name(part1, "media_file");

  curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime1);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                                            "(KHTML, like Gecko) Chrome/62.0.3202.94 Safari/537.36");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonReplyString);

  res = curl_easy_perform(curl);

  if (res == CURLE_OK)
  {
    rapidjson::Document jsonReply;

    if (jsonReply.Parse(jsonReplyString.c_str()).HasParseError())
    {
      std::cout << jsonReplyString.c_str();
    }
    else
    {
      cout << "path is: " << jsonReply["Path"].GetString() << "\n";
      checkTask(jsonReply["Path"].GetString());
    }
  }

  curl_easy_cleanup(curl);
  curl = NULL;
  curl_mime_free(mime1);
  mime1 = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;

  return jsonReplyString;
}

static void on_open_response(GtkDialog *dialog, int response)
{
  if (response == GTK_RESPONSE_ACCEPT)
  {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

    g_autoptr(GFile) file = gtk_file_chooser_get_file(chooser);
    string fileName = g_file_get_path(file);
    cout << "file name: " << fileName << endl;
    uploadMedia(fileName, token);
  }

  gtk_window_destroy(GTK_WINDOW(dialog));
}

void button_upload_media_clicked(__attribute__((unused)) GtkLabel *lbl)
{
  GtkWidget *dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  dialog = gtk_file_chooser_dialog_new("Open File",
                                       GTK_WINDOW(window),
                                       action,
                                       ("_Cancel"),
                                       GTK_RESPONSE_CANCEL,
                                       ("_Open"),
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);

  gtk_widget_show(dialog);

  g_signal_connect(dialog, "response", G_CALLBACK(on_open_response), NULL);
}

void button_post_status_clicked(__attribute__((unused)) GtkLabel *lbl)
{
  std::string status = gtk_editable_get_text(GTK_EDITABLE(input_status));
  postStatus(token, status, userinfo->serverUrl);
  gtk_editable_set_text(GTK_EDITABLE(input_status), "");
  refreshTimeline();
}

void storeUserSettings()
{
  std::ofstream out(userSettingsFile);
  out << userinfo->serverUrl << std::endl;
  out << userinfo->username << std::endl;
  out.close();
}

void readAndApplyUserSettings()
{
  if (std::experimental::filesystem::exists(userSettingsFile))
  {
    settingsFile.open(userSettingsFile, ios::in);
    if (settingsFile.is_open())
    {
      std::string settingsLine = "";
      for (int settingsIndex = 0; getline(settingsFile, settingsLine);
           settingsIndex++)
      {
        if (settingsIndex == 0)
        {
          gtk_editable_set_text(GTK_EDITABLE(input_server),
                                settingsLine.c_str());
        }
        if (settingsIndex == 1)
        {
          gtk_editable_set_text(GTK_EDITABLE(input_username),
                                settingsLine.c_str());
        }
      }
    }
  }
}

const SecretSchema *get_schema(void)
{
  static const SecretSchema the_schema = {
      "Yarn.Desktop.Password",
      SECRET_SCHEMA_NONE,
      {
          {"version", SECRET_SCHEMA_ATTRIBUTE_INTEGER},
          {"website", SECRET_SCHEMA_ATTRIBUTE_STRING},
          {"NULL"},
      }};
  return &the_schema;
}

static void on_password_stored(GObject *source, GAsyncResult *result,
                               gpointer unused)
{
  GError *error = NULL;

  secret_password_store_finish(result, &error);
  if (error != NULL)
  {
    /* ... handle the failure here */
    g_error_free(error);
  }
  else
  {
    std::cout << "password has been stored..\n";
  }
}

void storePassword()
{
  std::cout << "Storing password!\n";

  secret_password_store(PASSWORD_SCHEMA, SECRET_COLLECTION_DEFAULT,
                        "Yarn Desktop Client", userinfo->password.c_str(), NULL,
                        on_password_stored, NULL, "version", 0, "website",
                        "https://github.com/stig-atle/YarnDesktopClient", NULL);
}

void getpassword()
{
  GError *error = NULL;

  gchar *password = secret_password_lookup_sync(
      PASSWORD_SCHEMA, NULL, &error, "website",
      "https://github.com/stig-atle/YarnDesktopClient", "version", 0, NULL);

  if (error != NULL)
  {
    cout << "error!\n";
    g_error_free(error);
  }
  else if (password == NULL)
  {
    cout << "password was null\n";
  }
  else
  {
    std::string retrievedPassword(password);
    gtk_editable_set_text(GTK_EDITABLE(input_password),
                          retrievedPassword.c_str());
    secret_password_free(password);
  }
}

static void activate(GtkApplication *app,
                     __attribute__((unused)) gpointer user_data)
{
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

  checkbox_StoreUsernameServerUrl =
      gtk_check_button_new_with_label("Store username, serverurl locally. "
                                      "(password is placed in secure storage)");

  gtk_check_button_set_active(GTK_CHECK_BUTTON(checkbox_StoreUsernameServerUrl),
                              true);

  input_status = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_status), "Enter status");

  button_post_status = gtk_button_new_with_label("Post status");
  g_signal_connect(button_post_status, "clicked",
                   G_CALLBACK(button_post_status_clicked), NULL);

  button_refresh_timeline = gtk_button_new_with_label("Refresh timeline");
  g_signal_connect(button_refresh_timeline, "clicked",
                   G_CALLBACK(button_refresh_timeline_clicked), NULL);

  button_upload_media = gtk_button_new_with_label("Upload media");
  g_signal_connect(button_upload_media, "clicked",
                   G_CALLBACK(button_upload_media_clicked), NULL);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
                                timelineGrid);

  gtk_grid_set_column_homogeneous(GTK_GRID(statusEntryGrid), true);
  gtk_grid_set_column_homogeneous(GTK_GRID(timelineGrid), false);

  gtk_widget_set_hexpand(timelineGrid, false);
  gtk_widget_set_vexpand(timelineGrid, true);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  gtk_grid_attach(GTK_GRID(timelineGrid), input_username, 0, 0, 3, 1);
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_username), "Username");
  gtk_grid_attach(GTK_GRID(timelineGrid), input_password, 0, 1, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), input_server, 0, 2, 3, 1);
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_server), "Url");
  gtk_entry_set_placeholder_text(GTK_ENTRY(input_status), "Enter status");
  gtk_grid_attach(GTK_GRID(timelineGrid), button_login, 0, 3, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), checkbox_SSLVerify, 0, 4, 3, 1);
  gtk_grid_attach(GTK_GRID(timelineGrid), checkbox_StoreUsernameServerUrl, 0, 5,
                  3, 1);
  gtk_box_append(GTK_BOX(gridParent), scrolled_window);

  gtk_window_set_child(GTK_WINDOW(window), gridParent);

  gtk_widget_show(window);
  getpassword();
  readAndApplyUserSettings();
}

int main(int argc, char **argv)
{
  int status;

  app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return (status);
}