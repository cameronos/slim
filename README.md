# SLIM (simple lightweight image manager)
A minimal and fast image viewer for X11 using Imlib2. Written to the tune of <a href="https://www.cis.upenn.edu/~lee/06cse480/data/cstyle.ms.pdf">C Style and Coding Standards for SunOS</a>.

SLIM displays images in X11 windows with minimal overhead and works in most Linux/BSD distros. Supports JPEG, PNG, SVG, and any other format handled by Imlib2.
Zooming and panning are quick, backbuffered to avoid stutter, and flicker-free.

### Languages and styles used
<p>
    <a href="https://www.cprogramming.com/"><img height="28" width="28" src="https://upload.wikimedia.org/wikipedia/commons/1/19/C_Logo.png" /></a>
</p>

## Screenshots
<img width=60% src="https://raw.githubusercontent.com/cameronos/slim/main/slim_screenshot.png">

## Features
- Minimal X11 image viewer
- Zoom in/out with mouse wheel or +/- keys
- Pan images by left-mouse dragging
- Reset view with `R` key
- Image resizing when window is smaller

## Usage
To run SLIM:

```bash
slim <image-path>
