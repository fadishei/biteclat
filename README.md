# Introduction

This is an implementation of ECLAT algorithm for frequent itemset mining in C language which uses bitset compression for the vertical transaction data. It can use one of four different bitset compression algorithms:

- Roaring bitmaps
- Bitmagic
- CONCISE
- EWAH

The source code is developed for the purpose of experimets of a paper presented at the International Congress on High-Performance Computing and Big Data Analysis (TopHPC 2019). For more information, please refer to the [published paper](https://link.springer.com/chapter/10.1007/978-3-030-33495-6_10):

**Fadishei H., Doustian S., Saadati P. (2019) The Merits of Bitset Compression Techniques for Mining Association Rules from Big Data. In: Grandinetti L., Mirtaheri S., Shahbazian R. (eds) High-Performance Computing and Big Data Analysis. TopHPC 2019. Communications in Computer and Information Science, vol 891. Springer, Cham**

Powerpoint presentation of the paper can be found [here](https://www.slideshare.net/fadishei/the-merits-of-bitset-compression-techniques-for-mining-association-rules-from-big-data):

# Compiling

The selected bitset library can be specified at compile time using a `BITSET` environment variable. Its value can be one of `ROARING`, `BM`, `EWAH`, or `CONCISE`. For example:

    make clean
	BITSET=ROARING make

# Usage

Run `./eclat -h` to see the list of command line arguments.

    ./eclat -h
    usage: eclat [options]
    options:
    -d <dataset>  dataset file. csv of numbers. one transaction per line
    -f <frac>     fraction of transactions to process from start. default 1.0
    -h            print help
    -H            print header
    -m <sup>      minimum support. default 0.1
    -p            print frequent patterns
    -s            print stats
    -v            be verbose

The input file should be in the `dat` transaction format. Each line represents a transaction in which items are mentioned as 0-indexed integer values separated by space character.



