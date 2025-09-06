import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import '../constants/app_constants.dart';
import '../widgets/animated_clock.dart';

/// Hover box widget with tooltip and animations
class HoverBox extends StatelessWidget {
  final String icon;
  final String text;
  final String number;
  final String suffix;
  final AnimationController controller;
  final bool isHovered;
  final String explanation;
  final Function(bool) onHover;
  final bool isClockIcon;
  final AnimationController? clockController;

  const HoverBox({
    super.key,
    required this.icon,
    required this.text,
    required this.number,
    required this.suffix,
    required this.controller,
    required this.isHovered,
    required this.explanation,
    required this.onHover,
    this.isClockIcon = false,
    this.clockController,
  });

  @override
  Widget build(BuildContext context) {
    return MouseRegion(
      onEnter: (_) => onHover(true),
      onExit: (_) => onHover(false),
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          _buildMainBox(),
          _buildTooltip(),
        ],
      ),
    );
  }

  Widget _buildMainBox() {
    return Container(
      padding: const EdgeInsets.symmetric(
        horizontal: AppConstants.hoverBoxPadding,
        vertical: AppConstants.hoverBoxVerticalPadding,
      ),
      decoration: BoxDecoration(
        color: AppConstants.primaryBlue.withOpacity(0.1),
        borderRadius: BorderRadius.circular(AppConstants.hoverBoxBorderRadius),
        border: Border.all(
          color: AppConstants.primaryBlue.withOpacity(0.3),
          width: 1,
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          _buildIcon(),
          const SizedBox(width: 10),
          _buildAnimatedText(),
        ],
      ),
    );
  }

  Widget _buildIcon() {
    if (isClockIcon && clockController != null) {
      return AnimatedClock(controller: clockController!);
    }
    
    return SvgPicture.asset(
      icon,
      width: AppConstants.iconSize,
      height: AppConstants.iconSize,
      colorFilter: const ColorFilter.mode(
        AppConstants.primaryBlue,
        BlendMode.srcIn,
      ),
    );
  }

  Widget _buildAnimatedText() {
    return AnimatedBuilder(
      animation: controller,
      builder: (context, child) {
        double scale = 1.0 + (AppConstants.hoverScaleMultiplier * controller.value);
        double spacing = 2.0 * scale;
        
        return Row(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            _buildStaticText(text),
            SizedBox(width: spacing),
            _buildAnimatedNumber(scale),
            SizedBox(width: spacing),
            _buildStaticText(suffix),
          ],
        );
      },
    );
  }

  Widget _buildStaticText(String text) {
    return Text(
      text,
      style: const TextStyle(
        fontSize: 22,
        fontWeight: FontWeight.bold,
        fontFamily: AppConstants.fontFamily,
        color: AppConstants.textColor,
      ),
    );
  }

  Widget _buildAnimatedNumber(double scale) {
    return Transform.scale(
      scale: scale,
      child: Text(
        number,
        style: const TextStyle(
          fontSize: 22,
          fontWeight: FontWeight.w900,
          fontFamily: AppConstants.fontFamily,
          color: AppConstants.textColor,
        ),
      ),
    );
  }

  Widget _buildTooltip() {
    return Positioned(
      bottom: AppConstants.tooltipVerticalOffset,
      left: 0,
      right: 0,
      child: AnimatedBuilder(
        animation: controller,
        builder: (context, child) {
          return Opacity(
            opacity: controller.value,
            child: Center(
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                decoration: BoxDecoration(
                  color: AppConstants.tooltipBackgroundColor.withOpacity(0.8),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Text(
                  explanation,
                  style: const TextStyle(
                    color: AppConstants.tooltipTextColor,
                    fontSize: 12,
                    fontWeight: FontWeight.w500,
                  ),
                  textAlign: TextAlign.center,
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}
