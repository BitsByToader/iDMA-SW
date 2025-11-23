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

# Verilator NOTE
At the time of writing this README, the reg_test.sv implementation in the register_interface repository doesn't play nicely with verilator.

The register driver requires a virtual interface of the REG_BUS interface, which is paramterized, however the AW and DW parameters don't have default values defined.
This results in a bunch of errors.
Adding a default value manually in the checked out folder fixes this, but verilator then can't reference virtual interfaces for which the interface has multiple parameterizations in the project.
Thus, it is needed to provide a correct default parameter which is used in the project, and the project can (???) have only one paramterization of this interface as to not trigger this scenario.
The workaround works the basic testbench proposed for the desc64 <-> axi stream wrapper.

Might be related:
[Verilator open issue 1](https://github.com/verilator/verilator/issues/4286)
[Verulator open issue 2](https://github.com/verilator/verilator/issues/2783)