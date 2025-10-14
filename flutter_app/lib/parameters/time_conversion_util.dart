/// Utility class for converting time values between units
class TimeConversionUtil {
  /// Converts seconds to the most appropriate time unit and returns both the converted value and unit
  ///
  /// Rules:
  /// - If seconds is 0: display "0 min"
  /// - If seconds > 0 but < 60: display "1 min" (minimum)
  /// - If less than 3600 seconds (60 minutes): display in minutes (rounded)
  /// - Otherwise: display in hours
  ///
  /// Returns a [TimeConversionResult] with the converted value and appropriate suffix
  static TimeConversionResult convertSecondsToAppropriateUnit(double seconds) {
    if (seconds == 0) {
      // Only show 0 min when exactly 0 seconds
      return TimeConversionResult(
        value: 0,
        suffix: ' min',
        unit: TimeUnit.minutes,
      );
    } else if (seconds < 60) {
      // Show at least 1 min for any non-zero value under 60 seconds
      return TimeConversionResult(
        value: 1,
        suffix: ' min',
        unit: TimeUnit.minutes,
      );
    } else if (seconds < 3600) {
      // Display in minutes
      double minutes = seconds / 60;
      return TimeConversionResult(
        value: minutes.round(),
        suffix: ' min',
        unit: TimeUnit.minutes,
      );
    } else {
      // Display in hours
      double hours = seconds / 3600;
      return TimeConversionResult(
        value: hours.round(),
        suffix: ' hr',
        unit: TimeUnit.hours,
      );
    }
  }

  /// Simple helper to get separate number and suffix for UI components
  /// Returns a map with 'value' and 'suffix' keys
  static Map<String, String> getFormattedTime(double seconds) {
    final result = convertSecondsToAppropriateUnit(seconds);
    return {'value': result.valueString, 'suffix': result.suffix};
  }
}

/// Represents the result of a time conversion
class TimeConversionResult {
  final num value;
  final String suffix;
  final TimeUnit unit;

  const TimeConversionResult({
    required this.value,
    required this.suffix,
    required this.unit,
  });

  /// Returns the formatted display string (value + suffix)
  String get displayString => '${value.toString()}$suffix';

  /// Returns just the numeric value as a string
  String get valueString => value.toString();

  @override
  String toString() => displayString;
}

/// Enum representing different time units
enum TimeUnit { seconds, minutes, hours }
