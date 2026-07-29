# Motion Changelog

Similar to a .plan file

## 2026-07-28

* added kernel and stack segment memory mappings
    * will be tied into mmu MemorySegment system

## 2026-07-29

* implemented basic mmu address translation
    * ComponentMMU, has a translate method that returns a bool if a bus error occurred.