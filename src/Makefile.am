include $(top_srcdir)/config/Make-rules

SUBDIRS=std iface xml struct comm gxp graph sim

lib_LTLIBRARIES = libdlfree.la
libdlfree_la_SOURCES = dlfree.c
libdlfree_la_LIBADD = gxp/libgxp.la comm/libcomm.la struct/libstruct.la graph/libgraph.la xml/libxml.la iface/libiface.la std/libstd.la

bin_PROGRAMS = dlsim

dlsim_SOURCES = sim/simmain.c
dlsim_LDADD = sim/libsim.la xml/libxml.la graph/libgraph.la struct/libstruct.la std/libstd.la
