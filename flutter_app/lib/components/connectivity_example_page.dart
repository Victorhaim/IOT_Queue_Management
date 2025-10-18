import 'package:flutter/material.dart';
import '../components/connectivity_indicator.dart';

/// Example page showing different ways to use the ConnectivityIndicator
class ConnectivityExamplePage extends StatelessWidget {
  const ConnectivityExamplePage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Connectivity Indicator Examples'),
        // Example: In AppBar
        actions: const [
          Padding(
            padding: EdgeInsets.only(right: 16.0),
            child: ConnectivityIndicator(
              iconSize: 24,
              showStatusText: false,
            ),
          ),
        ],
      ),
      body: Stack(
        children: [
          const Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  'Different Connectivity Indicator Examples',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
                SizedBox(height: 40),
                
                // Example: Basic indicator
                Text('Basic Indicator:'),
                SizedBox(height: 10),
                ConnectivityIndicator(),
                
                SizedBox(height: 30),
                
                // Example: With status text
                Text('With Status Text:'),
                SizedBox(height: 10),
                ConnectivityIndicator(
                  showStatusText: true,
                  iconSize: 28,
                ),
                
                SizedBox(height: 30),
                
                // Example: Custom styling
                Text('Custom Styling:'),
                SizedBox(height: 10),
                ConnectivityIndicator(
                  showStatusText: true,
                  iconSize: 32,
                  textStyle: TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ],
            ),
          ),
          
          // Example: Floating indicator in top-right corner
          const FloatingConnectivityIndicator(
            top: 100,
            right: 20,
            showStatusText: true,
          ),
          
          // Example: Floating indicator in bottom-left corner
          const FloatingConnectivityIndicator(
            bottom: 20,
            left: 20,
            backgroundColor: Colors.blue,
            showStatusText: false,
          ),
        ],
      ),
    );
  }
}