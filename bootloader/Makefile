# program name should not be changed...
PROGRAM    = bootloader

# enter the parameters for the UISP isp tool
ISPPARAMS  = -dprog=stk200 -dlpt=0x378

############################################################
# You should not have to change anything below here.
############################################################

MCU_TARGET = atmega128
LDSECTION  = --section-start=.text=0x1E000
FUSE_L     = 0xdf
FUSE_H     = 0xc8
FUSE_E     = 0xff
ISPFUSES   = uisp -dpart=ATmega128 $(ISPPARAMS) --wr_fuse_l=$(FUSE_L) --wr_fuse_h=$(FUSE_H) --wr_fuse_e=$(FUSE_E)
ISPFLASH   = uisp -dpart=ATmega128 $(ISPPARAMS) --erase --upload if=$(PROGRAM).hex -v

OBJ        = $(PROGRAM).o
OPTIMIZE   = -O3

DEFS       =
LIBS       =

CC         = avr-gcc


# Override is only needed by avr-lib build system.

override CFLAGS   = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS  = -Wl,-Map,$(PROGRAM).map,$(LDSECTION)

OBJCOPY           = avr-objcopy
OBJDUMP           = avr-objdump

all: $(PROGRAM).elf lst text

isp: $(PROGRAM).hex
	$(ISPFUSES)
	$(ISPFLASH)

$(PROGRAM).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *.elf
	rm -rf *.lst *.map

lst:  $(PROGRAM).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PROGRAM).hex
bin:  $(PROGRAM).bin
srec: $(PROGRAM).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@
