#! /bin/sh
modprobe cdiousb
cp -f 180-cdiousb.rules /etc/udev/rules.d
cp -f contec_dio.conf /usr/local/etc/contec_dio.conf
#end
