include $(top_srcdir)/config/utility.am

EXTRA_DIST = \
	$(bin_SCRIPTS:=.in) \
	$(initd_SCRIPTS:=.in)

bin_SCRIPTS = \
	mongodb-status \
	mongodb-analyzer \
	mongodb-admin

mongodb_TESTS = \
	bad-replset \
	two-node-missing-arbiter \
	three-node-unavailable-server \
	three-node-unhealthy-server \
	three-node-missing-arbiter-and-database \
	three-node-missing-database \
	three-node-healthy

mongodb_admin_TESTS = \
	t1

check : check-mongodb-status check-mongodb-analyzer check-mongodb-admin;

check-mongodb-status:
	@echo "====mongodb-status tests===="
	for t in $(mongodb_TESTS); do \
	  echo $(srcdir)/test_data/$$t.status.json; \
	  $(srcdir)/mongodb-status.in \
	   --parse < $(srcdir)/test_data/$$t.json > x; \
	  diff x $(srcdir)/test_data/$$t.status.json || exit 1; \
	done

check-mongodb-analyzer:
	@echo "====mongodb-analysis tests===="
	for t in $(mongodb_TESTS); do \
	  echo $(srcdir)/test_data/$$t.analysis.json; \
	  $(srcdir)/mongodb-analyzer.in < $(srcdir)/test_data/$$t.status.json > x; \
	  diff x $(srcdir)/test_data/$$t.analysis.json || exit 1; \
	done

check-mongodb-admin:
	@echo "====mongodb-admin tests===="
	for t in $(mongodb_admin_TESTS); do \
	  echo $(srcdir)/test_data/$$t.primary.dryrun; \
	    $(srcdir)/mongodb-admin.in --host_port swift.hubler.us:27017 --dryrun \
	      --model $(srcdir)/test_data/admin.model.json < $(srcdir)/test_data/$$t.primary.commands > x; \
	  diff x $(srcdir)/test_data/$$t.primary.dryrun || exit 1; \
	done
	for t in $(mongodb_admin_TESTS); do \
	  echo $(srcdir)/test_data/$$t.secondary.dryrun; \
	    $(srcdir)/mongodb-admin.in --host_port swift.hubler.us:27019 --dryrun \
	      --model $(srcdir)/test_data/admin.model.json < $(srcdir)/test_data/$$t.secondary.commands > x; \
	  diff x $(srcdir)/test_data/$$t.secondary.dryrun || exit 1; \
	done
	for t in $(mongodb_admin_TESTS); do \
	  echo $(srcdir)/test_data/$$t.arbiter.dryrun; \
	    $(srcdir)/mongodb-admin.in --host_port swift.hubler.us:27018 --dryrun \
	      --model $(srcdir)/test_data/admin.model.json < $(srcdir)/test_data/$$t.arbiter.commands > x; \
	  diff x $(srcdir)/test_data/$$t.arbiter.dryrun || exit 1; \
	done

initddir = @SIPX_SERVICEDIR@
initd_SCRIPTS = \
	mongod-arbiter

$(initd_SCRIPTS) $(bin_SCRIPTS) : % : %.in
	@$(call SearchAndReplace,$<,$@)

CLEANFILES = $(initd_SCRIPTS) $(bin_SCRIPTS)
