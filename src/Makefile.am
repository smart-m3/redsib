bin_PROGRAMS = redsibd

# Compiler flags
redsibd_CFLAGS  =  -Wall  -I$(top_srcdir)/include -I/usr/local/include -I/usr/include 
redsibd_CFLAGS += @GNOME_CFLAGS@ @WHITEBOARD_CFLAGS@ @LIBSIB_CFLAGS@


# TODO: Define this in a more global and flexible way

redsibd_CFLAGS += -DSIB_LIBEXECDIR=\"$(libdir)/sib/libexec\"
if DEBUG
redsibd_CFLAGS += -g -O0
else
redsibd_CFLAGS += -g -O2
endif


# Linker flags -- 
redsibd_LDFLAGS = -lgthread-2.0 -luuid -ldl  -L/usr/lib/i386-linux-gnu  -L/usr/local/lib  -lrasqal -lraptor2 -lrdf 
redsibd_LDFLAGS += @GNOME_LIBS@  @WHITEBOARD_LIBS@ @LIBSIB_LIBS@  


AM_CFLAGS = $(redsibd_CFLAGS) -g

# Put these in alphabetical order so they are easy to find.
# Leave main.c out of this list because it must not be present
# in the unit testing library build.
sources = \
	dbushandler.c \
	sib_control.c \
	sib_operations.c \
	LCTableTools.c \
	core_utilities.c \
	reasoning.c \
	sparql_booster.c


redsibd_SOURCES = \
	main.c \
	$(sources)
