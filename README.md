# nuaf (No Using After Freeing)
Included in this repository is the source code, tests, and log files regarding performance of this system. Refer to report.pdf for more information regarding this system. The high-level idea of our system is derived from Oscar 2017.  

## To make our system:  
call `make` at the root of the repository.  

## To use our system:  
use `LD_PRELOAD=/path/to/nuaf.so <program>`.  

## Things to keep in mind:  
* Our system currently _will_ clobber any existing mappings in the program if it runs into them. Starting virtual addresses used by our system are hardcoded in a way that will support the lifetime of most programs. These may need to be changed if dealing with weird programs that make many virtual mappings, or are compiled with something other than basic gcc/clang. 
* You may need to increase the system-wide virtual mapping limit for allocation intensive programs. The default value for this is 2^16 ~ 65k mappings. You can change this value with the following command: `sudo sysctl -w vm.max_map_count=SOME_LARGE_NUMBER`.

## Report: 
see [this report](Report.pdf)
