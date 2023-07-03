
OBJS=waterwise.o
LIBOJS=

SHARE=/usr/local/share/house

all: waterwise

main: waterwise.o

clean:
	rm -f *.o *.a waterwise

rebuild: clean all

%.o: %.c
	gcc -c -Os -o $@ $<

waterwise: $(OBJS)
	gcc -Os -o waterwise $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lrt

dev:

# Distribution agnostic file installation -----------------------

install-files:
	mkdir -p /usr/local/bin
	mkdir -p /var/lib/house
	mkdir -p /etc/house
	rm -f /usr/local/bin/waterwise
	cp waterwise /usr/local/bin
	chown root:root /usr/local/bin/waterwise
	chmod 755 /usr/local/bin/waterwise
	mkdir -p $(SHARE)/public/waterwise
	cp public/* $(SHARE)/public/waterwise
	chmod 644 $(SHARE)/public/waterwise/*
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/waterwise
	touch /etc/default/waterwise

uninstall-files:
	rm -rf $(SHARE)/public/waterwise
	rm -f /usr/local/bin/waterwise

purge-config:
	rm -rf /etc/default/waterwise

# Distribution agnostic systemd support -------------------------

install-systemd:
	cp systemd.service /lib/systemd/system/waterwise.service
	chown root:root /lib/systemd/system/waterwise.service
	systemctl daemon-reload
	systemctl enable waterwise
	systemctl start waterwise

uninstall-systemd:
	if [ -e /etc/init.d/waterwise ] ; then systemctl stop waterwise ; systemctl disable waterwise ; rm -f /etc/init.d/waterwise ; fi
	if [ -e /lib/systemd/system/waterwise.service ] ; then systemctl stop waterwise ; systemctl disable waterwise ; rm -f /lib/systemd/system/waterwise.service ; systemctl daemon-reload ; fi

stop-systemd: uninstall-systemd

# Debian GNU/Linux install --------------------------------------

install-debian: stop-systemd install-files install-systemd

uninstall-debian: uninstall-systemd uninstall-files

purge-debian: uninstall-debian purge-config

# Void Linux install --------------------------------------------

install-void: install-files

uninstall-void: uninstall-files

purge-void: uninstall-void purge-config

# Default install (Debian GNU/Linux) ----------------------------

install: install-debian

uninstall: uninstall-debian

purge: purge-debian

# Docker install ------------------------------------------------

docker: all
	rm -rf build
	mkdir -p build
	cp Dockerfile build
	mkdir -p build/usr/local/bin
	cp waterwise build/usr/local/bin
	chmod 755 build/usr/local/bin/waterwise
	mkdir -p build/usr/local/share/house/public/waterwise
	cp public/* build/usr/local/share/house/public/waterwise
	chmod 644 build/usr/local/share/house/public/waterwise/*
	cp /usr/local/share/house/public/house.css build/usr/local/share/house/public
	chmod 644 build/usr/local/share/house/public/house.css
	cd build ; docker build -t waterwise .
	rm -rf build
