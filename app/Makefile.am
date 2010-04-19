AM_CFLAGS = -I$(top_srcdir)/include -g -Wall \
            -D_REENTRANT

bin_PROGRAMS = ringall2all lwall2all dlfree

# all-to-all broadcast program for ring topology
ringall2all_SOURCES = ringall2all.c collective.c
ringall2all_LDADD = $(top_builddir)/src/libdlfree.la

# all-to-all broadcast program for locality aware topology
lwall2all_SOURCES = lwall2all.c collective.c
lwall2all_LDADD = $(top_builddir)/src/libdlfree.la

# program used for data collection for HPDC2009
dlfree_SOURCES = ex1.c collective.c
dlfree_LDADD = $(top_builddir)/src/libdlfree.la
