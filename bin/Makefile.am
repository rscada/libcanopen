#-------------------------------------------------------------------------------
# Copyright (C) 2012, Robert Johansson <rob@raditex.nu>, Raditex Control AB
# All rights reserved.
#
# This file is part of rSCADA.
# http://www.rSCADA.se
# info@rscada.se
# 
#-------------------------------------------------------------------------------

PACKAGE	= @PACKAGE@
VERSION	= @VERSION@

AM_CPPFLAGS	= -I$(top_builddir) -I$(top_srcdir)

noinst_HEADERS = 
bin_PROGRAMS   = rs-canopen-ds401 rs-canopen-monitor rs-canopen-node-info \
                 rs-canopen-pdo-request rs-canopen-sdo-download rs-canopen-dump \
                 rs-canopen-nmt rs-canopen-pdo-download rs-canopen-pdo-upload \
                 rs-canopen-sdo-upload

# 
rs_canopen_ds401_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_ds401_LDADD	 = -lcanopen 
rs_canopen_ds401_SOURCES = rs-canopen-ds401.c 

rs_canopen_monitor_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_monitor_LDADD   = -lcanopen 
rs_canopen_monitor_SOURCES = rs-canopen-monitor.c

rs_canopen_node_info_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_node_info_LDADD	 = -lcanopen 
rs_canopen_node_info_SOURCES = rs-canopen-node-info.c 

rs_canopen_pdo_request_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_pdo_request_LDADD   = -lcanopen 
rs_canopen_pdo_request_SOURCES = rs-canopen-pdo-request.c 

rs_canopen_sdo_download_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_sdo_download_LDADD	= -lcanopen 
rs_canopen_sdo_download_SOURCES = rs-canopen-sdo-download.c

rs_canopen_dump_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_dump_LDADD   = -lcanopen 
rs_canopen_dump_SOURCES = rs-canopen-dump.c

rs_canopen_nmt_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_nmt_LDADD   = -lcanopen 
rs_canopen_nmt_SOURCES = rs-canopen-nmt.c

rs_canopen_pdo_download_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_pdo_download_LDADD	= -lcanopen 
rs_canopen_pdo_download_SOURCES = rs-canopen-pdo-download.c

rs_canopen_pdo_upload_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_pdo_upload_LDADD	  = -lcanopen 
rs_canopen_pdo_upload_SOURCES = rs-canopen-pdo-upload.c

rs_canopen_sdo_upload_LDFLAGS = -L$(top_builddir)/canopen
rs_canopen_sdo_upload_LDADD	  = -lcanopen 
rs_canopen_sdo_upload_SOURCES = rs-canopen-sdo-upload.c


