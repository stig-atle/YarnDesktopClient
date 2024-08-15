import 'package:flutter/material.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:file_picker/file_picker.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';

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
          // title: const Text('Yarn Desktop Client'),
          actions: [
            IconButton(
              icon: Icon(_isDarkMode ? Icons.light_mode : Icons.dark_mode),
              onPressed: _toggleTheme,
            ),
          ],
        ),
        body: Center(
          child: AuthWidget(toggleTheme: _toggleTheme),
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
  final ValueNotifier<List<dynamic>> _discoverTimeline = ValueNotifier([]);
  final ValueNotifier<List<dynamic>> _userTimeline = ValueNotifier([]);
  final ValueNotifier<List<dynamic>> _mentionsTimeline = ValueNotifier([]);
  bool _isLoading = false;
  bool _isLoggedIn = false;
  String _statusMessage = "";
  String _token = "";
  late TabController _tabController;

@override
void initState() {
  super.initState();
  
  // Initialize the controllers with default values or from secure storage
  _initializeControllers();

  _tabController = TabController(length: 3, vsync: this);
  _tabController.addListener(_handleTabSelection);
}

Future<void> _initializeControllers() async {
  const storage = FlutterSecureStorage();

  // Retrieve stored values or set default values
  String? storedUsername = await storage.read(key: 'username');
  String? storedPassword = await storage.read(key: 'password');
  String? storedServerUrl = await storage.read(key: 'server');

  setState(() {
    _usernameController.text = storedUsername ?? '';
    _passwordController.text = storedPassword ?? '';
    _serverUrlController.text = storedServerUrl ?? '';
  });
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

  void _handleTabSelection() async {
    if (_tabController.indexIsChanging) {
      return;
    }
    switch (_tabController.index) {
      case 0:
        await _fetchTimeline('discover');
        break;
      case 1:
        await _fetchTimeline('timeline');
        break;
      case 2:
        await _fetchTimeline('mentions');
        break;
    }
  }

  Future<String> getToken(
      String username, String password, String serverUrl) async {
    
    const storage = FlutterSecureStorage();
    await storage.write(key: 'username', value: username);
    await storage.write(key: 'password', value: password);
    await storage.write(key: 'server', value: serverUrl);

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
    await _fetchTimeline('discover');
    await _fetchTimeline('timeline');
    await _fetchTimeline('mentions');
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

      final Map<String, dynamic> jsonResponse = jsonDecode(responseBody);
      final String mediaPath = jsonResponse['Path'];
      setState(() {
        _statusController.text += " ![]($mediaPath)";
      });
    } else {
      throw Exception('Failed to upload media: ${response.reasonPhrase}');
    }
  }

List<Widget> parseStatusText(String text, String subjectToRemove) {
    final userUrlRegex = RegExp(r'@<(\w+)\s+([^>]+?)>');

    text = text.replaceAllMapped(userUrlRegex, (match) {
      return '@${match.group(1)}';
    });

    final List<Widget> widgets = [];
    final regex = RegExp(r'!\[[^\]]*\]\((.*?)\)');

    int lastMatchEnd = 0;

    text = text.replaceAll(subjectToRemove, '');
    final matches = regex.allMatches(text);
    for (final match in matches) {
      final imageUrl = match.group(1);

      if (lastMatchEnd < match.start) {
        widgets.add(SelectableText(text.substring(lastMatchEnd, match.start)));
      }

      if (imageUrl != null && imageUrl.isNotEmpty) {
        widgets.add(Image.network(imageUrl));
      }

      lastMatchEnd = match.end;
    }

    if (lastMatchEnd < text.length) {
      widgets.add(SelectableText(text.substring(lastMatchEnd)));
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
    _discoverTimeline.value =
        await getTimeline(serverUrl, tokenTemp, 'discover');
    _userTimeline.value = await getTimeline(serverUrl, tokenTemp, 'timeline');
    _mentionsTimeline.value =
        await getTimeline(serverUrl, tokenTemp, 'mentions');
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
          _discoverTimeline.value =
              await getTimeline(serverUrl, _token, endpoint);
          break;
        case 'timeline':
          _userTimeline.value = await getTimeline(serverUrl, _token, endpoint);
          break;
        case 'mentions':
          _mentionsTimeline.value =
              await getTimeline(serverUrl, _token, endpoint);
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

  void _postStatus() async {
    String status = _statusController.text.trim();
    if (status.isEmpty) return;

    try {
      setState(() {
        _isLoading = true;
        _statusMessage = "Posting status...";
      });

      await postStatus(_token, status, _serverUrlController.text.trim());
      _statusController.clear();
      await _fetchTimeline('timeline');
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

  void _pickFile() async {
    FilePickerResult? result = await FilePicker.platform.pickFiles();
    if (result != null && result.files.single.path != null) {
      try {
        await uploadMedia(result.files.single.path!, _token);
      } catch (e) {
        setState(() {
          _statusMessage = 'Error: ${e.toString()}';
        });
      }
    }
  }

void _replyToPost(String postHash, String postText, String postFeedUrl) {
  
  _statusController.text = "";
  // Regular expression to find all @<username url> mentions in the postText
  final RegExp mentionRegex = RegExp(r'@<\w+\s+[^>]+>');

  // Find all matches in the postText
  final Iterable<RegExpMatch> matches = mentionRegex.allMatches(postText);

  // Extract the actual mentions from the matches and filter out those containing _username
  final List<String?> mentions = matches
      .map((match) => match.group(0))
      .where((mention) => !mention!.contains(_username))
      .toList();

  setState(() {
    // Add all the filtered mentions and the postHash to _statusController.text
    _statusController.text += "$postHash $postFeedUrl ${mentions.join(' ')}";
  });
}


  @override
  Widget build(BuildContext context) {
    if (!_isLoggedIn) {
      return Padding(
        padding: const EdgeInsets.all(16.0),
        child: _isLoading
            ? const Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  CircularProgressIndicator(),
                  SizedBox(height: 20),
                  Text("Logging in... Please wait"),
                ],
              )
            : Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
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
                  ElevatedButton(
                    onPressed: _fetchData,
                    child: const Text('Login'),
                  ),
                  if (_statusMessage.isNotEmpty)
                    Padding(
                      padding: const EdgeInsets.only(top: 20),
                      child: Text(_statusMessage),
                    ),
                ],
              ),
      );
    } else {
      return Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Row(
              children: [
                const SizedBox(width: 8),
                Text(
                  _username,
                  style: const TextStyle(fontSize: 18),
                ),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.logout),
                  onPressed: () {
                    setState(() {
                      _isLoggedIn = false;
                    });
                  },
                ),
              ],
            ),
            const SizedBox(height: 10),
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
                  _buildTimeline(_discoverTimeline),
                  _buildTimeline(_userTimeline),
                  _buildTimeline(_mentionsTimeline),
                ],
              ),
            ),
            TextField(
              controller: _statusController,
              decoration: const InputDecoration(
                labelText: 'Post a status',
              ),
              minLines: 1,
              maxLines: 5,
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.end,
              children: [
                IconButton(
                  icon: const Icon(Icons.attach_file),
                  onPressed: _pickFile,
                ),
                IconButton(
                  icon: const Icon(Icons.send),
                  onPressed: _postStatus,
                ),
              ],
            ),
          ],
        ),
      );
    }
  }

  Widget _buildTimeline(ValueNotifier<List<dynamic>> timeline) {
    return ValueListenableBuilder<List<dynamic>>(
      valueListenable: timeline,
      builder: (context, value, child) {
        if (value.isEmpty) {
          return const Center(child: Text("No posts available."));
        } else {
          return RefreshIndicator(
            onRefresh: () async {
              await _fetchTimeline(timeline == _discoverTimeline
                  ? 'discover'
                  : timeline == _userTimeline
                      ? 'timeline'
                      : 'mentions');
            },
            child: ListView.separated(
              itemCount: value.length,
              itemBuilder: (context, index) {
                final post = value[index];
                final username = post['twter']['nick'] ?? 'Unknown';
                final avatarUrl = post['twter']['avatar'] ?? '';
                final postSubject = post['subject'] ?? '';
                final postFeedUrl = "${"${"@<" + username} " + post['twter']['uri']}>";

                return ListTile(
                  leading: CircleAvatar(
                    backgroundImage:
                        avatarUrl.isNotEmpty ? NetworkImage(avatarUrl) : null,
                    child: avatarUrl.isEmpty
                        ? Text(username[0].toUpperCase())
                        : null,
                  ),
                  title: Text(username),
                  subtitle: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      ...parseStatusText(post['text'] ?? '', postSubject),
                      IconButton(
                        icon: const Icon(Icons.reply),
                        onPressed: () => _replyToPost(postSubject, post['text'], postFeedUrl),
                      ),
                    ],
                  ),
                );
              },
              separatorBuilder: (context, index) => const Divider(),
            ),
          );
        }
      },
    );
  }
}
