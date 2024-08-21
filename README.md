# OPENGLES-TEST

## Sub-Project

### Triangle

open X11 window and display a triangle.

> "Hello OpenGL"

### render_rgba

display a rgb24(folder name is wrong) file to x11 window, render directly, without any process.

### render_nv24

display a nv24 file to x11 window, use two textures to pass Y and UV planes, then transform to RGB in fragment shader.

### render

display yuv file(now support rgb24 and nv24) to x11 window, easy extending other pixelformats

### cvt_color(wip)

convert color space without renderring, this is the final target