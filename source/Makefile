TARGETS := AppletCompanion Sysmodule/source/version.h Sysmodule
TOPTARGETS := all clean mrproper

GIT_HASH := $(shell git rev-parse --short HEAD)
GIT_TAG := $(shell git describe --tags `git rev-list --tags --max-count=1`)
GIT_TAG_COMMIT_COUNT := $(shell git rev-list  `git rev-list --tags --no-walk --max-count=1`..HEAD --count)

.PHONY: $(TOPTARGETS) $(TARGETS)

all: $(TARGETS)

libstratosphere:
	$(MAKE) -C ../lib/Atmosphere-libs/libstratosphere

AppletCompanion:
	$(MAKE) -C AppletCompanion

Sysmodule/source/version.h: ../.git/HEAD ../.git/index
	echo "#pragma once" > $@
	echo "namespace syscon::version" >> $@
	echo "{" >> $@
	echo "const char * syscon_tag = \"$(GIT_TAG)\";" >> $@
	echo "const char * syscon_git_hash = \"$(GIT_HASH)\";" >> $@
	echo "const unsigned int syscon_commit_count = $(GIT_TAG_COMMIT_COUNT);" >> $@
	echo "}" >> $@

Sysmodule: libstratosphere Sysmodule/source/version.h
	$(MAKE) -C Sysmodule

clean:
	$(MAKE) -C AppletCompanion clean
	$(MAKE) -C Sysmodule clean
	
mrproper: clean
	$(MAKE) -C libstratosphere clean