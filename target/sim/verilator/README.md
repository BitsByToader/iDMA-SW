Before generating the input files for Verilator using Bender, certaing files must be first generated. While this fork contains important generated files already copied to their correct paths, this step is still good to be ran, from the root directory of the project:
```
$ python -m venv .venv # create an environment for the dependencies of the generator scripts
$ source .venv/bin/activate # go into the environment
$ pip install -r requirements.txt
$ make idma_hw_all
$ exit
```

Design/testbench files must be listed, depending on their checked out path. To do this using Bender, run the following first in this directory:
```
$ bender script verilator --target test --target rtl --target idma_test --target synth --target simulation > input.vc
```

To build and simulate the design, run `make` with the following arguments from this directory:
```
$ make top_module=YOUR_TOP_MODULE_NAME top_file=YOUR_TOP_FILE_WITH_TOP_MODULE
```

Or also redirect all outputs to a log file:
```
$ make top_module=YOUR_TOP_MODULE_NAME top_file=YOUR_TOP_FILE_WITH_TOP_MODULE 2>&1 | tee log.txt # Redirect all outputs to a log file
```