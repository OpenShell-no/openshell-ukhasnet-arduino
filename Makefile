DEVICEFAMILY ?= atmega
DEVICE       ?= atmega328pb
CLOCK        ?= 8000000
SERIALPORT   ?= COM33
BAUDRATE     ?= 19200

NODENAME ?= OSTEST

BUILDDIR = ./build
ASSETS   = ./assets

PACKDIR ?= $(HOME)/tools/atmel/packs

LIBRARIES = ../libraries/ukhasnet-rfm69 ./libraries/onewire-ukhasnet $(PACKDIR)/$(DEVICEFAMILY)/latest/include/

DEVICEPACK = $(PACKDIR)/$(DEVICEFAMILY)/latest/gcc/dev/$(DEVICE)/

SOURCES = $(wildcard *.cpp) $(wildcard */*.cpp) ./libraries/onewire-ukhasnet/OneWire.cpp
OBJECTS = $(addprefix $(BUILDDIR)/,$(SOURCES:.cpp=.o)) ../libraries/ukhasnet-rfm69/ukhasnet-rfm69.o

VARIABLES = -DF_CPU=$(CLOCK) -D'AVR=' -DNODENAME=$(NODENAME)

INCLUDES = $(patsubst %,-I %,$(LIBRARIES))
# Compiler flags. Optimise for code size. Allow C99 standards.
COMPILE = avr-g++ -Wall -Wextra -w -pedantic -Os -gdwarf-2 -std=c++1y $(VARIABLES) -mmcu=$(DEVICE) -B $(DEVICEPACK) $(INCLUDES)
AVRDUDE_CONF ?= /etc/avrdude.conf
AVRDUDE = avrdude -C $(AVRDUDE_CONF) -c arduino -P $(SERIALPORT) -p $(DEVICE) -b $(BAUDRATE)

default: clean all

all: firmware_version.h $(ASSETS)/main.hex $(ASSETS)/main.eep

# "::" means allways remake file.
firmware_version.h::
	python2.7 ./hg_hooks/versionheader.py

# TODO: do the same for .c files
$(BUILDDIR)/%.o: %.cpp firmware_version.h
	@mkdir -p "$(@D)"
	$(COMPILE) -c $< -o $@

#$(BUILDDIR)/%.o: %.c firmware_version.h
#	@mkdir -p "$(@D)"
#	$(COMPILE) -c $< -o $@

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

$(ASSETS):;mkdir -p $@

$(BUILDDIR)/main.elf: $(OBJECTS)
	$(COMPILE) -o $@ $(OBJECTS)
	avr-size -C --mcu=$(DEVICE) $@

$(ASSETS)/main.hex: $(BUILDDIR)/main.elf
	@mkdir -p "$(@D)"
	rm -f $@
	avr-objcopy -j .text -j .data -O ihex $< $@

$(ASSETS)/main.eep: $(BUILDDIR)/main.elf
	@mkdir -p "$(@D)"
	avr-objcopy -j .eeprom -O ihex $< $@

.PHONY: clean upload auto fuses
clean:
	@-rm -rv $(BUILDDIR) $(ASSETS)
	@-rmdir /q /s "$(BUILDDIR)" "$(ASSETS)"

upload: $(ASSETS)/main.hex $(ASSETS)/main.eep
	$(AVRDUDE) -U flash:w:$(ASSETS)/main.hex:i -U eeprom:w:$(ASSETS)/main.eep:i

# efuse should be 0xfd, but that errors verifying with 0xf5. avrdude config error?
fuses:
	$(AVRDUDE) -U lfuse:w:0xe2:m -U hfuse:w:0xd1:m -U efuse:w:0xf5:m

auto: clean all fuses upload
