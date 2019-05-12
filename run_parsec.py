import sys
import subprocess
import array
import os

path_to_parsec = "/home/yangxiny/Desktop/parsec-benchmark/pkgs/"
ld_preload = "LD_PRELOAD=/home/yangxiny/Desktop/nuaf/nuaf.so"

# Declare a dictionary
dict = {
    "blackscholes": [
        path_to_parsec + "apps/blackscholes/inst/amd64-linux.gcc/bin/blackscholes",
        "1",
        path_to_parsec + "apps/blackscholes/run/in_10M.txt",
        path_to_parsec + "apps/blackscholes/run/prices.txt",
    ],
    "bodytrack": [
        path_to_parsec + "apps/bodytrack/inst/amd64-linux.gcc/bin/bodytrack",
        path_to_parsec + "apps/bodytrack/run/sequenceB_261",
        "4",
        "261",
        "4000",
        "5",
        "0",
        "1",
    ],
    "canneal": [
        path_to_parsec + "kernels/canneal/inst/amd64-linux.gcc/bin/canneal",
        "1",
        "15000",
        "2000",
        path_to_parsec + "kernels/canneal/run/2500000.nets",
        "6000",
    ],
    "facesim": [
        path_to_parsec + "apps/facesim/inst/amd64-linux.gcc/bin/facesim",
        "-timing",
        "-threads",
        "1",
        "-lastframe",
        "100",
    ],
    "ferret": [
        path_to_parsec + "apps/ferret/inst/amd64-linux.gcc/bin/ferret",
        path_to_parsec + "apps/ferret/run/corel",
        "lsh",
        path_to_parsec + "apps/ferret/run/queries",
        "50",
        "20",
        "1",
        path_to_parsec + "apps/ferret/run/output.txt",
    ],
    "fluidanimate": [
        path_to_parsec + "apps/fluidanimate/inst/amd64-linux.gcc/bin/fluidanimate",
        "1",
        "500",
        path_to_parsec + "apps/fluidanimate/run/in_500K.fluid",
        path_to_parsec + "apps/fluidanimate/run/out.fluid",
    ],
    "freqmine": [
        path_to_parsec + "apps/freqmine/inst/amd64-linux.gcc/bin/freqmine",
        path_to_parsec + "apps/freqmine/run/webdocs_250k.dat",
        "11000",
    ],
    "raytrace": [
        path_to_parsec + "apps/raytrace/run/thai_statue.obj",
        "-automove",
        "-nthreads",
        "1",
        "-frames",
        "200",
        "-res",
        "1920",
        "1080",
    ],
    "streamcluster": [
        path_to_parsec + "kernels/streamcluster/inst/amd64-linux.gcc/bin/streamcluster",
        "10",
        "20",
        "128",
        "1000000",
        "200000",
        "5000",
        "none",
        path_to_parsec + "apps/fluidanimate/run/output.txt",
        "1",
    ],
    "swaptions": [
        path_to_parsec + "apps/swaptions/inst/amd64-linux.gcc/bin/swaptions",
        "-ns",
        "128",
        "-sm",
        "1000000",
        "-nt",
        "1",
    ],
    "dedup": [
        path_to_parsec + "kernels/dedup/inst/amd64-linux.gcc/bin/dedup",
        "-c",
        "-p",
        "-v",
        "-t",
        "1",
        "-i",
        path_to_parsec + "kernels/dedup/run/FC-6-x86_64-disc1.iso",
        "-o",
        path_to_parsec + "kernels/dedup/run/output.dat.ddp",
    ],
    "vips": [
        path_to_parsec + "apps/vips/inst/amd64-linux.gcc/bin/vips",
        "im_benchmark",
        path_to_parsec + "apps/vips/run/orion_18000x18000.v",
        path_to_parsec + "apps/vips/run/output.v",
    ],
    "x264": [
        path_to_parsec + "apps/x264/inst/amd64-linux.gcc/bin/x264",
        "--quiet",
        "--qp",
        "20",
        "--partitions",
        "b8x8,i4x4",
        "--ref",
        "5",
        "--direct",
        "auto",
        "--weightb",
        "--mixed-refs",
        "--no-fast-pskip",
        "--me",
        "umh",
        "--subme",
        "7",
        "--analyse",
        "b8x8,i4x4",
        "--threads",
        "1",
        "-o",
        path_to_parsec + "apps/x264/run/eledream.264",
        path_to_parsec + "apps/x264/run/eledream_1920x1080_512.y4m",
    ],
}

# Lists of the benchmarks we can run excluding splash group and netapps
benchmarks = [
    "blackscholes",
    "bodytrack",
    # "canneal",
    "facesim", #other error
    "ferret",
    "fluidanimate",
    "freqmine",
    # "raytrace", permission denied
    "streamcluster",
    "swaptions",
    "dedup",
    "vips",
    "x264",
]

# Verify command line arguments
if len(sys.argv) < 5:
    print(
        "Usage: python run_parsec.py [print/execute] [nuaf/null] [output.log] [benchmark1/all]  ... [benchmarkn]"
    )
else:
    if sys.argv[4] != "all":
        benchmarks = sys.argv[4:]
    if sys.argv[1] == "print":
        ld = ""
        command = ""
        if sys.argv[2] == "nuaf":
            ld = ld_preload
        for benchmark in benchmarks:
            temp = ""
            for str in dict[benchmark]:
                temp = temp + " " + str
            command = (
                command + "echo; echo; >&2 echo --------" + benchmark + "--------;"
            )
            command = command + ">&2 echo " + "\"nuaf version\"" + "; "
            command = command + "time " + ld + temp + "; "
            command = command + ">&2 echo " + "\"normal version\"" + "; "
            command = command + "time " + "" + temp + "; "
        command = "{ " + command + " } 2> sss.log; "
        print(command)
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

