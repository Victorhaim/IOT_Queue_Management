import 'dart:async';
import 'dart:io';
import 'package:connectivity_plus/connectivity_plus.dart';
import 'package:flutter/foundation.dart';

/// Service to monitor network connectivity and internet access
class ConnectivityService extends ChangeNotifier {
  static final ConnectivityService _instance = ConnectivityService._internal();
  factory ConnectivityService() => _instance;
  ConnectivityService._internal();

  final Connectivity _connectivity = Connectivity();
  late StreamSubscription<List<ConnectivityResult>> _connectivitySubscription;
  
  bool _hasInternet = true;
  bool _isConnected = true;
  
  /// Returns true if device has internet access
  bool get hasInternet => _hasInternet;
  
  /// Returns true if device is connected to any network
  bool get isConnected => _isConnected;
  
  /// Returns the connection status as a user-friendly string
  String get connectionStatus {
    if (!_isConnected) return 'No Connection';
    if (!_hasInternet) return 'No Internet';
    return 'Connected';
  }
  
  /// Initialize the connectivity service
  Future<void> initialize() async {
    // Check initial connectivity
    await _updateConnectionStatus();
    
    // Listen for connectivity changes
    _connectivitySubscription = _connectivity.onConnectivityChanged.listen(
      (List<ConnectivityResult> results) async {
        await _updateConnectionStatus();
      },
    );
  }
  
  /// Update the connection status
  Future<void> _updateConnectionStatus() async {
    try {
      final List<ConnectivityResult> connectivityResults = 
          await _connectivity.checkConnectivity();
      
      // Check if connected to any network
      _isConnected = connectivityResults.isNotEmpty && 
          !connectivityResults.contains(ConnectivityResult.none);
      
      if (_isConnected) {
        // If connected to network, check for actual internet access
        _hasInternet = await _hasInternetAccess();
      } else {
        _hasInternet = false;
      }
      
      notifyListeners();
    } catch (e) {
      // If there's an error, assume no connectivity
      _isConnected = false;
      _hasInternet = false;
      notifyListeners();
    }
  }
  
  /// Check if there's actual internet access by trying to reach a reliable host
  Future<bool> _hasInternetAccess() async {
    try {
      if (kIsWeb) {
        // For web, we assume if connected to network, internet is available
        // Web apps run in browsers that handle network connectivity
        return true;
      }
      
      // For mobile/desktop, try to connect to a reliable host
      final result = await InternetAddress.lookup('google.com')
          .timeout(const Duration(seconds: 5));
      
      return result.isNotEmpty && result[0].rawAddress.isNotEmpty;
    } catch (e) {
      return false;
    }
  }
  
  /// Manually refresh the connection status
  Future<void> refreshConnectionStatus() async {
    await _updateConnectionStatus();
  }
  
  /// Dispose of the service
  @override
  void dispose() {
    _connectivitySubscription.cancel();
    super.dispose();
  }
}