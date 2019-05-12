import sys
import subprocess
import array
import os

path_to_CMA = "/home/yangxiny/Desktop/nuaf/CMA_benchmarks"
ld_preload = "LD_PRELOAD=/home/yangxiny/Desktop/nuaf/nuaf.so"

thread_amount = [1, 3, 5, 7, 9]
size_offset = [1, 50, 100, 200]
# Declare a dictionary
dict = {
    "cache-scratch": [
        path_to_CMA + "/cache-scratch/cache-scratch",
        "thread_placeholder",
        "100",
        "2000", #size
        "1000000",
    ],
    "cache-thrash": [
        path_to_CMA + "/cache-thrash/cache-thrash",
        "thread_placeholder",
        "100",
        "2000", #size
        "100000",
    ],
    "larson": [
        path_to_CMA + "/larson/larson",
        "10",
        "70", #obj size
        "80", #obj size
        "1000",
        "1000",
        "1",
        "thread_placeholder",
    ],
    "threadtest": [
        path_to_CMA + "/threadtest/threadtest",
        "thread_placeholder",
        "5000",
        "1000", 
        "0",
        "80", #size
    ],
    "linux-scalability": [
       path_to_CMA + "/linux-scalability/linux-scalability",
       "8", #obj size
       "10000000", 
       "thread_placeholder",
    ],
}

# Lists of the benchmarks we can run excluding splash group and netapps
benchmarks = [
    "cache-scratch", 
    "cache-thrash",
    "larson", 
    "threadtest", 
    # "linux-scalability", #killed
]

# Verify bash_cmd line arguments
if len(sys.argv) < 5:
    print(
        "Usage: python run_CMA.py [print/execute] [nuaf/null] [output.log] [benchmark1/all]  ... [benchmarkn]"
    )
else:
    if sys.argv[4] != "all":
        benchmarks = sys.argv[4:]
    if sys.argv[1] == "print":
        ld = ""
        bash_cmd = ""
        if sys.argv[2] == "nuaf":
            ld = ld_preload
        for benchmark in benchmarks:
            run_command = ""
            for frag in dict[benchmark]:
                run_command = run_command + " " + frag
            bash_cmd = bash_cmd + "echo; echo; >&2 echo --------" + benchmark + "--------;\n"
            bash_cmd = bash_cmd + ">&2 echo " + "\"--nuaf version--\"" + "; \n"
            for thread in thread_amount:
                threaded_command = run_command.replace("thread_placeholder", str(thread))
                bash_cmd = bash_cmd + ">&2 echo " + "\"thread = " + str(thread) + "\"; \n"
                bash_cmd = bash_cmd + " time " + ld + threaded_command + "; \n"
            
            bash_cmd = bash_cmd + ">&2 echo " + "\"--normal version--\"" + "; \n"
            for thread in thread_amount:
                threaded_command = run_command.replace("thread_placeholder", str(thread))
                bash_cmd = bash_cmd + ">&2 echo " + "\"thread = " + str(thread) + "\"; \n"
                bash_cmd = bash_cmd + " time " + threaded_command + "; \n"
        bash_cmd = "{ " + bash_cmd + " } 2> " + sys.argv[3] + "; \n"
        print(bash_cmd)
    else:  # execute:
        if sys.argv[2] == "all":
            # To run all benchmarks, first build and run the ones that use gcc.cfg and then the ones that use clang.cfg
            for benchmark in benchmarks:
                if benchmark in benchmarks:
                    subprocess.call(dict[benchmark])
                else:
                    print(
                        benchmark
                        + " is not in available parsec benchmark set -----------------"
                    )

# How to use:
# Put this script in ALEX directory and be in this directory
# `pushd` to parsec-benchmark directory and run `source env.sh`
# also make sure that you get the input and configure the bldconfig file with debug symbol flags
# `popd` back to ALEX directory
# Run `python parsec.py [print/execute] [nuaf/null] [benchmark1/all] ... [benchmarkn]`

# Caution:
# run-alex-benchmark.sh needs to be in ALEX directory to run parsec

