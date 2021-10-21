# qbus_collection
A collection of QBUS libraries and modules

## Compile:

CMake is used to generate a configuration for your available compiler.

- Create a build folder:

*mkdir build*

*cd build*

- Generate the compiler configuration

*cmake ..*

- Compile

*make*

- Install

*make install*

## Run Webserver

There is a small webserver shipped with this framework to test some parts of the software. We assume to be in the build directory

- Create a public folder

*mkdir public*

- Create an index.html

*echo "hello world" > public/index.html*

- Start the webserver

*./libs/qwebs/app/qwebs_app*

