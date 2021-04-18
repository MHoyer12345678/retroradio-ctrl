ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src

DISTCHECK_CONFIGURE_FLAGS = \
  --with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

if HAVE_SYSTEMD
systemdsystemunit_DATA = \
		conf/retroradio-controller.service
endif

retroradioconfdir=$(sysconfdir)/retroradiocontroller
retroradioconf_DATA = conf/retroradio.conf

asoundconfdir=$(sysconfdir)
asoundconf_DATA = conf/asound.conf
