dbusservicedir = $(datadir)/dbus-1/services
#nodist_dbusservicedir_DATA = whiteboard.service

%.service: %.service.in Makefile
	rm -f $@ $@.tmp
	sed -e 's,\@bindir\@,$(bindir),g' $< > $@.tmp
	mv $@.tmp $@
