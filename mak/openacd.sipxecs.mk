lib += openacd

openacd_VER = 2.0.0

# we'll have to generated this automatically based on the number
# of commits or something similar
openacd_PACKAGE_REVISION = 1

openacd_SRPM = openacd-$(openacd_VER)-$(openacd_PACKAGE_REVISION).src.rpm
openacd_TAR = $(SRC)/$(PROJ)/openacd-$(openacd_VER).tar.gz
openacd_SOURCES = $(openacd_TAR)

openacd_SRPM_DEFS = --define "buildno $(openacd_PACKAGE_REVISION)"
openacd_RPM_DEFS = --define="buildno $(openacd_PACKAGE_REVISION)"

openacd.autoreconf:
	cd $(SRC)/$(PROJ); \
	  autoreconf -if

openacd.configure:;
	cd $(SRC)/$(PROJ); \
	  ./configure

openacd.dist :
	cd $(SRC)/$(PROJ); \
	  make dist
