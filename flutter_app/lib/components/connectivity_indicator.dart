import 'package:flutter/material.dart';
import '../services/connectivity_service.dart';

/// Widget that displays the current connectivity status
class ConnectivityIndicator extends StatefulWidget {
  /// Size of the connectivity icon
  final double iconSize;
  
  /// Whether to show the status text alongside the icon
  final bool showStatusText;
  
  /// Custom styling for the status text
  final TextStyle? textStyle;
  
  const ConnectivityIndicator({
    super.key,
    this.iconSize = 24.0,
    this.showStatusText = false,
    this.textStyle,
  });

  @override
  State<ConnectivityIndicator> createState() => _ConnectivityIndicatorState();
}

class _ConnectivityIndicatorState extends State<ConnectivityIndicator> {
  late ConnectivityService _connectivityService;

  @override
  void initState() {
    super.initState();
    _connectivityService = ConnectivityService();
    _connectivityService.addListener(_onConnectivityChanged);
  }

  @override
  void dispose() {
    _connectivityService.removeListener(_onConnectivityChanged);
    super.dispose();
  }

  void _onConnectivityChanged() {
    if (mounted) {
      setState(() {});
    }
  }

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        _buildConnectivityIcon(),
        if (widget.showStatusText) ...[
          const SizedBox(width: 8),
          _buildStatusText(),
        ],
      ],
    );
  }

  Widget _buildConnectivityIcon() {
    IconData iconData;
    Color iconColor;
    
    if (!_connectivityService.isConnected) {
      // No network connection
      iconData = Icons.wifi_off;
      iconColor = Colors.red;
    } else if (!_connectivityService.hasInternet) {
      // Connected to network but no internet
      iconData = Icons.wifi;
      iconColor = Colors.red;
    } else {
      // Connected with internet
      iconData = Icons.wifi;
      iconColor = Colors.green;
    }

    return AnimatedSwitcher(
      duration: const Duration(milliseconds: 300),
      child: Icon(
        iconData,
        key: ValueKey('$iconData$iconColor'),
        size: widget.iconSize,
        color: iconColor,
      ),
    );
  }

  Widget _buildStatusText() {
    final defaultStyle = TextStyle(
      fontSize: 12,
      color: _connectivityService.hasInternet ? Colors.green : Colors.red,
    );
    
    return AnimatedSwitcher(
      duration: const Duration(milliseconds: 300),
      child: Text(
        _connectivityService.connectionStatus,
        key: ValueKey(_connectivityService.connectionStatus),
        style: widget.textStyle ?? defaultStyle,
      ),
    );
  }
}

/// A floating connectivity indicator that can be positioned anywhere on screen
class FloatingConnectivityIndicator extends StatelessWidget {
  /// Position from the top of the screen
  final double? top;
  
  /// Position from the bottom of the screen
  final double? bottom;
  
  /// Position from the left of the screen
  final double? left;
  
  /// Position from the right of the screen
  final double? right;
  
  /// Background color of the indicator
  final Color? backgroundColor;
  
  /// Border radius of the indicator
  final double borderRadius;
  
  /// Padding inside the indicator
  final EdgeInsets padding;
  
  /// Size of the WiFi icon
  final double iconSize;
  
  /// Whether to show status text
  final bool showStatusText;

  const FloatingConnectivityIndicator({
    super.key,
    this.top,
    this.bottom,
    this.left,
    this.right,
    this.backgroundColor,
    this.borderRadius = 20.0,
    this.padding = const EdgeInsets.all(8.0),
    this.iconSize = 20.0,
    this.showStatusText = false,
  });

  @override
  Widget build(BuildContext context) {
    return Positioned(
      top: top,
      bottom: bottom,
      left: left,
      right: right,
      child: Container(
        padding: padding,
        decoration: BoxDecoration(
          color: backgroundColor ?? Colors.black.withOpacity(0.7),
          borderRadius: BorderRadius.circular(borderRadius),
        ),
        child: ConnectivityIndicator(
          iconSize: iconSize,
          showStatusText: showStatusText,
          textStyle: const TextStyle(
            color: Colors.white,
            fontSize: 12,
          ),
        ),
      ),
    );
  }
}