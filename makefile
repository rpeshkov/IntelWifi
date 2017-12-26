KEXT=DerivedData/IntelWifi/Build/Products/Debug/IntelWifi.kext

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
