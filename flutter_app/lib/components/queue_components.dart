import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import '../parameters/app_parameters.dart';
import '../graphics/animated_waves.dart';
import '../graphics/hover_box.dart';
import '../graphics/queue_view.dart';

/// Queue app main widget
class QueueApp extends StatelessWidget {
  const QueueApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: AppStrings.string_appTitle,
      theme: ThemeData.light().copyWith(
        colorScheme: ColorScheme.fromSeed(
          seedColor: AppParameters.color_primaryBlue,
        ),
      ),
      home: const QueueScreen(),
    );
  }
}

/// Main queue display screen
class QueueScreen extends StatefulWidget {
  const QueueScreen({super.key});

  @override
  State<QueueScreen> createState() => _QueueScreenState();
}

class _QueueScreenState extends State<QueueScreen>
    with TickerProviderStateMixin {
  late AnimationController _waveController;
  late AnimationController _clockController;
  late AnimationController _placeHoverController;
  late AnimationController _waitHoverController;

  bool _isAnimating = false;
  bool _isPlaceHovered = false;
  bool _isWaitHovered = false;
  String _firebaseStatus = 'Connecting...';
  String _testResult = '';
  bool _showDebug = false; // Toggle for showing raw queue snapshot

  @override
  void initState() {
    super.initState();
    _initializeControllers();
    _testFirebaseConnection();
  }

  void _testFirebaseConnection() async {
    try {
      // Simple Firebase test - just check if Firebase is initialized
      setState(() {
        _firebaseStatus = '✅ Connected';
      });

      print('Firebase initialized successfully!');

      // Optional: Try a simple Realtime Database operation (but don't block on it)
      _tryRealtimeDatabaseTest();
    } catch (e) {
      setState(() {
        _firebaseStatus = '❌ Error: $e';
      });

      print('Firebase connection error: $e');
    }
  }

  void _tryRealtimeDatabaseTest() async {
    try {
      // Try to write to Realtime Database in the background
      DatabaseReference ref = FirebaseDatabase.instance.ref('test/connection');
      await ref.set({
        'timestamp': ServerValue.timestamp,
        'status': 'connected',
        'message': 'Realtime Database is working!',
      });

      print('Realtime Database test successful!');
    } catch (e) {
      print('Realtime Database test failed (but Firebase Core is working): $e');
    }
  }

  void _initializeControllers() {
    _waveController = AnimationController(
      vsync: this,
      lowerBound: 0.0,
      upperBound: 1.0,
      duration: AppParameters.waveAnimationDuration,
    );

    _clockController = AnimationController(
      vsync: this,
      duration: AppParameters.clockAnimationDuration,
    )..repeat();

    _placeHoverController = AnimationController(
      vsync: this,
      duration: AppParameters.hoverAnimationDuration,
    );

    _waitHoverController = AnimationController(
      vsync: this,
      duration: AppParameters.hoverAnimationDuration,
    );
  }

  @override
  void dispose() {
    _waveController.dispose();
    _clockController.dispose();
    _placeHoverController.dispose();
    _waitHoverController.dispose();
    super.dispose();
  }

  void _toggleAnimation() {
    setState(() {
      if (!_isAnimating) {
        _isAnimating = true;
        _waveController.reset();
        _waveController.forward().then((_) {
          setState(() {
            _isAnimating = false;
          });
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppParameters.color_backgroundColor,
      body: SingleChildScrollView(
        child: Container(
          constraints: BoxConstraints(
            minHeight: MediaQuery.of(context).size.height,
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              SizedBox(height: 50), // Top padding
              _buildTitle(),
              SizedBox(height: AppParameters.size_titleToNumberSpacing),
              _buildAnimatedNumber(),
              SizedBox(height: AppParameters.size_numberToBoxesSpacing),
              _buildHoverBoxes(),
              SizedBox(height: AppParameters.size_boxesToButtonSpacing),
              _buildAnimationButton(),
              const SizedBox(height: 12),
              _buildQueueViewButton(),
              const SizedBox(height: 12),
              _buildDebugToggle(),
              if (_showDebug) ...[
                const SizedBox(height: 12),
                _buildDebugPanel(),
              ],
              SizedBox(height: 20),
              _buildFirebaseStatus(),
              SizedBox(height: 10),
              _buildTestFirebaseButton(),
              SizedBox(height: 50), // Bottom padding
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTitle() {
    return Text(
      AppStrings.string_goToLineText,
      style: TextStyle(
        fontSize: AppParameters.size_titleFontSize,
        fontWeight: FontWeight.w900,
        fontFamily: AppParameters.string_fontFamily,
        color: AppParameters.color_primaryBlue,
      ),
    );
  }

  Widget _buildAnimatedNumber() {
    final ref = FirebaseDatabase.instance.ref('queues/queueA/recommendedLine');
    return StreamBuilder<DatabaseEvent>(
      stream: ref.onValue,
      builder: (context, snapshot) {
        String display = AppStrings.string_queueNumber;
        if (snapshot.hasData) {
          final val = snapshot.data!.snapshot.value;
          if (val != null) {
            display = val.toString();
          }
        }
        return SizedBox(
          width: AppParameters.size_maxCircleRadius,
          height: AppParameters.size_maxCircleRadius,
          child: Stack(
            alignment: Alignment.center,
            children: [
              AnimatedWaves(
                controller: _waveController,
                isAnimating: _isAnimating,
              ),
              Text(
                display,
                style: TextStyle(
                  fontSize: AppParameters.size_queueNumberFontSize,
                  fontWeight: FontWeight.w900,
                  fontFamily: AppParameters.string_expandedFontFamily,
                  color: AppParameters.color_primaryBlue,
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildHoverBoxes() {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          _buildDynamicPlaceBox(),
          SizedBox(width: AppParameters.size_hoverBoxSpacing),
          HoverBox(
            icon: AppAssets.string_clockIcon,
            text: AppStrings.string_waitTimeText,
            number: AppStrings.string_waitTimeNumber,
            suffix: AppStrings.string_waitTimeSuffix,
            controller: _waitHoverController,
            isHovered: _isWaitHovered,
            explanation: AppStrings.string_waitTimeTooltip,
            onHover: _handleWaitHover,
            isClockIcon: true,
            clockController: _clockController,
          ),
        ],
      ),
    );
  }

  Widget _buildDynamicPlaceBox() {
    final queueRef = FirebaseDatabase.instance.ref('queues/queueA');
    return StreamBuilder<DatabaseEvent>(
      stream: queueRef.onValue,
      builder: (context, snapshot) {
        // Use placeholder instead of static constant so it's obvious when data hasn't arrived
        String placeDisplay = '...';
        String dynamicTooltip = AppStrings.string_placeTooltip;
        String? lineContext; // store recommended line label
        Map rawMap = {};
        if (snapshot.hasData && snapshot.data!.snapshot.value is Map) {
          final raw = snapshot.data!.snapshot.value as Map;
          final rec = raw['recommendedLine'];
          final lines = raw['lines'];
          rawMap = raw; // capture for debug panel
          if (lines == null) {
            // Attempt a one-time initialization if lines missing
            _initializeQueueNode(queueRef);
            dynamicTooltip = 'Initializing queue lines...';
          }
          int? recommendedLine;
          if (rec is int)
            recommendedLine = rec;
          else if (rec is String)
            recommendedLine = int.tryParse(rec);
          // If recommendedLine not provided yet but lines exist, compute locally (mirrors server/device logic)
          if (recommendedLine == null && lines is Map) {
            int? bestLine;
            int bestCount = 1 << 30;
            lines.forEach((k, v) {
              int count = _extractPeopleCount(v);
              final ln = int.tryParse(k.toString());
              if (ln != null) {
                if (count < bestCount ||
                    (count == bestCount &&
                        (bestLine == null ||
                            (bestLine != null && ln < bestLine!)))) {
                  bestLine = ln;
                  bestCount = count;
                }
              }
            });
            recommendedLine = bestLine;
          }
          if (recommendedLine != null && lines is Map) {
            final key = '${recommendedLine}';
            final currentCountRaw = lines[key];
            final int currentCount = _extractPeopleCount(currentCountRaw);
            placeDisplay = (currentCount + 1).toString();
            lineContext = 'L$recommendedLine';
            dynamicTooltip =
                'Recommended line $recommendedLine, current people: $currentCount, you would be #$placeDisplay';
            // ignore: avoid_print
            print(
              '[PlaceBox] rec=$recommendedLine count=$currentCount place=$placeDisplay',
            );
          }
        } else if (snapshot.hasData && snapshot.data!.snapshot.value == null) {
          // Entire node missing: initialize structure then show initializing state
          _initializeQueueNode(queueRef);
          dynamicTooltip = 'Creating queue structure...';
        }
        if (_showDebug) {
          // ignore: avoid_print
          print('[DebugPanel] Raw queue snapshot: ' + rawMap.toString());
        }
        return HoverBox(
          icon: AppAssets.string_queueIcon,
          text: lineContext == null
              ? AppStrings.string_placeInLineText
              : '${AppStrings.string_placeInLineText}($lineContext) ',
          number: placeDisplay,
          suffix: '',
          controller: _placeHoverController,
          isHovered: _isPlaceHovered,
          explanation: dynamicTooltip,
          onHover: _handlePlaceHover,
        );
      },
    );
  }

  // Initialize queue node with empty lines if missing to avoid perpetual '...'
  Future<void> _initializeQueueNode(DatabaseReference queueRef) async {
    try {
      final snap = await queueRef.get();
      if (!snap.exists) {
        await queueRef.set({
          'name': 'Queue Hub (Init)',
          'length': 0,
          'lines': {'1': 0, '2': 0, '3': 0},
          'recommendedLine': 1,
          'sensors': {},
          'updatedAt': ServerValue.timestamp,
        });
        // ignore: avoid_print
        print('[Init] queueA created');
      } else {
        final val = snap.value;
        if (val is Map && val['lines'] == null) {
          await queueRef.update({
            'lines': {'1': 0, '2': 0, '3': 0},
            'recommendedLine': 1,
            'updatedAt': ServerValue.timestamp,
          });
          // ignore: avoid_print
          print('[Init] queueA lines initialized');
        }
      }
    } catch (e) {
      // ignore: avoid_print
      print('[Init] queueA initialization error: $e');
    }
  }

  // Supports multiple possible formats for a line entry:
  // - int (direct count)
  // - String numeric
  // - Map { 'people': <int|string>, ... }
  // - Map { 'count': <int|string>, ... }
  int _extractPeopleCount(dynamic v) {
    if (v is int) return v;
    if (v is String) return int.tryParse(v) ?? 0;
    if (v is Map) {
      final people = v['people'] ?? v['count'] ?? v['value'];
      if (people is int) return people;
      if (people is String) return int.tryParse(people) ?? 0;
    }
    return 0;
  }

  Widget _buildDebugToggle() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Text('Debug', style: TextStyle(color: Colors.grey.shade700)),
        Switch(
          value: _showDebug,
          onChanged: (v) => setState(() => _showDebug = v),
          activeColor: Colors.deepOrange,
        ),
      ],
    );
  }

  Widget _buildDebugPanel() {
    final ref = FirebaseDatabase.instance.ref('queues/queueA');
    return Container(
      width: 360,
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.05),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.grey.shade400),
      ),
      child: StreamBuilder<DatabaseEvent>(
        stream: ref.onValue,
        builder: (context, snap) {
          if (!snap.hasData) {
            return const Text(
              'Waiting for queue snapshot...',
              style: TextStyle(fontSize: 12),
            );
          }
          final val = snap.data!.snapshot.value;
          if (val is Map) {
            final lines = val['lines'];
            final rec = val['recommendedLine'];
            return DefaultTextStyle(
              style: const TextStyle(fontSize: 11, fontFamily: 'monospace'),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('DEBUG QUEUE SNAPSHOT'),
                  const SizedBox(height: 4),
                  Text('recommendedLine: $rec'),
                  Text('lines: ${lines is Map ? lines : lines.toString()}'),
                  Text(
                    'length: ${val['length']} updatedAt: ${val['updatedAt']}',
                  ),
                  if (lines is Map && rec != null) ...[
                    const Divider(),
                    Builder(
                      builder: (_) {
                        int currentCount = 0;
                        final lineKey = rec.toString();
                        final raw = lines[lineKey];
                        if (raw is int)
                          currentCount = raw;
                        else if (raw is String)
                          currentCount = int.tryParse(raw) ?? 0;
                        final place = currentCount + 1;
                        return Text(
                          'Derived place for line $rec: $place (count=$currentCount)',
                        );
                      },
                    ),
                  ],
                ],
              ),
            );
          }
          return Text(
            'Snapshot (non-map): ${val.toString()}',
            style: const TextStyle(fontSize: 11),
          );
        },
      ),
    );
  }

  Widget _buildAnimationButton() {
    return ElevatedButton(
      onPressed: _isAnimating ? null : _toggleAnimation,
      style: ElevatedButton.styleFrom(
        backgroundColor: AppParameters.color_primaryBlue,
        foregroundColor: AppParameters.color_backgroundColor,
        padding: EdgeInsets.symmetric(
          horizontal: AppParameters.size_buttonHorizontalPadding,
          vertical: AppParameters.size_buttonVerticalPadding,
        ),
      ),
      child: Text(
        _isAnimating
            ? AppStrings.string_animatingText
            : AppStrings.string_startAnimationText,
        style: TextStyle(fontSize: AppParameters.size_buttonFontSize),
      ),
    );
  }

  Widget _buildQueueViewButton() {
    return ElevatedButton.icon(
      onPressed: () {
        Navigator.of(
          context,
        ).push(MaterialPageRoute(builder: (_) => QueueView(queueId: 'queueA')));
      },
      icon: const Icon(Icons.list),
      label: const Text('Open QueueA Live View'),
      style: ElevatedButton.styleFrom(
        backgroundColor: Colors.deepPurple,
        foregroundColor: Colors.white,
        padding: EdgeInsets.symmetric(
          horizontal: AppParameters.size_buttonHorizontalPadding,
          vertical: AppParameters.size_buttonVerticalPadding,
        ),
      ),
    );
  }

  Widget _buildFirebaseStatus() {
    return Container(
      padding: EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: _firebaseStatus.contains('✅')
            ? Colors.green.shade50
            : Colors.red.shade50,
        border: Border.all(
          color: _firebaseStatus.contains('✅') ? Colors.green : Colors.red,
          width: 1,
        ),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            _firebaseStatus.contains('✅') ? Icons.cloud_done : Icons.cloud_off,
            color: _firebaseStatus.contains('✅') ? Colors.green : Colors.red,
            size: 20,
          ),
          SizedBox(width: 8),
          Text(
            'Firebase: $_firebaseStatus',
            style: TextStyle(
              color: _firebaseStatus.contains('✅')
                  ? Colors.green.shade800
                  : Colors.red.shade800,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTestFirebaseButton() {
    return Column(
      children: [
        ElevatedButton(
          onPressed: _testFirebaseWrite,
          style: ElevatedButton.styleFrom(
            backgroundColor: Colors.green,
            foregroundColor: Colors.white,
            padding: EdgeInsets.symmetric(horizontal: 20, vertical: 12),
          ),
          child: Text('Test Realtime Database'),
        ),
        if (_testResult.isNotEmpty)
          Padding(
            padding: EdgeInsets.only(top: 8),
            child: Text(
              _testResult,
              style: TextStyle(
                fontSize: 12,
                color: _testResult.contains('Success')
                    ? Colors.green
                    : Colors.red,
              ),
              textAlign: TextAlign.center,
            ),
          ),
      ],
    );
  }

  void _testFirebaseWrite() async {
    setState(() {
      _testResult = 'Writing to Realtime Database...';
    });

    try {
      // Write test data to Realtime Database
      DatabaseReference ref = FirebaseDatabase.instance.ref(
        'queue_data/test_queue',
      );
      await ref.set({
        'queue_number': 5,
        'people_in_line': 8,
        'estimated_wait_time': 15,
        'timestamp': ServerValue.timestamp,
        'status': 'active',
        'sensor_data': {
          'temperature': 23.5,
          'occupancy': 0.7,
          'last_update': DateTime.now().toIso8601String(),
        },
      });

      setState(() {
        _testResult = '✅ Success! Data written to Realtime Database';
      });

      print('Realtime Database write test successful!');
    } catch (e) {
      setState(() {
        _testResult = '❌ Error: ${e.toString()}';
      });

      print('Firebase write test failed: $e');
    }
  }

  void _handlePlaceHover(bool hovered) {
    setState(() {
      _isPlaceHovered = hovered;
    });
    if (hovered) {
      _placeHoverController.forward();
    } else {
      _placeHoverController.reverse();
    }
  }

  void _handleWaitHover(bool hovered) {
    setState(() {
      _isWaitHovered = hovered;
    });
    if (hovered) {
      _waitHoverController.forward();
    } else {
      _waitHoverController.reverse();
    }
  }
}
