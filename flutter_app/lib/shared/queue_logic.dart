/// Shared queue logic utilities that can be used by both simulation and real ESP32 integration.
/// Contains algorithms for queue management decisions.
class QueueLogic {
  /// Selects the best line number based on queue management strategy.
  /// 
  /// Strategy: Choose the line with the fewest people.
  /// Tie-breaker: If multiple lines have the same count, choose the lowest line number.
  /// 
  /// Parameters:
  /// - [lineCounts]: Map of line number to people count
  /// 
  /// Returns:
  /// - The line number that should be selected, or -1 if no lines available
  static int getNextLineNumber(Map<int, int> lineCounts) {
    if (lineCounts.isEmpty) return -1;
    
    int bestLine = -1;
    int bestCount = 1 << 30; // Large number (max int / 2)
    
    lineCounts.forEach((line, count) {
      if (count < bestCount || (count == bestCount && line < bestLine)) {
        bestLine = line;
        bestCount = count;
      }
    });
    
    return bestLine;
  }

  /// Alternative implementation that returns the line with minimum count
  /// and handles edge cases more explicitly.
  static int getRecommendedLine(Map<int, int> lineCounts) {
    if (lineCounts.isEmpty) return -1;
    
    // Find minimum count
    int minCount = lineCounts.values.reduce((a, b) => a < b ? a : b);
    
    // Find all lines with minimum count
    List<int> linesWithMinCount = lineCounts.entries
        .where((entry) => entry.value == minCount)
        .map((entry) => entry.key)
        .toList();
    
    // Return the lowest line number among those with minimum count
    return linesWithMinCount.reduce((a, b) => a < b ? a : b);
  }
}