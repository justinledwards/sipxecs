include $(top_srcdir)/config/utility.am

EXTRA_DIST = \
	$(bin_SCRIPTS:=.in) \
	$(initd_SCRIPTS:=.in)

bin_SCRIPTS = \
	mongodb-status \
	mongodb-analyzer \
	mongodb-admin

check :
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 --replSet sipxecs \
	   --parse < $(srcdir)/test_data/bad-replset.json > x
	diff x $(srcdir)/test_data/bad-replset.status.json
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 \
	   --replSet sipxecs --parse < $(srcdir)/test_data/two-node-missing-arbiter.json > x
	diff x $(srcdir)/test_data/two-node-missing-arbiter.status.json
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 \
	   --arbiters swift.hubler.us:27017 \
	   --replSet sipxecs --parse < $(srcdir)/test_data/three-node-unavailable-server.json > x
	diff x $(srcdir)/test_data/three-node-unavailable-server.status.json
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 \
	   --arbiters swift.hubler.us:27017 \
	   --replSet sipxecs --parse < $(srcdir)/test_data/three-node-unhealthy-server.json > x
	diff x $(srcdir)/test_data/three-node-unhealthy-server.status.json
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 \
	   --arbiters swift.hubler.us:27017 \
	   --replSet sipxecs --parse < $(srcdir)/test_data/three-node-missing-arbiter-and-database.json > x
	diff x $(srcdir)/test_data/three-node-missing-arbiter-and-database.status.json
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 \
	   --arbiters swift.hubler.us:27017 \
	   --replSet sipxecs --parse < $(srcdir)/test_data/three-node-missing-database.json > x
	diff x $(srcdir)/test_data/three-node-missing-database.status.json
	$(srcdir)/mongodb-status.in \
	   --servers swift.hubler.us:27017 swift.hubler.us:27019 \
	   --arbiters swift.hubler.us:27017 \
	   --replSet sipxecs --parse < $(srcdir)/test_data/three-node-healthy.json > x
	diff x $(srcdir)/test_data/three-node-healthy.status.json

initddir = @SIPX_SERVICEDIR@
initd_SCRIPTS = \
	mongod-arbiter

$(initd_SCRIPTS) $(bin_SCRIPTS) : % : %.in
	@$(call SearchAndReplace,$<,$@)

CLEANFILES = $(initd_SCRIPTS) $(bin_SCRIPTS)
