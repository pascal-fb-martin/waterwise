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

HAPP=waterwise
HROOT=/usr/local
SHARE=$(HROOT)/share/house

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

install-app:
	mkdir -p $(HROOT)/bin
	mkdir -p /var/lib/house
	mkdir -p /etc/house
	rm -f $(HROOT)/bin/waterwise
	cp waterwise $(HROOT)/bin
	chown root:root $(HROOT)/bin/waterwise
	chmod 755 $(HROOT)/bin/waterwise
	mkdir -p $(SHARE)/public/waterwise
	cp public/* $(SHARE)/public/waterwise
	chmod 644 $(SHARE)/public/waterwise/*
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/waterwise
	touch /etc/default/waterwise

uninstall-app:
	rm -rf $(SHARE)/public/waterwise
	rm -f $(HROOT)/bin/waterwise

purge-app:

purge-config:
	rm -rf /etc/default/waterwise

# System installation. ------------------------------------------

include $(SHARE)/install.mak

# Docker installation -------------------------------------------

docker: all
	rm -rf build
	mkdir -p build
	cp Dockerfile build
	mkdir -p build$(HROOT)/bin
	cp waterwise build$(HROOT)/bin
	chmod 755 build$(HROOT)/bin/waterwise
	mkdir -p build$(HROOT)/share/house/public/waterwise
	cp public/* build$(HROOT)/share/house/public/waterwise
	chmod 644 build$(HROOT)/share/house/public/waterwise/*
	cp $(SHARE)/public/house.css build$(SHARE)/public
	chmod 644 build$(SHARE)/public/house.css
	cd build ; docker build -t waterwise .
	rm -rf build
