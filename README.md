# This project was discontined
The author has switched his primary code editor to VS Code.

# Original Intro Content
**A color picker plugin for Notepad++**
The C++ Color Picker class can also be reused in other programs.

[[Download from Amazon Mirror]](https://s3-ap-southeast-1.amazonaws.com/nppqcp/nppqcp-2.0.zip)


v2.0
* Fixed crash problem cause by Scintilla RegExp search interface
* Use self-drawn underline marker to avoid conflict with other plugins & features
* Added Color Picker & Screen Picker commands for hotkey assignment
* Added HSL & HSLA color format support
* Added transparent color support (with adjustment)
* Use SPACE key toggle mouse speed in Screen Picker for better aiming
* Slightly enlarged UI elements

[View Full Changelog](https://github.com/nulled666/nppqcp/blob/wiki/Changelog.md)


![Screenshot](https://s3-ap-southeast-1.amazonaws.com/nppqcp/features-2.0.png)

**FEATURES**

* HEX/RGB/RGBA/HSL/HSLA color code highlight
* Double-click activation of color picker
  * double-click hex code portion "ffcc00" of hex string
  * double-click "rgb"/"rgba"/"hsl"/"hsla" header of bracket colors
* Allow assign hotkeys for Color Picker and Screen Picker
* Professional color palette
  * same palette layout as in most popular design software
  * recent color swatches
  * mark current color on palette
  * compare new & old colors
* Quick HSLA color tuning
  * click to fine-tune the Hue, Saturation, Lightness or Transparency of your color
  * right-click on palette color to put the color into tuning swatch
* Screen color picker
  * hide notepad++ window while picking color
  * zoomed aim, press SPACE to toggle precise aiming mode
  * real-time color code display
*  Access Windows Color Chooser
 * recent colors will appear in custom colors list

This project is using https://github.com/kkaefer/css-color-parser-cpp for css color parse.
