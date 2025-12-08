# display-RYB
This is a first version, not the final yet. So changes may still ocure. The updated version wil either be simplified, or a lot faster.\
It would be better to use shared memory directly, but that is not allowed by the wierd constrains of the project.
The libpynq source is [here](https://pynq.tue.nl/libpynq/5EWC0-2023-v0.2.6/index.html)
## To do:
 - [x] update write command.
 - [x] speed up display.
 - [x] prevent display write to limit input.
 - [] rewrite spi write command, by removing the sleep loops.
## more notes
- the previous version of the code, that is still slow is in a form in the time folder, to compare the speedup
 - see not yet tested branch for more features that are not yet fully working