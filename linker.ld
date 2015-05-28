ENTRY (main)

MEMORY
{
ram : ORIGIN = 0x00000000, LENGTH = 32M
rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{

    text 0x218000 : /* The actual instructions. */
    {
    *(.text)
    *(.got)
    *(.got.plt)
    *(.rodata)
    *(.rodata.str1.4)
    *(.glue_7)
    *(.glue_7t)
    } >ram

    data : /* Initialized data. */
    {
    _DataStart = . ;
    *(.data)
    *(.data.rel.ro.local)
    *(.data.rel.local)
    _DataEnd = . ;
    } >ram

    bss : /* Uninitialized data. */
    {
    _BssStart = . ;
    *(.bss)
    _BssEnd = . ;
    } >ram


}
