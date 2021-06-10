Installation:
------------
make install

This will first invoke the 'configure'-script and then build the sources.
You can also run 'configure' manually. Then the following switches apply:
	--with-tfo      force enable tcp fast open
	--with-ncurses  force enable ncurses
	--with-openssl  force enable openssl
	--with-fftw3    force enable fftw3

Please note that TCP fast open requires a Linux kernel of version 3.7 or more recent.

'fftw3' support is only useful if you include the ncurses interface.

Debian package names:
	ncurses requires libncursesw5-dev
	fftw3 requires libfftw3-dev
	openssl requires libssl-dev


Usage:
-----
httping www.vanheusden.com


See:
	httping -h
for a list of commandline switches. Also check the man-page.

plot-json.py is a script to convert the json-output of httping to a script for gnuplot.

If this script fails with the following error:
	ValueError: Expecting object: [...]
then make sure the json-file ends with a ']' (without the quotes).
In some cases this character is missing.


Thanks to Thanatos for cookie and authentication support.
Many thanks to Olaf van der Spek for lots of bug-reports, testing, ideas and suggestions.


For everything more or less related to 'httping', please feel free
to contact me on: mail@vanheusden.com

Please support my opensource development: http://www.vanheusden.com/wishlist.php
Or send any surplus bitcoins to 1N5Sn4jny4xVwTwSYLnf7WnFQEGoVRmTQF
