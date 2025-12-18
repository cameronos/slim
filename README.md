# SLIM (simple lightweight image manager)
A minimal and fast image viewer for X11 using Imlib2. Written to the tune of <a href="https://www.cis.upenn.edu/~lee/06cse480/data/cstyle.ms.pdf">C Style and Coding Standards for SunOS</a>.

SLIM displays images in X11 windows with minimal overhead and works in most Linux/BSD distros. Supports JPEG, PNG, SVG, and any other format handled by Imlib2.
Zooming and panning are quick, backbuffered to avoid stutter, and flicker-free.

### Languages and styles used
<p>
    <a href="https://www.cprogramming.com/"><img height="28" width="28" src="https://upload.wikimedia.org/wikipedia/commons/1/19/C_Logo.png" /></a>
</p>

## Screenshots
<img width=60% src="https://raw.githubusercontent.com/cameronos/slim/main/slim_sc.png">

## Features
- Minimal X11 image viewer
- Zoom in/out with mouse wheel or +/- keys
- Pan images by left-mouse dragging
- Reset view with `R` key
- Image resizing when window is smaller

## Building
### Fedora
```bash
sudo dnf install libX11-devel imlib2-devel
gcc -O2 -o slim slim.c $(pkg-config --cflags --libs imlib2 x11)
sudo mv slim /usr/local/bin/
```

### Debian-based
```bash
sudo apt install libx11-dev libimlib2-dev
gcc -O2 -o slim slim.c $(pkg-config --cflags --libs imlib2 x11)
sudo mv slim /usr/local/bin/
```
You can also run the ./install_debian.sh if you cloned this repository if you want a seamless install.

### NetBSD
You **must** be root to compile SLIM on NetBSD, for whatever reason... 
```bash
su -
pkgin install modular-xorg imlib2 pkg-config
gcc -O2 -o slim slim.c $(pkg-config --cflags --libs imlib2 x11)
mv slim /usr/pkg/bin/
```

For other distros, find out how to get libX11 and libimlib2 installed, then just compile slim.c into an executable and run it.

## Usage
To run SLIM:

```bash
slim <image-path>
```
