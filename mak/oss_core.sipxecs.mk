oss_core_VER = 2.0.0
oss_core_REV = $(shell cd $(SRC)/$(PROJ); ../config/revision-gen $(oss_core_VER))
oss_core_TAR = $(PROJ)/oss_core-$(oss_core_VER).tar.gz
oss_core_SRPM = oss_core-$(oss_core_VER)-$(oss_core_REV).src.rpm
oss_core_OPTIONS = --disable-dep-check

oss_core.dist : oss_core.autoreconf oss_core.configure oss_core.dist_;
