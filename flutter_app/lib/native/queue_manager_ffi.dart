import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

/// FFI bindings for the shared C++ QueueManager
/// This allows Flutter to use the same queue logic as the ESP32
class QueueManagerFFI {
  static const String _libName = 'queue_manager_shared';
  
  /// The dynamic library
  static final DynamicLibrary _dylib = _openDylib();

  /// Opens the dynamic library based on the platform
  static DynamicLibrary _openDylib() {
    if (Platform.isMacOS || Platform.isIOS) {
      return DynamicLibrary.open('../../shared/cpp/build/lib$_libName.dylib');
    }
    if (Platform.isAndroid || Platform.isLinux) {
      return DynamicLibrary.open('../../shared/cpp/build/lib$_libName.so');
    }
    if (Platform.isWindows) {
      return DynamicLibrary.open('../../shared/cpp/build/lib$_libName.dll');
    }
    throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
  }

  // C function signatures
  static final Pointer<NativeType> Function(int, int) _createQueueManager = _dylib
      .lookup<NativeFunction<Pointer<NativeType> Function(Int32, Int32)>>('queue_manager_create')
      .asFunction();

  static final void Function(Pointer<NativeType>) _destroyQueueManager = _dylib
      .lookup<NativeFunction<Void Function(Pointer<NativeType>)>>('queue_manager_destroy')
      .asFunction();

  static final bool Function(Pointer<NativeType>) _enqueue = _dylib
      .lookup<NativeFunction<Bool Function(Pointer<NativeType>)>>('queue_manager_enqueue')
      .asFunction();

  static final bool Function(Pointer<NativeType>, int) _dequeue = _dylib
      .lookup<NativeFunction<Bool Function(Pointer<NativeType>, Int32)>>('queue_manager_dequeue')
      .asFunction();

  static final bool Function(Pointer<NativeType>, int) _enqueueOnLine = _dylib
      .lookup<NativeFunction<Bool Function(Pointer<NativeType>, Int32)>>('queue_manager_enqueue_on_line')
      .asFunction();

  static final int Function(Pointer<NativeType>) _size = _dylib
      .lookup<NativeFunction<Int32 Function(Pointer<NativeType>)>>('queue_manager_size')
      .asFunction();

  static final bool Function(Pointer<NativeType>) _isEmpty = _dylib
      .lookup<NativeFunction<Bool Function(Pointer<NativeType>)>>('queue_manager_is_empty')
      .asFunction();

  static final bool Function(Pointer<NativeType>) _isFull = _dylib
      .lookup<NativeFunction<Bool Function(Pointer<NativeType>)>>('queue_manager_is_full')
      .asFunction();

  static final int Function(Pointer<NativeType>) _getNextLineNumber = _dylib
      .lookup<NativeFunction<Int32 Function(Pointer<NativeType>)>>('queue_manager_get_next_line_number')
      .asFunction();

  static final int Function(Pointer<NativeType>) _getNumberOfLines = _dylib
      .lookup<NativeFunction<Int32 Function(Pointer<NativeType>)>>('queue_manager_get_number_of_lines')
      .asFunction();

  static final int Function(Pointer<NativeType>, int) _getLineCount = _dylib
      .lookup<NativeFunction<Int32 Function(Pointer<NativeType>, Int32)>>('queue_manager_get_line_count')
      .asFunction();

  static final void Function(Pointer<NativeType>, int, int) _setLineCount = _dylib
      .lookup<NativeFunction<Void Function(Pointer<NativeType>, Int32, Int32)>>('queue_manager_set_line_count')
      .asFunction();

  static final void Function(Pointer<NativeType>) _reset = _dylib
      .lookup<NativeFunction<Void Function(Pointer<NativeType>)>>('queue_manager_reset')
      .asFunction();

  static final Pointer<Int32> Function(Pointer<NativeType>) _getLineCountsArray = _dylib
      .lookup<NativeFunction<Pointer<Int32> Function(Pointer<NativeType>)>>('queue_manager_get_line_counts_array')
      .asFunction();

  static final void Function(Pointer<NativeType>, Pointer<Int32>, int) _updateFromArray = _dylib
      .lookup<NativeFunction<Void Function(Pointer<NativeType>, Pointer<Int32>, Int32)>>('queue_manager_update_from_array')
      .asFunction();

  /// Pointer to the C++ QueueManager instance
  final Pointer<NativeType> _queueManagerPtr;

  /// Creates a new QueueManager instance
  QueueManagerFFI(int maxSize, int numberOfLines) 
      : _queueManagerPtr = _createQueueManager(maxSize, numberOfLines);

  /// Destroys the QueueManager instance
  void dispose() {
    _destroyQueueManager(_queueManagerPtr);
  }

  /// Add a person to the queue (auto-selects best line)
  bool enqueue() => _enqueue(_queueManagerPtr);

  /// Remove a person from a specific line
  bool dequeue(int lineNumber) => _dequeue(_queueManagerPtr, lineNumber);

  /// Add a person to a specific line
  bool enqueueOnLine(int lineNumber) => _enqueueOnLine(_queueManagerPtr, lineNumber);

  /// Get total number of people in queue
  int get size => _size(_queueManagerPtr);

  /// Check if queue is empty
  bool get isEmpty => _isEmpty(_queueManagerPtr);

  /// Check if queue is full
  bool get isFull => _isFull(_queueManagerPtr);

  /// Get the recommended line number (with fewest people)
  int getNextLineNumber() => _getNextLineNumber(_queueManagerPtr);

  /// Get number of lines in this queue
  int get numberOfLines => _getNumberOfLines(_queueManagerPtr);

  /// Get number of people in a specific line
  int getLineCount(int lineNumber) => _getLineCount(_queueManagerPtr, lineNumber);

  /// Set number of people in a specific line
  void setLineCount(int lineNumber, int count) => _setLineCount(_queueManagerPtr, lineNumber, count);

  /// Reset all lines to zero people
  void reset() => _reset(_queueManagerPtr);

  /// Get all line counts as a List
  List<int> getLineCountsArray() {
    final arrayPtr = _getLineCountsArray(_queueManagerPtr);
    final result = <int>[];
    for (int i = 0; i < numberOfLines; i++) {
      result.add(arrayPtr[i]);
    }
    return result;
  }

  /// Update line counts from a List
  void updateFromArray(List<int> lineCounts) {
    final arrayPtr = malloc<Int32>(lineCounts.length);
    for (int i = 0; i < lineCounts.length; i++) {
      arrayPtr[i] = lineCounts[i];
    }
    _updateFromArray(_queueManagerPtr, arrayPtr, lineCounts.length);
    malloc.free(arrayPtr);
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