
How to create an image that can be used for mouting?
	To create the PNG (RGBA) from JPG:
	 # convert sample.jpg -depth 8 -transparent none sample.png

	Note: Requires package ImageMagick


Dependencies:
	aptitude install libpng-dev
