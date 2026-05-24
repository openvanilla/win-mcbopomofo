# GUI

This document describes the main graphical user interface components of Win-McBopomofo on Windows, along with their design requirements and implementation principles.

## UI Components

The IME UI mainly consists of three parts:

- Candidate Window
- Tooltip Window
- Language Bar Icons

These three components are responsible for candidate presentation, auxiliary user hints, and IME mode and feature entry points.

## Shared Requirements

All IME UI components should meet the following requirements:

- They must render at the correct scale across different screen resolutions and DPI settings, without appearing blurry, too small, or too large.
- They must be able to render color emoji correctly, so the text rendering API must support color fonts and color emoji.
- Window sizes must be calculated from actual content so that text, candidate items, and hints are not clipped.
- The candidate window and tooltip window should use the same drawing API and similar rendering flow whenever practical, to reduce maintenance cost and behavioral drift.
- They must be able to appear correctly in a wide range of Windows applications, including traditional Win32 applications, modern applications, and different text input hosting environments.

## Candidate Window

The candidate window is the primary custom UI of the IME. It displays candidates, the current selection, and any necessary supporting information.

### Requirements

- It must support configurable font size.
- Its background color and text color must follow the system theme setting, including dark mode and light mode.
- Its highlight color must be derived from the system theme.
- Its overall appearance should feel integrated with the Windows system UI. Personalization should be handled through system theme and system color settings, rather than through additional IME-specific appearance settings.
- It must not provide font family customization.

### Design Principles

- Visually, it should feel like a natural extension of system UI rather than a separate themed surface.
- The adjustable option is readability-related font size, not overall typographic styling.
- It should maintain clear contrast and legibility across different system themes and accent color combinations.

## Tooltip Window

The tooltip window is used to present short, direct hints to the user in specific application scenarios.

### Requirements

- It should use a simple pale yellow background with black text.
- It must not provide font family or font size customization.
- It is intended to provide user hints in specific application contexts.

### Design Principles

- The tooltip is a supporting hint surface, not the primary interaction surface, so its presentation should remain simple, stable, and easy to read.
- Although its visual design is more fixed than the candidate window, it should still share the same window management and drawing foundation whenever practical.

## Language Bar

The Language Bar icons are the IME's integration points in the Windows language bar, providing mode switching and feature entry points.

### Requirements

- It must provide a Chinese/English mode switch button.
- It must provide an advanced features menu.

### Design Principles

- The Language Bar should expose the most essential and frequently used switching actions.
- Advanced features should be grouped under a menu entry rather than scattered across multiple UI surfaces.
- This layer should prioritize system integration and behave in a way that matches Windows users' expectations of the language bar.

## Summary

The core GUI goals of this IME are:

- Stable presentation across a wide range of Windows host applications.
- Correct scaling on high-DPI and mixed-resolution displays.
- Use of a rendering API that can correctly display color emoji.
- A shared technical foundation between the candidate window and tooltip window wherever practical.
- Visual behavior that primarily follows Windows system theme and personalization, rather than introducing extensive IME-specific appearance customization.
