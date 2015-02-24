# libds
# Copyright (c) Chunfeng Zhang(CrazyPandar@gmail.com), All rights reserved.
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3.0 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FIDSESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library.
#

############### config #################
PROJ_NAME = libds
TARGETS = libds
###############################################

VER ?= $(shell cat ../VERSION)
RELEASE_PATH ?= ..
export VER

defaut: all

help configure all install clean distclean:
	@for i in `echo $(TARGETS)`; do \
		$(MAKE) -f $$i.mf $@ || exit 1; \
	done

release:clean
	rm -f ../$(PROJ_NAME)-$(VER).tar.gz
	cd .. && tar -zcf  $(PROJ_NAME)-$(VER).tar.gz $(PROJ_NAME) --exclude=.svn

.PHONY:default help all install clean release distclean
