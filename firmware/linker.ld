/* Entry Point */
ENTRY(Reset_Handler)

/* Memory regions */
MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K   /* 128 KB SRAM */
  ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 512K    /* 512 KB Flash */
}

/* Sections */
SECTIONS
{
  /* Vector table placed at start of flash */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))   /* Vector table section (startup) */
    . = ALIGN(4);
  } >ROM

  /* Code (.text) and related sections */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* All .text code */
    *(.text*)          /* Additional code */
    *(.glue_7)         /* ARM to Thumb glue code */
    *(.glue_7t)        /* Thumb to ARM glue code */
    *(.eh_frame)

    KEEP(*(.init))     /* Optional init functions */
    KEEP(*(.fini))     /* Optional fini functions */

    . = ALIGN(4);
    _etext = .;        /* End of text section */
  } >ROM

  /* Read-only data */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >ROM

  /* Data section (initialized data in RAM, loaded from flash) */
  _sidata = LOADADDR(.data);
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> ROM

  /* Uninitialized data section (zero-initialized) */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM
    /* Heap starts here */
  . = ALIGN(4);
  _end = .;
  PROVIDE(end = _end);

}



/* Provide stack pointer symbol */
PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
