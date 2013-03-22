bin_PROGRAMS = redsibd

# Compiler flags
redsibd_CFLAGS  = -Wall -I$(top_srcdir)/include -I/usr/local/include -I/usr/include -I.
redsibd_CFLAGS += @GNOME_CFLAGS@ @WHITEBOARD_CFLAGS@ @LIBSIB_CFLAGS@


# TODO: Define this in a more global and flexible way

redsibd_CFLAGS += -DSIB_LIBEXECDIR=\"$(libdir)/sib/libexec\"
if DEBUG
redsibd_CFLAGS += -g -O0
else
redsibd_CFLAGS += -g -O2
endif


# Linker flags -- 25.5.2009 added libpiglet / jh
redsibd_LDFLAGS = @GNOME_LIBS@  @WHITEBOARD_LIBS@ @LIBSIB_LIBS@  -lgthread-2.0 -luuid -lrasqal -lraptor -ldl -L/usr/lib/i386-linux-gnu -lrdf


AM_CFLAGS = $(redsibd4_CFLAGS) -g

# Put these in alphabetical order so they are easy to find.
# Leave main.c out of this list because it must not be present
# in the unit testing library build.
sources = \
	dbushandler.c \
	sib_control.c \
	sib_operations.c \
	LCTableTools.c \
	utilities.c \
	reasoning.c


redsibd_SOURCES = \
	main.c \
	$(sources)
