# PNGDrive - The easiest path to plausible deniability #

Secretly store your files within images without any trace.

<a href='http://www.youtube.com/watch?feature=player_embedded&v=TuNXhB_rXjU' target='_blank'><img src='http://img.youtube.com/vi/TuNXhB_rXjU/0.jpg' width='425' height=344 /></a>

## PNG meets Steganography meets Fuse ##

v1.0 is out!

The easiest way to have plausible deniability.


Usage:
**`pngdrive [-debug] [-key=<key>] [-mask=<mask>] [-format] <png image file> `**
  * debug: enables verbose/debug mode
  * key: uses `<key>` to encript the data
  * mask:  is an hexacimal 32bits integer starting with 0x, each bit set  in the mask will be used to store data, while bits that are not set remain unchanged. (e.g. -mask=0xFF0000 will mean that only the Red channel will contain data [RGB images](for.md)). Default mask is 0x01010300.
  * format: creates an empty filesystem using current values for 'key' and 'mask'.

How to create an image that can be used for mouting?
> To create the PNG (RGBA) from JPG:
    * `convert sample.jpg -depth 8 -transparent none sample.png`

> Note: Requires package ImageMagick


Dependencies:
  * `aptitude install libpng-dev libssl-doc libssl-dev`

Compiling:
  * `make`

Installing (as root):
  * `make install`

by [José Feiteirinha](http://www.feiteira.org)


See also:
[Wikipedia page](https://en.wikipedia.org/wiki/PNGDrive)