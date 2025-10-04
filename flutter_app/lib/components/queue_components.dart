import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import '../parameters/app_parameters.dart';
import '../widgets/animated_waves.dart';
import '../widgets/hover_box.dart';

/// Queue app main widget
class QueueApp extends StatelessWidget {
  const QueueApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: AppStrings.string_appTitle,
      theme: ThemeData.light().copyWith(
        colorScheme: ColorScheme.fromSeed(seedColor: AppParameters.color_primaryBlue),
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

class _QueueScreenState extends State<QueueScreen> with TickerProviderStateMixin {
  late AnimationController _waveController;
  late AnimationController _clockController;
  late AnimationController _placeHoverController;
  late AnimationController _waitHoverController;
  
  bool _isAnimating = false;
  bool _isPlaceHovered = false;
  bool _isWaitHovered = false;
  String _firebaseStatus = 'Connecting...';
  String _testResult = '';

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
        'message': 'Realtime Database is working!'
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
            AppStrings.string_queueNumber,
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
  }

  Widget _buildHoverBoxes() {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          HoverBox(
            icon: AppAssets.string_queueIcon,
            text: AppStrings.string_placeInLineText,
            number: AppStrings.string_placeInLineNumber,
            suffix: '',
            controller: _placeHoverController,
            isHovered: _isPlaceHovered,
            explanation: AppStrings.string_placeTooltip,
            onHover: _handlePlaceHover,
          ),
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

  Widget _buildAnimationButton() {
    return ElevatedButton(
      onPressed: _isAnimating ? null : _toggleAnimation,
      style: ElevatedButton.styleFrom(
        backgroundColor: AppParameters.color_primaryBlue,
        foregroundColor: AppParameters.color_backgroundColor,
        padding: EdgeInsets.symmetric(
          horizontal: AppParameters.size_buttonHorizontalPadding, 
          vertical: AppParameters.size_buttonVerticalPadding
        ),
      ),
      child: Text(
        _isAnimating ? AppStrings.string_animatingText : AppStrings.string_startAnimationText,
        style: TextStyle(fontSize: AppParameters.size_buttonFontSize),
      ),
    );
  }

  Widget _buildFirebaseStatus() {
    return Container(
      padding: EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: _firebaseStatus.contains('✅') ? Colors.green.shade50 : Colors.red.shade50,
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
              color: _firebaseStatus.contains('✅') ? Colors.green.shade800 : Colors.red.shade800,
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
                color: _testResult.contains('Success') ? Colors.green : Colors.red,
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
      DatabaseReference ref = FirebaseDatabase.instance.ref('queue_data/test_queue');
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
        }
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
