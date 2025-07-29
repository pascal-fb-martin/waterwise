# waterwise - A simple home web service to get the bewaterwise.com index.
#
# Copyright 2023, Pascal Martin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

prefix=/usr/local

HAPP=waterwise
SHARE=$(prefix)/share/house

INSTALL=/usr/bin/install

# Application build. --------------------------------------------

OBJS=waterwise.o
LIBOJS=

all: waterwise

main: waterwise.o

clean:
	rm -f *.o *.a waterwise

rebuild: clean all

%.o: %.c
	gcc -c -Os -o $@ $<

waterwise: $(OBJS)
	gcc -Os -o waterwise $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lrt

# Application files installation --------------------------------

install-ui: install-preamble
	$(INSTALL) -m 0755 -d $(DESTDIR)$(SHARE)/public/waterwise
	$(INSTALL) -m 0644 public/* $(DESTDIR)$(SHARE)/public/waterwise

install-app: install-ui
	$(INSTALL) -m 0755 -s waterwise $(DESTDIR)$(DESTDIR)$(prefix)/bin
	touch $(DESTDIR)/etc/default/waterwise

uninstall-app:
	rm -rf $(DESTDIR)$(SHARE)/public/waterwise
	rm -f $(DESTDIR)$(prefix)/bin/waterwise

purge-app:

purge-config:
	rm -rf $(DESTDIR)/etc/default/waterwise

# System installation. ------------------------------------------

include $(SHARE)/install.mak

# Docker installation -------------------------------------------

docker: all
	rm -rf build
	mkdir -p build
	cp Dockerfile build
	mkdir -p build$(prefix)/bin
	cp waterwise build$(prefix)/bin
	chmod 755 build$(prefix)/bin/waterwise
	mkdir -p build$(prefix)/share/house/public/waterwise
	cp public/* build$(prefix)/share/house/public/waterwise
	chmod 644 build$(prefix)/share/house/public/waterwise/*
	cp $(SHARE)/public/house.css build$(SHARE)/public
	chmod 644 build$(SHARE)/public/house.css
	cd build ; docker build -t waterwise .
	rm -rf build

