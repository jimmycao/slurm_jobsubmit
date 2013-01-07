# Makefile for dynalloc (resource dynamic allocation) plugin

AUTOMAKE_OPTIONS = foreign

AM_CXXFLAGS = -fexceptions

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src/common

#if SLURM_ENABLE_DYNAMIC_ALLOCATION
pkglib_LTLIBRARIES = job_submit_dynalloc.la

job_submit_dynalloc_la_SOURCES = \
	allocate.c	\
	allocate.h	\
	allocator.c	\
	allocator.h	\
	argv.c		\
	argv.h		\
	info.c	\
	info.h	\
	msg.c	\
	msg.h	\
	job_submit_dynalloc.c
job_submit_dynalloc_la_LDFLAGS = $(SO_LDFLAGS) $(PLUGIN_FLAGS)
#endif

