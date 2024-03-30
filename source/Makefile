COMPONENTS := libstratosphere AppletCompanion Sysmodule
TOPTARGETS := all clean mrproper

.PHONY: $(TOPTARGETS) $(COMPONENTS)

all: $(COMPONENTS)

libstratosphere:
	$(MAKE) -C ../lib/Atmosphere-libs/libstratosphere

AppletCompanion:
	$(MAKE) -C AppletCompanion

Sysmodule: libstratosphere
	$(MAKE) -C Sysmodule

clean:
	$(MAKE) -C AppletCompanion clean
	$(MAKE) -C Sysmodule clean
	
mrproper: clean
	$(MAKE) -C libstratosphere clean