import 'package:flutter/material.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:file_picker/file_picker.dart';

void main() {
  runApp(const MainApp());
}

class MainApp extends StatefulWidget {
  const MainApp({super.key});

  @override
  _MainAppState createState() => _MainAppState();
}

class _MainAppState extends State<MainApp> {
  bool _isDarkMode = true;

  void _toggleTheme() {
    setState(() {
      _isDarkMode = !_isDarkMode;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      theme: _isDarkMode ? ThemeData.dark() : ThemeData.light(),
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Yarn Desktop Client'),
          actions: [
            IconButton(
              icon: Icon(_isDarkMode ? Icons.light_mode : Icons.dark_mode),
              onPressed: _toggleTheme,
            ),
          ],
        ),
        body: Center(
          child: AuthWidget(
            toggleTheme: _toggleTheme,
          ),
        ),
      ),
    );
  }
}

class AuthWidget extends StatefulWidget {
  final Function toggleTheme;

  const AuthWidget({super.key, required this.toggleTheme});

  @override
  _AuthWidgetState createState() => _AuthWidgetState();
}

class _AuthWidgetState extends State<AuthWidget>
    with SingleTickerProviderStateMixin {
  final TextEditingController _serverUrlController = TextEditingController();
  final TextEditingController _usernameController = TextEditingController();
  final TextEditingController _passwordController = TextEditingController();
  final TextEditingController _statusController = TextEditingController();
  String _username = "Unknown";
  List<dynamic> _discoverTimeline = [];
  List<dynamic> _userTimeline = [];
  List<dynamic> _mentionsTimeline = [];
  bool _isLoading = false;
  bool _isLoggedIn = false;
  String _statusMessage = "";
  String _token = "";
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
    _tabController.addListener(_handleTabSelection);
  }

  @override
  void dispose() {
    _serverUrlController.dispose();
    _usernameController.dispose();
    _passwordController.dispose();
    _statusController.dispose();
    _tabController.removeListener(_handleTabSelection);
    _tabController.dispose();
    super.dispose();
  }

  void _handleTabSelection() {
    if (_tabController.indexIsChanging) {
      return;
    }
    switch (_tabController.index) {
      case 0:
        _fetchTimeline('discover');
        break;
      case 1:
        _fetchTimeline('timeline');
        break;
      case 2:
        _fetchTimeline('mentions');
        break;
    }
  }

  Future<String> getToken(
      String username, String password, String serverUrl) async {
    final String apiUrl = "$serverUrl/api/v1/auth";
    final response = await http.post(
      Uri.parse(apiUrl),
      headers: {"Content-Type": "application/json"},
      body: jsonEncode({"username": username, "password": password}),
    );

    if (response.statusCode == 200) {
      final Map<String, dynamic> jsonResponse = jsonDecode(response.body);
      if (jsonResponse.containsKey('token')) {
        return jsonResponse['token'];
      } else {
        throw Exception('Token not found in response.');
      }
    } else if (response.body == "Invalid Credentials\n") {
      throw Exception('Invalid credentials!');
    } else {
      throw Exception('Failed to authenticate: ${response.reasonPhrase}');
    }
  }

  Future<String> whoAmI(String serverUrl, String tokenTemp) async {
    final String apiUrl = "$serverUrl/api/v1/whoami";
    final response = await http.get(
      Uri.parse(apiUrl),
      headers: {
        "Content-Type": "application/json",
        "token": tokenTemp,
      },
    );

    if (response.statusCode == 200) {
      final Map<String, dynamic> jsonResponse = jsonDecode(response.body);
      if (jsonResponse.containsKey('username')) {
        return jsonResponse['username'];
      } else {
        throw Exception('Username not found in response.');
      }
    } else {
      throw Exception('Failed to load user data: ${response.reasonPhrase}');
    }
  }

  Future<List<dynamic>> getTimeline(
      String serverUrl, String tokenTemp, String endpoint) async {
    final String apiUrl = "$serverUrl/api/v1/$endpoint";
    final response = await http.post(
      Uri.parse(apiUrl),
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
        "token": tokenTemp,
      },
      body: '{}', // Sending an empty JSON object as the body
    );

    if (response.statusCode == 200) {
      final Map<String, dynamic> jsonResponse = jsonDecode(response.body);
      if (jsonResponse.containsKey('twts')) {
        return jsonResponse['twts'];
      } else {
        throw Exception('twts not found in response.');
      }
    } else {
      throw Exception('Failed to load timeline: ${response.reasonPhrase}');
    }
  }

  Future<void> postStatus(String token, String status, String serverUrl) async {
    final String apiUrl = "$serverUrl/api/v1/post";
    final response = await http.post(
      Uri.parse(apiUrl),
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
        "token": token,
      },
      body: jsonEncode({"text": status}),
    );

    if (response.statusCode != 200) {
      throw Exception('Failed to post status: ${response.reasonPhrase}');
    }
  }

  Future<void> uploadMedia(String filePath, String token) async {
    final String uploadUrl =
        "${_serverUrlController.text.trim()}/api/v1/upload";
    final request = http.MultipartRequest('POST', Uri.parse(uploadUrl));
    request.headers['token'] = token;
    request.files
        .add(await http.MultipartFile.fromPath('media_file', filePath));

    final response = await request.send();

    if (response.statusCode == 200) {
      final responseBody = await response.stream.bytesToString();

      // Debugging: Print the response body
      //print("Response body: $responseBody");

      //try {
      final Map<String, dynamic> jsonResponse = jsonDecode(responseBody);
      final String mediaPath = jsonResponse['Path'];
      //print("mediapath: $mediaPath");
      setState(() {
        _statusController.text += " ![]($mediaPath)";
      });
      //await checkTask(taskUrl, token);
      /* if (jsonResponse.containsKey('Path')) {
          final String taskUrl = jsonResponse['Path'];
          await checkTask(taskUrl, token);
        } else {
          throw Exception('Invalid response format: "Path" key not found');
        }
      } catch (e) {
        // If the response is not in JSON format, print the error and the response body
        print("Error decoding JSON: $e");
        print("Response body: $responseBody");
        throw Exception('Failed to decode server response: $e');
      }
    } else {
      throw Exception('Failed to upload media: ${response.reasonPhrase}');
    }*/
    }
  }

  Future<void> checkTask(String taskUrl, String token) async {
    while (true) {
      final response = await http.get(
        Uri.parse(taskUrl),
        headers: {
          "Content-Type": "application/json",
          "token": token,
        },
      );

      if (response.statusCode == 200) {
        final Map<String, dynamic> jsonResponse =
            jsonDecode(response.body) as Map<String, dynamic>;

        final String taskState = jsonResponse['state'];
        final String error = jsonResponse['error'];

        if (error.isNotEmpty) {
          throw Exception('Error from task: $error');
        } else if (taskState == 'complete') {
          final String mediaUri = jsonResponse['data']['mediaURI'];
          setState(() {
            _statusController.text += " ![]($mediaUri)";
          });
          return;
        } else {
          await Future.delayed(const Duration(seconds: 2));
        }
      } else {
        throw Exception('Failed to check task: ${response.reasonPhrase}');
      }
    }
  }

  List<Widget> parseStatusText(String text) {
    final List<Widget> widgets = [];
    final regex = RegExp(r'!\[\]\((.*?)\)|(@\w+)');
    final matches = regex.allMatches(text);
    int lastMatchEnd = 0;

    for (final match in matches) {
      final imageUrl = match.group(1);

      if (lastMatchEnd < match.start) {
        widgets.add(Text(text.substring(lastMatchEnd, match.start)));
      }

      if (imageUrl != null && imageUrl.isNotEmpty) {
        widgets.add(Image.network(imageUrl));
      }

      lastMatchEnd = match.end;
    }

    if (lastMatchEnd < text.length) {
      widgets.add(Text(text.substring(lastMatchEnd)));
    }

    return widgets;
  }

  void _fetchData() async {
    setState(() {
      _isLoading = true;
      _statusMessage = "Logging in...";
    });
    try {
      String serverUrl = _serverUrlController.text.trim();
      String username = _usernameController.text.trim();
      String password = _passwordController.text.trim();

      if (serverUrl.isEmpty || username.isEmpty || password.isEmpty) {
        throw Exception('All fields are required.');
      }

      _token = await getToken(username, password, serverUrl);
      String fetchedUsername = await whoAmI(serverUrl, _token);

      setState(() {
        _statusMessage = "Fetching timelines...";
      });

      await _fetchAllTimelines(serverUrl, _token);

      setState(() {
        _username = fetchedUsername;
        _isLoggedIn = true;
        _statusMessage = "Logged in and timelines fetched successfully.";
      });
    } catch (e) {
      setState(() {
        _statusMessage = 'Error: ${e.toString()}';
      });
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }

  Future<void> _fetchAllTimelines(String serverUrl, String tokenTemp) async {
    _discoverTimeline = await getTimeline(serverUrl, tokenTemp, 'discover');
    _userTimeline = await getTimeline(serverUrl, tokenTemp, 'timeline');
    _mentionsTimeline = await getTimeline(serverUrl, tokenTemp, 'mentions');
  }

  Future<void> _fetchTimeline(String endpoint) async {
    setState(() {
      _isLoading = true;
      _statusMessage = "Refreshing $endpoint...";
    });

    try {
      String serverUrl = _serverUrlController.text.trim();
      switch (endpoint) {
        case 'discover':
          _discoverTimeline = await getTimeline(serverUrl, _token, 'discover');
          break;
        case 'timeline':
          _userTimeline = await getTimeline(serverUrl, _token, 'timeline');
          break;
        case 'mentions':
          _mentionsTimeline = await getTimeline(serverUrl, _token, 'mentions');
          break;
      }

      setState(() {
        _statusMessage = "$endpoint refreshed successfully.";
      });
    } catch (e) {
      setState(() {
        _statusMessage = 'Error: ${e.toString()}';
      });
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }

  void _postNewStatus() async {
    final String status = _statusController.text.trim();
    if (status.isEmpty) {
      setState(() {
        _statusMessage = 'Status cannot be empty.';
      });
      return;
    }

    try {
      await postStatus(_token, status, _serverUrlController.text.trim());
      setState(() {
        _statusMessage = 'Status posted successfully.';
        _statusController.clear();
        _fetchData();
      });
    } catch (e) {
      setState(() {
        _statusMessage = 'Error: ${e.toString()}';
      });
    }
  }

  void _replyToStatus(String hash) {
    _statusController.text = '$hash ';
    FocusScope.of(context).requestFocus(FocusNode());
  }

  Future<void> _pickAndUploadMedia() async {
    FilePickerResult? result = await FilePicker.platform.pickFiles();
    if (result != null && result.files.single.path != null) {
      try {
        await uploadMedia(result.files.single.path!, _token);
        setState(() {
          _statusMessage = 'Media uploaded successfully.';
        });
      } catch (e) {
        setState(() {
          _statusMessage = 'Error: ${e.toString()}';
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(16.0),
      child: Column(
        children: [
          if (!_isLoggedIn) ...[
            TextField(
              controller: _serverUrlController,
              decoration: const InputDecoration(labelText: 'Server URL'),
            ),
            TextField(
              controller: _usernameController,
              decoration: const InputDecoration(labelText: 'Username'),
            ),
            TextField(
              controller: _passwordController,
              decoration: const InputDecoration(labelText: 'Password'),
              obscureText: true,
            ),
            const SizedBox(height: 20),
            _isLoading
                ? const CircularProgressIndicator()
                : ElevatedButton(
                    onPressed: _fetchData,
                    child: const Text('Login'),
                  ),
          ] else ...[
            Text('Welcome, $_username!'),
            const SizedBox(height: 20),
            _isLoading
                ? const CircularProgressIndicator()
                : Expanded(
                    child: Column(
                      children: [
                        TabBar(
                          controller: _tabController,
                          tabs: const [
                            Tab(text: 'Discover'),
                            Tab(text: 'Timeline'),
                            Tab(text: 'Mentions'),
                          ],
                        ),
                        Expanded(
                          child: TabBarView(
                            controller: _tabController,
                            children: [
                              _buildTimelineView(_discoverTimeline),
                              _buildTimelineView(_userTimeline),
                              _buildTimelineView(_mentionsTimeline),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),
            const SizedBox(height: 20),
            TextField(
              controller: _statusController,
              decoration: const InputDecoration(labelText: 'New status'),
            ),
            const SizedBox(height: 10),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton(
                    onPressed: _postNewStatus,
                    child: const Text('Post Status'),
                  ),
                ),
                const SizedBox(width: 10),
                ElevatedButton(
                  onPressed: _pickAndUploadMedia,
                  child: const Icon(Icons.upload),
                ),
              ],
            ),
            const SizedBox(height: 20),
            Text(_statusMessage),
          ],
        ],
      ),
    );
  }

  Widget _buildTimelineView(List<dynamic> timeline) {
    return ListView.separated(
      itemCount: timeline.length,
      separatorBuilder: (context, index) => const Divider(),
      itemBuilder: (context, index) {
        var post = timeline[index];
        String avatarUrl = post['twter']['avatar'] ?? '';
        String statusText = post['text'] ?? 'No text available';
        String hash = post['subject'] ?? '';

        statusText = statusText.replaceAll(hash, '');

        return ListTile(
          leading: avatarUrl.isNotEmpty
              ? CircleAvatar(
                  backgroundImage: NetworkImage(avatarUrl),
                )
              : const CircleAvatar(
                  child: Icon(Icons.person),
                ),
          title: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              ...parseStatusText(statusText),
            ],
          ),
          subtitle: Row(
            children: [
              Text('Author: ${post['twter']['nick'] ?? 'Unknown'}'),
              const Spacer(),
              IconButton(
                icon: const Icon(Icons.reply),
                onPressed: () => _replyToStatus(hash),
              ),
            ],
          ),
        );
      },
    );
  }
}

class UserProfilePage extends StatelessWidget {
  final String url;

  const UserProfilePage({super.key, required this.url});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('User Profile')),
      body: Center(
        child: Text('User Profile Page (URL: $url)'),
      ),
    );
  }
}
