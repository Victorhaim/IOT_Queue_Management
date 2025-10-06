/// Dart adaptation of the C++ QueueManager for simulation and UI decisions.
/// Chooses the line with the fewest people (ties -> lowest line number).
class QueueManagerDart {
  QueueManagerDart({required int maxSize, required int numberOfLines})
      : _maxSize = maxSize,
        _numberOfLines = numberOfLines {
    for (var i = 1; i <= _numberOfLines; i++) {
      _lines[i] = 0; // peopleCount
    }
  }

  final int _maxSize; // 0 means unlimited
  final int _numberOfLines;
  int _totalPeople = 0;
  final Map<int, int> _lines = {}; // lineNumber -> peopleCount

  bool get isEmpty => _totalPeople == 0;
  bool get isFull => _maxSize != 0 && _totalPeople >= _maxSize;
  int get totalPeople => _totalPeople;
  int get numberOfLines => _numberOfLines;

  int getLineCount(int lineNumber) => _lines[lineNumber] ?? -1;

  bool enqueue() {
    if (isFull) return false;
    final ln = getNextLineNumber();
    if (ln == -1) return false;
    _lines[ln] = _lines[ln]! + 1;
    _totalPeople++;
    return true;
  }

  bool enqueueOnLine(int lineNumber) {
    if (isFull) return false;
    if (!_lines.containsKey(lineNumber)) return false;
    _lines[lineNumber] = _lines[lineNumber]! + 1;
    _totalPeople++;
    return true;
  }

  bool dequeue(int lineNumber) {
    if (!_lines.containsKey(lineNumber)) return false;
    if (_lines[lineNumber]! == 0) return false;
    _lines[lineNumber] = _lines[lineNumber]! - 1;
    _totalPeople--;
    return true;
  }

  void setLineCount(int lineNumber, int count) {
    if (!_lines.containsKey(lineNumber)) return;
    _totalPeople -= _lines[lineNumber]!;
    _lines[lineNumber] = count < 0 ? 0 : count;
    _totalPeople += _lines[lineNumber]!;
  }

  void reset() {
    _lines.updateAll((key, value) => 0);
    _totalPeople = 0;
  }

  int getNextLineNumber() {
    if (_lines.isEmpty) return -1;
    int bestLine = -1;
    int bestCount = 1 << 30;
    _lines.forEach((line, count) {
      if (count < bestCount || (count == bestCount && line < bestLine)) {
        bestLine = line;
        bestCount = count;
      }
    });
    return bestLine;
  }

  Map<String, dynamic> toJson() => {
        'lines': _lines.map((k, v) => MapEntry(k.toString(), v)),
        'totalPeople': _totalPeople,
        'recommendedLine': getNextLineNumber(),
      };
}
