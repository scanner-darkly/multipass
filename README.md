# multipass

multipass is a framework that makes it easier to build alternative firmwares for monome eurorack modules: white whale, earthsea, meadowphysics, ansible and teletype (eventually it might be expanded to other platforms).

it's a very lightweight layer over [libavr32](https://github.com/monome/libavr32) monome library which abstracts hardware input/output and some of the system functions.

another benefit of using multipass is - you can code an app once and then be able to build it for any of the supported modules.

for an example of an app using multipass check out [polyearthsea](https://github.com/scanner-darkly/polyearthsea).

# architecture

the main components are:
- `main.c` is the glue that ties everything together and takes care of figuring out how to talk to hardware. you never need to modify it.

- `control.h` and `control.c` - your controller files. controller is responsible for talking to `main.c` and responding to any events (grid presses, MIDI notes etc). it's still aware of the fact it's talking to hardware, but it doesn't need to know any specific low level details - `main.c` will take care of it.

- `engine.h` and `engine.c` - your main app logic. this component shouldn't be aware of what hardware it's running on at all - it should be a totally abstracted app logic. this is so you can potentially re-use the code for some other platform - if you don't care about it, feel free to just leave the skeleton and do everything in the controller.

- `interface.h` - this is how the controller and `main.c` talk to each other. this is where all the possible actions and events are defined.

# getting started

- set up toolchain as documented here: https://github.com/monome/libavr32
- create a new repository, clone it locally
- if you're planning to make modifications to multipass itself, make a fork of [multipass repo](https://github.com/scanner-darkly/multipass)
- navigate to the repo directory
- execute `git submodule add https://github.com/scanner-darkly/multipass.git` (change to your fork if needed)
- execute `git submodule update --init --recursive`
- create `src` folder and copy files from `multipass/src_template`
- navigate to `multipass/monome_euro` and then a directory of the module you want to build for and run `make`

at this point if you have the toolchain properly set up it should build a hex firmware file succesfully. **important**: if you want to build for a different module make sure to remove all `*.d` and `*.o` files first.

now build your app by modifying `control.c`. you can use this app as a sample: https://github.com/scanner-darkly/multipass-simple
