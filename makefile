KEXT=DerivedData/IntelWifi/Build/Products/Debug/IntelWifi.kext

OPTIONS:=$(OPTIONS) -scheme IntelWifi

ifeq ($(findstring 32,$(BITS)),32)
    OPTIONS:=$(OPTIONS) -arch i386
endif

ifeq ($(findstring 64,$(BITS)),64)
    OPTIONS:=$(OPTIONS) -arch x86_64
endif

.PHONY: build
build:
	xcodebuild build $(OPTIONS) -configuration Debug

.PHONY: deps
deps:
	sudo kextlibs -xml $(KEXT)

.PHONY: load
load:
	sudo chown -R root:wheel $(KEXT)
	sudo kextutil $(KEXT)

.PHONY: unload
unload:
	sudo kextunload $(KEXT)

.PHONY: clean
clean:
	sudo rm -rf $(KEXT)
