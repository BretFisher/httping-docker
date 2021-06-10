# The GPL applies to this program.
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
# You must obey the GNU General Public License in all respects
# for all of the code used other than OpenSSL.  If you modify
# file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so.  If you
# do not wish to do so, delete this exception statement from your
# version.  If you delete this exception statement from all source
# files in the program, then also delete it here.

-include makefile.inc

# *** configure script ***
# support for tcp fast open?
#TFO=yes
# disable SSL? (no = disable so the default is use openssl)
# SSL=no
# enable NCURSES interface?
#NC=yes
# do fft in ncurses interface? (requires libfftw3)
#FW=yes

############# do not change anything below here #############

include version

TARGET=httping

LOCALEDIR=/usr/share/locale

DEBUG=yes
WFLAGS=-Wall -W -Wextra -pedantic -D_FORTIFY_SOURCE=2
OFLAGS=
CFLAGS+=$(WFLAGS) $(OFLAGS) -DVERSION=\"$(VERSION)\" -DLOCALEDIR=\"$(LOCALEDIR)\"
LDFLAGS+=-lm

PACKAGE=$(TARGET)-$(VERSION)
PREFIX?=/usr
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man
DOCDIR=$(PREFIX)/share/doc/$(TARGET)

INSTALL=install
INSTALLDIR=$(INSTALL) -m 0755 -d
INSTALLBIN=$(INSTALL) -m 0755
INSTALLMAN=$(INSTALL) -m 0644
INSTALLDOC=$(INSTALL) -m 0644
STRIP=/usr/bin/strip
RMDIR=/bin/rm -rf
MKDIR=/bin/mkdir
ARCHIVE=/bin/tar cf -
COMPRESS=/bin/gzip -9

TRANSLATIONS=nl.mo ru.mo

OBJS=gen.o http.o io.o error.o utils.o main.o tcp.o res.o socks5.o kalman.o cookies.o help.o colors.o

MAN_EN=httping.1
MAN_NL=httping-nl.1
MAN_RU=httping-ru.1

DOCS=license.txt license.OpenSSL readme.txt

ifeq ($(SSL),no)
CFLAGS+=-DNO_SSL
else
OBJS+=mssl.o
LDFLAGS+=-lssl -lcrypto
endif

ifeq ($(TFO),yes)
CFLAGS+=-DTCP_TFO
endif

ifeq ($(NC),yes)
CFLAGS+=-DNC
OBJS+=nc.o
LDFLAGS+=-lncursesw
endif

ifeq ($(FW),yes)
CFLAGS+=-DFW
OBJS+=fft.o
LDFLAGS+=-lfftw3
endif

ifeq ($(DEBUG),yes)
CFLAGS+=-D_DEBUG -ggdb
LDFLAGS+=-g
endif

ifeq ($(ARM),yes)
CC=arm-linux-gcc
endif

all: $(TARGET) $(TRANSLATIONS)

$(TARGET): $(OBJS)
	$(CC) $(WFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)
	#
	# Oh, blatant plug: http://www.vanheusden.com/wishlist.php

install: $(TARGET) $(TRANSLATIONS)
	$(INSTALLDIR) $(DESTDIR)/$(BINDIR)
	$(INSTALLBIN) $(TARGET) $(DESTDIR)/$(BINDIR)
	$(INSTALLDIR) $(DESTDIR)/$(MANDIR)/man1
	$(INSTALLMAN) $(MAN_EN) $(DESTDIR)/$(MANDIR)/man1
	$(INSTALLDIR) $(DESTDIR)/$(MANDIR)/nl/man1
	$(INSTALLMAN) $(MAN_NL) $(DESTDIR)/$(MANDIR)/nl/man1
	$(INSTALLDIR) $(DESTDIR)/$(MANDIR)/ru/man1
	$(INSTALLMAN) $(MAN_RU) $(DESTDIR)/$(MANDIR)/ru/man1
	$(INSTALLDIR) $(DESTDIR)/$(DOCDIR)
	$(INSTALLDOC) $(DOCS) $(DESTDIR)/$(DOCDIR)
ifneq ($(DEBUG),yes)
	$(STRIP) $(DESTDIR)/$(BINDIR)/$(TARGET)
endif
	mkdir -p $(DESTDIR)/$(PREFIX)/share/locale/nl/LC_MESSAGES
	cp nl.mo $(DESTDIR)/$(PREFIX)/share/locale/nl/LC_MESSAGES/httping.mo
	mkdir -p $(DESTDIR)/$(PREFIX)/share/locale/ru/LC_MESSAGES
	cp ru.mo $(DESTDIR)/$(PREFIX)/share/locale/ru/LC_MESSAGES/httping.mo


makefile.inc:
	./configure

nl.mo: nl.po
	msgfmt -o nl.mo nl.po
ru.mo: ru.po
	msgfmt -o ru.mo ru.po


clean:
	$(RMDIR) $(OBJS) $(TARGET) *~ core cov-int *.mo

distclean: clean
	rm -f makefile.inc

package:
	# source package
	$(RMDIR) $(PACKAGE)*
	$(MKDIR) $(PACKAGE)
	$(INSTALLDOC) *.c *.h configure Makefile *.po version $(MAN_EN) $(MAN_NL) $(MAN_RU) $(DOCS) $(PACKAGE)
	$(INSTALLBIN) configure $(PACKAGE)
	$(ARCHIVE) $(PACKAGE) | $(COMPRESS) > $(PACKAGE).tgz
	$(RMDIR) $(PACKAGE)

check: makefile.inc
	cppcheck -v --force -j 3 --enable=all --std=c++11 --inconclusive -I. . 2> err.txt
	#
	make clean
	scan-build make

coverity: makefile.inc
	make clean
	rm -rf cov-int
	CC=gcc cov-build --dir cov-int make all
	tar vczf ~/site/coverity/httping.tgz README cov-int/
	putsite -q
	/home/folkert/.coverity-hp.sh
