SDVX Memory Reader
------------------
C library for reading memory of Sound Voltex Exceed Gear. 
It does not implement any methods to inject itself into the program, but instead just calls ReadProcessMemory.
I plan to add more to it, because at the moment it's fairly barebones. 
I also need to add docs on some more of the functions, but as of now it works fine. 
Future updates may break it, in which case you can just create an `issue`_.

Programs that use this library
##############################


How to use
##########
memory_reader contains the functionality to control everything. 

* ``memory_reader_init`` looks for the program and finds all the memory locations. 
* ``memory_reader_cleanup`` performs cleanup for when you're done using the memory reader.
* ``memory_reader_update`` is an integral function that reads memory and updates the values of ``MemoryData``. The values of ``MemoryData`` won't update otherwise. 
* ``MemoryData`` is a variable that contains all the data read from the sdvx process after ``memory_reader_update`` is called.
* If you encounter any problems, just make an `issue`_.

.. _issue: https://github.com/Sheppsu/sdvx_memory_reader/issues
