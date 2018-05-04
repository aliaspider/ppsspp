// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include "ppsspp_config.h"

#ifdef __wiiu__

#include <string>
#include <wiiu/os/memory.h>
#include <wiiu/os/debug.h>

#include "FileUtil.h"
#include "MemoryUtil.h"
#include "MemArena.h"


size_t MemArena::roundup(size_t x) { return (x + (OS_MMAP_PAGE_SIZE - 1)) & ~(OS_MMAP_PAGE_SIZE - 1); }
void *rounddown(void *addr) { return (void *)((uintptr_t)addr & ~(OS_MMAP_PAGE_SIZE - 1)); }

bool MemArena::NeedsProbing() { return false; }

void MemArena::GrabLowMemSpace(size_t size) {
	// TODO: this is unreliable as it could be fragmented.
	memblock = (u8 *)MEM2_alloc(size, OS_MMAP_PAGE_SIZE);
	DEBUG_VAR(memblock);
	DEBUG_VAR(OSEffectiveToPhysical(memblock));
}

void MemArena::ReleaseSpace() {
	MEM2_free(memblock);
	memblock = nullptr;
}

void *MemArena::CreateView(s64 offset, size_t size, void *base) {
	// TODO: [oldbase, oldbase + oldsize] needs to be inside [newbase, newbase + newsize], then return oldbase
	// this should work since there is no page collisions on the requested views.
	size = roundup(size);
	size_t diff = (u32)base & (OS_MMAP_PAGE_SIZE - 1);
	base = OSAllocVirtAddr(rounddown(base), size, OS_MMAP_PAGE_SIZE);
	if (!OSMapMemory(base, OSEffectiveToPhysical(memblock + offset), size, OS_MMAP_RW))
		return nullptr;

	return (u8*)base + diff;
}

void MemArena::ReleaseView(void *view, size_t size) {
	OSUnmapMemory(rounddown(view), roundup(size));
	OSFreeVirtAddr(rounddown(view), roundup(size));
}

u8 *MemArena::Find4GBBase() {
	size_t size = 0x10000000;
	void *base = OSAllocVirtAddr(nullptr, size, OS_MMAP_PAGE_SIZE);
	if (!base) {
		PanicAlert("Failed to map 256 MB of memory space");
		return nullptr;
	}
	OSFreeVirtAddr(base, size);
	return (u8 *)base;
}

#endif
