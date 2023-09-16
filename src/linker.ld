ENTRY(main)
SECTIONS {
  . = 0x200000;          /* start text at this location */
  .text : {              /* force main to start of text section, with all other functions following */
    *(.text.main);
    *(.text*)
  }
}
