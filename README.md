# sif

Search In Files, A command line tool to search for regex patterns inside all files in a selected directory, Written in c.


## Install

```bash
git clone https://github.com/BishrGhalil/sif.git
cd sif
sudo make install
```

## Usage

```bash
Usage: sif [options] -s <regex_pattern> -p <path>

Search for regex patterns inside files.

    -h, --help            show this help message and exit

Arguments
    -s, --search=<str>    Pattern to search for
    -p, --path=<str>      Directory path

Search options
    -u, --hidden          Search hidden folders

Output options
    -m, --matches         Matches number
    -l, --lines           Total searched lines
    -f, --files           Total searched files

Info options
    -v, --version         Version

Bishr Ghalil.
```
## Example
To search for all lines in my home directory that contain the word `github`:
```bash
sif -s "github" -p $HOME
```
To search for all gmail emails in my home directory:
```bash
sif -s "^[a-z0-9](\.?[a-z0-9]){5,}@g(oogle)?mail\.com$" -p $HOME
```
