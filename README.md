# tic_tac_toe_online

This is a c code for the tic-tac-toe game on the network. the network protocol of this project is TCP/IP.

*Note*: This project is not complete so this has very bug! if you see one of them please make a PL or issue

## build

in the Linux (Unix-based OS) you should just run the builder script:

``` ./builder ```

after this, you can see a folder with the name build.

and you can make an image for docker:

``` ./builde_docker ```

after this you can see to image with names : tic_tac_toe_server and tic_tac_toe_server

``` docker image list ```

## Run

To run the server you can use this command:

``` ./build/server ```

the default port is 8013. you can change it :

``` ./build/server [YOUR_PORT] ```

after you see the server is UP, you can run your client:

``` ./build/client ```

Note that you can run as many clients as you want in your system.
The limitation only applies when your RAM memory is full because it creates and manages users with linked lists.
