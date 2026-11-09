ARCH ?= x86_64
BASE_CFLAGS += $(foreach d, $(INCLUDE), -I$d)
BASE_CFLAGS += -fno-stack-protector -mno-red-zone -fshort-wchar -fpic -ffreestanding -Os
CFLAGS += $(BASE_CFLAGS) -std=c17
CXXFLAGS += $(BASE_CFLAGS) -std=c++26 -fno-exceptions -fno-unwind-tables
EFI_INC = /usr/include/efi
EFI_ARCH_INC = $(EFI_INC)/$(ARCH)
INCLUDE = $(EFI_INC) $(EFI_ARCH_INC)

EFI_LIBDIR ?= /usr/lib
EFI_CRT = $(EFI_LIBDIR)/crt0-efi-$(ARCH).o
EFI_LDSCRIPT = $(EFI_LIBDIR)/elf_$(ARCH)_efi.lds
EFI_LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDSCRIPT) -shared -Bsymbolic --no-undefined
EFI_LIBS = -lgnuefi -lefi
OBJCOPY ?= objcopy
OBJCOPY_SECTIONS = .text .sdata .data .rodata .dynamic .dynsym .rel .rela .reloc
OBJCOPYFLAGS = $(foreach s, $(OBJCOPY_SECTIONS), -j $(s)) -O efi-app-$(ARCH)

SRC_DIRS = src/loader
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c) $(wildcard $(dir)/*.cpp))
BUILD_DIR = build
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))
OBJS := $(addprefix $(BUILD_DIR)/,$(OBJS))
TARGET = $(BUILD_DIR)/paradox.efi
EFI_ELF = $(TARGET:.efi=.so)

EFI_FS = efi_root
EFI_FS_BOOT = $(EFI_FS)/EFI/BOOT
EFI_FS_EXE = $(EFI_FS_BOOT)/BOOTX64.EFI
EFI_FS_CONFIG = $(EFI_FS_BOOT)/paradox.cfg

OS_IMG = os.img

.PHONY: all
all: $(TARGET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EFI_ELF): $(OBJS)
	$(LD) $(EFI_LDFLAGS) -L$(EFI_LIBDIR) $(EFI_CRT) $(OBJS) $(EFI_LIBS) -o $@

$(TARGET): $(EFI_ELF)
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

$(EFI_FS_BOOT):
	mkdir -p $(@)

$(EFI_FS_EXE): $(TARGET) | $(EFI_FS_BOOT)
	cp $(<) $(@)

$(EFI_FS_CONFIG): | $(EFI_FS_BOOT)
	echo "some key=some value\nname  =  example\nstart=\n=end" > $(@)

$(OS_IMG):
	dd if=/dev/zero of=$(@).tmp bs=1M count=100
	echo "label: gpt\nstart=8M,type=\"Microsoft basic data\"" | sfdisk $(@).tmp
	mformat -Fi $(@).tmp@@8M
	mv $(@).tmp $(@)

.PHONY: test
test: $(EFI_FS_EXE) $(EFI_FS_CONFIG) $(OS_IMG)
	qemu-system-x86_64 -drive file=fat:rw:$(EFI_FS),format=raw \
		-drive file=$(OS_IMG),format=raw,media=disk \
		-machine graphics=off \
		-nographic \
		-bios OVMF.fd \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		--no-reboot \
		|| true

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(EFI_FS) $(OS_IMG) $(OS_IMG).tmp
