
OBJS=waterwise.o
LIBOJS=

SHARE=/usr/local/share/house

all: waterwise

main: waterwise.o

clean:
	rm -f *.o *.a waterwise

rebuild: clean all

%.o: %.c
	gcc -c -g -O -o $@ $<

waterwise: $(OBJS)
	gcc -g -O -o waterwise $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lrt

install:
	if [ -e /etc/init.d/waterwise ] ; then systemctl stop waterwise ; fi
	mkdir -p /usr/local/bin
	mkdir -p /var/lib/house
	mkdir -p /etc/house
	rm -f /usr/local/bin/waterwise /etc/init.d/waterwise
	cp waterwise /usr/local/bin
	cp init.debian /etc/init.d/waterwise
	chown root:root /usr/local/bin/waterwise /etc/init.d/waterwise
	chmod 755 /usr/local/bin/waterwise /etc/init.d/waterwise
	mkdir -p $(SHARE)/public/waterwise
	cp public/* $(SHARE)/public/waterwise
	chmod 644 $(SHARE)/public/waterwise/*
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/waterwise
	touch /etc/default/waterwise
	systemctl daemon-reload
	systemctl enable waterwise
	systemctl start waterwise

uninstall:
	systemctl stop waterwise
	systemctl disable waterwise
	rm -f /usr/local/bin/waterwise /etc/init.d/waterwise
	systemctl daemon-reload

purge: uninstall
	rm -rf /etc/default/waterwise

