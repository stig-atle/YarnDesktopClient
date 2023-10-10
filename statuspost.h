#pragma once

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
  vector<string> links;
  string authorUri = "";
};
