Before generating the input files for Vivado using Bender, certaing files must be first generated. While this fork contains important generated files already copied to their correct paths, this step is still good to be ran, from the root directory of the project:
```
$ python -m venv .venv # create an environment for the dependencies of the generator scripts
$ source .venv/bin/activate # go into the environment
$ pip install -r requirements.txt
$ make idma_hw_all
$ deactivate # venv is no longer needed
```

Design/testbench files must be listed, depending on their checked out path. To do this using Bender, run the following first in this directory:
```
$ bender script vivado[-sim] --target test --target rtl --target idma_test --target synth --target simulation > add_files.tcl
```
The command above will generate a .tcl script which can be used in Vivado to add the sources to the project.

Thus, after creating a Vivado project, source the generated script to add all necessary project files (or source the file as needed in non-project mode).