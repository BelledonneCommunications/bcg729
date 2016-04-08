SUBDIRS = bin

testCampaignAll-exec: $(srcdir)/testCampaignAll $(srcdir)/testCampaign
	cp -f $(srcdir)/testCampaignAll testCampaignAll-exec
	cp -f $(srcdir)/testCampaign testCampaign-exec


TESTS = testCampaignAll-exec

EXTRA_DIST=testCampaignAll testCampaign

CLEANFILES=testCampaignAll-exec testCampaign-exec

clean-local:
	rm -rf patterns
