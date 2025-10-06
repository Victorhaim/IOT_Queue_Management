@JS()
library queue_manager_wasm;

import 'dart:async';
import 'dart:js' as js;
import 'dart:js_util';
import 'dart:html' as html;

import 'package:js/js.dart';

/// JavaScript interop declarations for the WebAssembly QueueManager module
@JS('QueueManagerModule')
external js.JsObject get QueueManagerModule;

/// WebAssembly bindings for the shared C++ QueueManager
/// This allows Flutter Web to use the same queue logic as the ESP32 and native platforms
class QueueManagerWASM {
  static js.JsObject? _module;
  static bool _initialized = false;

  // JavaScript function wrappers for WASM
  static js.JsFunction? _cwrap;

  // Wrapped C functions
  late final js.JsFunction _createQueueManager;
  late final js.JsFunction _destroyQueueManager;
  late final js.JsFunction _enqueue;
  late final js.JsFunction _dequeue;
  late final js.JsFunction _enqueueOnLine;
  late final js.JsFunction _size;
  late final js.JsFunction _isEmpty;
  late final js.JsFunction _isFull;
  late final js.JsFunction _getNextLineNumber;
  late final js.JsFunction _getNumberOfLines;
  late final js.JsFunction _getLineCount;
  late final js.JsFunction _setLineCount;
  late final js.JsFunction _reset;
  late final js.JsFunction _getLineCountsArray;
  late final js.JsFunction _updateFromArray;
  late final js.JsFunction _malloc;
  late final js.JsFunction _free;

  /// Pointer to the C++ QueueManager instance in WASM memory
  late final int _queueManagerPtr;

  /// Initialize the WebAssembly module (call once before creating instances)
  static Future<void> initialize() async {
    if (_initialized) return;

    try {
      // Load and initialize the WebAssembly module
      await _loadWasmModule();
      _initialized = true;
    } catch (e) {
      throw Exception('Failed to initialize QueueManager WASM module: $e');
    }
  }

  /// Load the WebAssembly module from the web assets
  static Future<void> _loadWasmModule() async {
    // Load the JavaScript wrapper for the WASM module
    final script = html.ScriptElement()
      ..src = '/wasm/queue_manager.js'
      ..type = 'text/javascript';

    final completer = Completer<void>();

    script.onLoad.listen((_) async {
      try {
        // Initialize the WASM module
        _module = await promiseToFuture(
          callMethod(js.context, 'QueueManagerModule', []),
        );

        // Get the cwrap function
        _cwrap = _module!['cwrap'];

        completer.complete();
      } catch (e) {
        completer.completeError(e);
      }
    });

    script.onError.listen((e) {
      completer.completeError('Failed to load WASM module: $e');
    });

    html.document.head!.append(script);
    return completer.future;
  }

  /// Creates a new QueueManager instance
  QueueManagerWASM(int maxSize, int numberOfLines) {
    if (!_initialized || _module == null) {
      throw StateError(
        'QueueManagerWASM must be initialized before use. '
        'Call QueueManagerWASM.initialize() first.',
      );
    }

    // Wrap all the C functions using cwrap
    _createQueueManager = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_create',
      'number',
      ['number', 'number'],
    ]);

    _destroyQueueManager = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_destroy',
      null,
      ['number'],
    ]);

    _enqueue = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_enqueue',
      'number',
      ['number'],
    ]);

    _dequeue = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_dequeue',
      'number',
      ['number', 'number'],
    ]);

    _enqueueOnLine = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_enqueue_on_line',
      'number',
      ['number', 'number'],
    ]);

    _size = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_size',
      'number',
      ['number'],
    ]);

    _isEmpty = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_is_empty',
      'number',
      ['number'],
    ]);

    _isFull = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_is_full',
      'number',
      ['number'],
    ]);

    _getNextLineNumber = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_get_next_line_number',
      'number',
      ['number'],
    ]);

    _getNumberOfLines = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_get_number_of_lines',
      'number',
      ['number'],
    ]);

    _getLineCount = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_get_line_count',
      'number',
      ['number', 'number'],
    ]);

    _setLineCount = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_set_line_count',
      null,
      ['number', 'number', 'number'],
    ]);

    _reset = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_reset',
      null,
      ['number'],
    ]);

    _getLineCountsArray = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_get_line_counts_array',
      'number',
      ['number'],
    ]);

    _updateFromArray = callMethod(_cwrap!, 'call', [
      _module,
      'queue_manager_update_from_array',
      null,
      ['number', 'number', 'number'],
    ]);

    _malloc = callMethod(_cwrap!, 'call', [
      _module,
      'malloc',
      'number',
      ['number'],
    ]);

    _free = callMethod(_cwrap!, 'call', [
      _module,
      'free',
      null,
      ['number'],
    ]);

    // Create the C++ QueueManager instance
    _queueManagerPtr = callMethod(_createQueueManager, 'call', [
      null,
      maxSize,
      numberOfLines,
    ]);

    if (_queueManagerPtr == 0) {
      throw Exception('Failed to create QueueManager instance in WASM');
    }
  }

  /// Destroys the QueueManager instance
  void dispose() {
    if (_queueManagerPtr != 0) {
      callMethod(_destroyQueueManager, 'call', [null, _queueManagerPtr]);
    }
  }

  /// Add a person to the queue (auto-selects best line)
  bool enqueue() {
    final result = callMethod(_enqueue, 'call', [null, _queueManagerPtr]);
    return result != 0;
  }

  /// Remove a person from a specific line
  bool dequeue(int lineNumber) {
    final result = callMethod(_dequeue, 'call', [
      null,
      _queueManagerPtr,
      lineNumber,
    ]);
    return result != 0;
  }

  /// Add a person to a specific line
  bool enqueueOnLine(int lineNumber) {
    final result = callMethod(_enqueueOnLine, 'call', [
      null,
      _queueManagerPtr,
      lineNumber,
    ]);
    return result != 0;
  }

  /// Get total number of people in queue
  int get size {
    return callMethod(_size, 'call', [null, _queueManagerPtr]);
  }

  /// Check if queue is empty
  bool get isEmpty {
    final result = callMethod(_isEmpty, 'call', [null, _queueManagerPtr]);
    return result != 0;
  }

  /// Check if queue is full
  bool get isFull {
    final result = callMethod(_isFull, 'call', [null, _queueManagerPtr]);
    return result != 0;
  }

  /// Get the recommended line number (with fewest people)
  int getNextLineNumber() {
    return callMethod(_getNextLineNumber, 'call', [null, _queueManagerPtr]);
  }

  /// Get number of lines in this queue
  int get numberOfLines {
    return callMethod(_getNumberOfLines, 'call', [null, _queueManagerPtr]);
  }

  /// Get number of people in a specific line
  int getLineCount(int lineNumber) {
    return callMethod(_getLineCount, 'call', [
      null,
      _queueManagerPtr,
      lineNumber,
    ]);
  }

  /// Set number of people in a specific line
  void setLineCount(int lineNumber, int count) {
    callMethod(_setLineCount, 'call', [
      null,
      _queueManagerPtr,
      lineNumber,
      count,
    ]);
  }

  /// Reset all lines to zero people
  void reset() {
    callMethod(_reset, 'call', [null, _queueManagerPtr]);
  }

  /// Get all line counts as a List
  List<int> getLineCountsArray() {
    final arrayPtr = callMethod(_getLineCountsArray, 'call', [
      null,
      _queueManagerPtr,
    ]);
    final result = <int>[];

    // Read the array from WASM memory using the module's HEAP
    final heap = _module!['HEAP32'];
    final startIndex = arrayPtr >> 2; // Convert byte offset to int32 offset

    for (int i = 0; i < numberOfLines; i++) {
      result.add(heap[startIndex + i]);
    }

    return result;
  }

  /// Update line counts from a List
  void updateFromArray(List<int> lineCounts) {
    // Allocate memory in WASM for the array
    final arrayPtr = callMethod(_malloc, 'call', [
      null,
      lineCounts.length * 4,
    ]); // 4 bytes per int32

    try {
      // Write the array to WASM memory using the module's HEAP
      final heap = _module!['HEAP32'];
      final startIndex = arrayPtr >> 2; // Convert byte offset to int32 offset

      for (int i = 0; i < lineCounts.length; i++) {
        heap[startIndex + i] = lineCounts[i];
      }

      // Call the update function
      callMethod(_updateFromArray, 'call', [
        null,
        _queueManagerPtr,
        arrayPtr,
        lineCounts.length,
      ]);
    } finally {
      // Free the allocated memory
      callMethod(_free, 'call', [null, arrayPtr]);
    }
  }

  /// Convert to JSON-like Map for compatibility with existing code
  Map<String, dynamic> toJson() {
    final lines = <String, int>{};
    for (int i = 1; i <= numberOfLines; i++) {
      lines[i.toString()] = getLineCount(i);
    }

    return {
      'lines': lines,
      'totalPeople': size,
      'recommendedLine': getNextLineNumber(),
    };
  }
}
