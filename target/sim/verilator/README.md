Design/testbench files must be listed, depending on their checked out path. To do this using bender, run the following first:
```
$ bender script verilator --target rtl --target idma_test --target synth > input.vc
```

To build and simulate the design, run `make` with the following arguments:
```
$ make top_module=YOUR_TOP_MODULE_NAME top_file=YOUR_TOP_FILE_WITH_TOP_MODULE
```
