If a valid pointer is given to virtual realloc, and size is specified as 0,
 then virtual realloc acts as virtual free, and returns 0.
 
 Virtual malloc with size 0, returns NULL.
 
 If initial size is less than minimum size, then the code immediately exits,
 with error message:
 "Initial size cannot be less than the minimum block size"
 
 If virtual_sbrk fails, then the error message printed is:
 "virtual sbrk failed!"
