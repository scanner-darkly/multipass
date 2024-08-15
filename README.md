# multipass

multipass is a framework that makes it easier to build alternative firmwares for monome eurorack modules: white whale, earthsea, meadowphysics, ansible and teletype (eventually it might be expanded to other platforms).

essentially it's a lightweight layer over [libavr32](https://github.com/monome/libavr32) monome library which takes care of initializing the hardware, provides an event queue and timers and offers helper functions for interacting with various controllers and generating CVs and gates.

in short, it allows you to easily create your own applications without having to learn low level details. not only that, but you'll be able to code an app once and run it on any of the supported modules.

# getting started

- create a new repository, clone it locally
- navigate to the repo directory
- execute `git submodule add https://github.com/scanner-darkly/multipass.git`
- execute `git submodule update --init --recursive`
- create `src` folder and copy files from `multipass/src_template`
- set up one of the ways to build firmware (see below)
- build firmware
- if everything was set up properly, a firmware hex file will be generated
- now build your own app! see the [architecture section](#architecture) below and check out the [wiki](https://github.com/scanner-darkly/multipass/wiki)

# building firmware

there are 3 main options - for local dev i highly recommend using the docker option as it's the easiest to set up and use.

### docker option

if you run into any issues with the above option and can't figure them out, try using this docker image instead: https://github.com/Dewb/monome-build

to build using this option:

- open CMD
- navigate to the root folder of your firmware
- run:
```
del /s /q *.d,*.o
docker run -v %CD%:/target -t dewb/monome-build "cd multipass/monome_euro/ansible; make"
```
(replace "ansible" with whatever version you want to build)

### github actions

in order to use this option, you will need to host your repository on github and commit each time you want to build. however it's sufficiently fast to be usable and provides an additional benefit of helping you create releases targeting multiple modules.

- navigate to your repository folder
- create folder named `.github` and create a subfolder in it named `workflows`
- copy the files from `multipass/github_actions` folder to `workflows` folder
- commit to github
- open your github repository in a browser and navigate to `Actions` tab - you should see the workflows there

there are 2 workflows currently provided: `build firmware` and `upload release`.

`build firmware` has to be triggered manually but you can change it to trigger on each commit or pull request. a build typically takes 30-60 seconds. if there are no errors, it will build firmware for each specified module and upload hex files as artifacts to the workflow run (unfortunately, github will zip them when you download, simply unpack once you download them). the provided template targets teletype and ansible only, if you want to build for other modules simply edit `module` parameter under `matrix` in the .yml file.

`upload release` is triggered automatically when you create a new release. it will build firmware for each specified module and upload both hex and zip versions to your release as assets. **important**: unlike the previous action, this action will reference the commit the release tag is pointing to, not the latest commit!

both workflows will use the name of your repo for firmware files, if you prefer a different name simply modify the workflow templates.

### local toolchain

you will likely want to set up the toolchain locally so you can easily make and test changes. this option will require most work to set it up but it will save you time down the road. follow the instructions here: https://github.com/denravonska/avr32-toolchain

additional information is available here: https://github.com/monome/libavr32 and here: https://github.com/monome/teletype

please note that some of the instructions are out of date (atmel doesn't provide headers anymore), so adjust accordingly.

**important**: if you want to build for a different module make sure to remove all `*.o` files first.

# architecture

the main components are:
- `main.c` is the glue that ties everything together and takes care of figuring out how to talk to hardware. you should never need to modify it.

- `control.h` and `control.c` - your controller files. controller is responsible for talking to `main.c` and responding to any events (grid presses, MIDI notes etc). it's still aware of the fact it's talking to hardware, but it doesn't need to know any specific low level details - `main.c` will take care of it.

- `engine.h` and `engine.c` - your main app logic. this component shouldn't be aware of what hardware it's running on at all - it should be a totally abstracted app logic. this is so you can potentially re-use the code for some other platform - if you don't care about it, feel free to just leave the skeleton and do everything in the controller.

- `interface.h` - this is how the controller and `main.c` talk to each other. this is where all the possible actions and events are defined.

# samples

for some simple app examples check out:

- simple app (translates MIDI notes to CV/gates): https://github.com/scanner-darkly/multipass-simple
- shtnh control app (uses shnth as a controller to generate CV/gates): https://github.com/scanner-darkly/multipass-shnth

and for something more complex: https://github.com/scanner-darkly/polyearthsea
