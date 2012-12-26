# Makefile for dynalloc (resource dynamic allocation) plugin

AUTOMAKE_OPTIONS = foreign

AM_CXXFLAGS = -fexceptions

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src/common

pkglib_LTLIBRARIES = sched_dynalloc.la

sched_dynalloc_la_SOURCES = \
	allocate.c	\
	allocate.h	\
	info.c	\
	info.h	\
	ip_port.c \
	ip_port.h \
	msg.c	\
	msg.h	\
	sched_dynalloc.c
sched_dynalloc_la_LDFLAGS = $(SO_LDFLAGS) $(PLUGIN_FLAGS)